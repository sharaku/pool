/* --
MIT License

Copyright (c) 2018 Abe Takafumi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**/

// radix-treeへのentry追加方法
//  1. 最初の登録は必ずleafノードである。
//  2. 次のノードが1.のノードの中にあれば、それに登録する。
//     もし、1.と違う場合、分岐となるleafを追加して登録する。
//  3. 以降、2により、必要に応じて分岐を追加していく。
//
// radix-treeへのentry削除方法
//  1. 削除によってslotsのカウントが0となる場合はノードを破棄する。
//     ノード破棄時は上位のノードの参照を破棄する。
//  2. leafノード以外は参照が1になった場合に自身のparentをleafのparentとする。

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <slab.h>
#include <radix-tree.h>

static struct slab_cache *__node_slab;

void
radix_tree_init(void)
{
	__node_slab = slab_create_def(sizeof(radix_tree_node_t));
}

static inline radix_tree_node_t *
__radix_tree_node_alloc(struct radix_tree_root *root,
			uint8_t shift, uint8_t mapsize, uint64_t index)
{
	// slabからnodeを獲得する。
	radix_tree_node_t *nodep = slab_alloc(__node_slab);
	if (nodep) {
		uint64_t index_mask = (~RADIX_TREE_MAP_MASK64) << shift;;

		memset((void*)nodep, 0, sizeof(radix_tree_node_t));
		nodep->shift = shift;
		nodep->mapsize = mapsize;
		nodep->index = index & index_mask;
		nodep->root = root;
	}

	return nodep;
}

static inline uint8_t
__radix_tree_get_offset(radix_tree_node_t *nodep, uint64_t index)
{
	return (index >> nodep->shift) & RADIX_TREE_MAP_MASK64;
}

static inline void
__radix_tree_link_branch(radix_tree_node_t *nodep, radix_tree_node_t *parentp)
{
	uint8_t offset = __radix_tree_get_offset(parentp, nodep->index);
	nodep->offset = offset;
	nodep->parent = parentp;
	parentp->slots[offset] = nodep;
	parentp->count++;
}

static inline void
__radix_tree_link_leaf(radix_tree_node_t *nodep, void *entry, uint64_t index)
{
	uint8_t offset = __radix_tree_get_offset(nodep, index);
	nodep->slots[offset] = (void*)entry;
	nodep->count++;
}

static inline radix_tree_node_t *
__radix_tree_branch(radix_tree_node_t *nodep, uint64_t index)
{
	// nodeのindexと親nodeのindexの間に該当するものが存在する。
	// もし親nodeがNULLならnodeのindexと挿入するターゲットのindexの
	// 差分からnodeを見つけ出す。
	// つまり、見つけ出すべきshift値はnode->shift + RADIX_TREE_MAP_SHIFT64
	// から検索を開始し、parent->shift もしくは、parent->shiftがNULLの
	// 場合は60までの間に存在する。

	uint64_t index_mask;
	uint8_t shift;
	for (shift = nodep->shift + RADIX_TREE_MAP_SHIFT64,
	     index_mask = (~RADIX_TREE_MAP_MASK64) << shift;
	     (index & index_mask) != (nodep->index & index_mask);
	     shift += RADIX_TREE_MAP_SHIFT64,
	     index_mask = (~RADIX_TREE_MAP_MASK64) << shift);

	// nodepはNULLではない。つまり、所属するrootは存在する。
	// よってnodep->rootはNULLではない。
	struct radix_tree_root *rootp = nodep->root;
	radix_tree_node_t *parent = nodep->parent;

	// 新規ノードを作成し、つなげる。
	radix_tree_node_t *branch_nodep;
	branch_nodep = __radix_tree_node_alloc(rootp, shift, RADIX_TREE_MAP_SIZE64, index);
	__radix_tree_link_branch(nodep, branch_nodep);

	if (parent == NULL) {
		// 親がない場合、作成したノードをrootとする。
		rootp->rnode = branch_nodep;
	} else {
		// 親がある場合はその下にぶら下げる。
		// ただし、このルートは置き換えであるため、linkにて過剰に加算
		// した分を減算する。
		__radix_tree_link_branch(branch_nodep, parent);
		parent->count--;
	}

	// ブランチして新規ノードを作成する。
	radix_tree_node_t *new_nodep;
	new_nodep = __radix_tree_node_alloc(rootp, 0, RADIX_TREE_MAP_SIZE64, index);
	__radix_tree_link_branch(new_nodep, branch_nodep);

	return new_nodep;
}

int
radix_tree_insert(struct radix_tree_root *rootp, uint64_t index, void *entry)
{
	radix_tree_node_t *nodep = rootp->rnode;
	uint64_t index_mask;
	uint64_t offset;

	if (!nodep) {
		// 最初の登録
		// offsetはRADIX_TREE_MAP_MASK64でマスクしたもの。
		// ノードのindexは~RADIX_TREE_MAP_MASK64でマスクしたもの。
		nodep = __radix_tree_node_alloc(rootp, 0,
					        RADIX_TREE_MAP_SIZE64,
					        index);
		rootp->rnode = nodep;
		goto out;
	}

	// nodep->shiftが0であるが、indexが違うことはある。
	// その場合を考慮して最初にチェックを行う。
	index_mask = (~RADIX_TREE_MAP_MASK64) << nodep->shift;
	if ((index & index_mask) != nodep->index) {
		// 一番最上位がすでに違っていたらブランチする
		nodep = __radix_tree_branch(nodep, index);
		goto out;
	}

	while (nodep->shift) {
		// 下位へ降りる。
		offset = __radix_tree_get_offset(nodep, index);
		if (nodep->slots[offset] == NULL) {
			radix_tree_node_t *new_nodep;
			// NULLならブランチを切る。
			new_nodep = __radix_tree_node_alloc(rootp, 0,
							    RADIX_TREE_MAP_SIZE64,
							    index);
			__radix_tree_link_branch(new_nodep, nodep);
		}
		nodep = (radix_tree_node_t *)nodep->slots[offset];

		index_mask = (~RADIX_TREE_MAP_MASK64) << nodep->shift;
		if ((index & index_mask) != nodep->index) {
			// ノードが管理しているindexは挿入しようとしている
			// indexと異なる。
			// この場合、上位ノードを分割する。
			nodep = __radix_tree_branch(nodep, index);
			goto out;
		}
	}

out:
	__radix_tree_link_leaf(nodep, entry, index);
	return 0;
}

static inline radix_tree_node_t *
__radix_tree_lookup_node(radix_tree_root_t *rootp, uint64_t index)
{
	radix_tree_node_t *nodep = rootp->rnode;

	if (!nodep) {
		return NULL;
	}

	uint8_t offset;
	while (nodep->shift) {
		offset = __radix_tree_get_offset(nodep, index);
		nodep = nodep->slots[offset];
	}

	// 見つけたノードが目的のleafであるかチェック。
	// ちがうならNULL
	uint64_t index_mask = (~RADIX_TREE_MAP_MASK64) << 0;
	if ((index & index_mask) != nodep->index) {
		return NULL;
	} else {
		return nodep;
	}
}

void *
radix_tree_lookup(radix_tree_root_t *rootp, uint64_t index)
{
	radix_tree_node_t *nodep;
	nodep = __radix_tree_lookup_node(rootp, index);

	if (!nodep) {
		return NULL;
	} else {
		uint8_t offset = __radix_tree_get_offset(nodep, index);
		return nodep->slots[offset];
	}
}

static inline void
__radix_tree_shrink(radix_tree_node_t *nodep)
{
	radix_tree_node_t *parent;
	radix_tree_node_t *chiled;
	int i;

	// 縮めない場合、何もしない。
	if (nodep->count != 1) {
		return;
	}

	chiled = NULL;
	for (i = 0; i < RADIX_TREE_MAP_SIZE64; i++) {
		if (nodep->slots[i]) {
			chiled = (radix_tree_node_t *)nodep->slots[i];
			break;
		}
	}
	if (!chiled) {
		// ここに来るのはあり得ない。
		return;
	}

	parent = nodep->parent;
	// 最上位ノードの場合、rootノードとの置き換えになる。
	if (!parent) {
		goto release_top;
	}

	// ノードを入れ替える
	__radix_tree_link_branch(chiled, parent);
	parent->count--;

	nodep->count--;
	slab_put(nodep);
	return;

release_top:
	// rootノードと入れ替える
	nodep->root->rnode = chiled;
	nodep->count--;
	slab_put(nodep);
	return;
}

// radix-treeのノードを開放する。
// もし、この開放によって上位ノードが参照カウント1になった場合、
// 上位ノードを破棄してさらに上位のノードへマージする。
static inline void
__radix_tree_unlink_node(radix_tree_node_t *nodep, uint64_t index)
{
	uint8_t offset = __radix_tree_get_offset(nodep, index);
	radix_tree_node_t *parent;

	// 該当ノードがあれば削除する。
	// lookup後にするので必ず通過するはず...
	if (nodep->slots[offset]) {
		nodep->slots[offset] = NULL;
		nodep->count--;
		if (!nodep->count) {
			goto release;
		}
	}
	return;

release:
	parent = nodep->parent;
	if (parent) {
		// 上位の参照を開放する。
		// ちなみにこの操作によって上位が開放されることはあり得ない。
		// なぜならば上位は必ず複数参照だからである。
		// 単一参照になった段階で中抜きされて開放されるため、単一参照の
		// 上位ノードは存在しない。
		__radix_tree_unlink_node(parent, index);
		__radix_tree_shrink(parent);
	}
	slab_put(nodep);
}

int
radix_tree_delete(struct radix_tree_root *rootp, uint64_t index)
{
	radix_tree_node_t *nodep;

	// 削除対象のノードを検索し、発見出来たらそのノードの
	// 該当indexを削除する。
	// それに伴い、上位ノードの削除が必要になったら再帰的に削除していく。
	nodep = __radix_tree_lookup_node(rootp, index);
	if (!nodep) {
		return -ENOENT;
	} else {
		__radix_tree_unlink_node(nodep, index);
		return 0;
	}
}

static inline void
__radix_tree_node_dump(radix_tree_node_t *nodep, uint8_t offset)
{
	int i, spc;

	if (!nodep) {
		return;
	}

	for(spc = 0; spc < (64 - nodep->shift); spc++) {
		printf(" ");
	}
	printf("+ slot[%02u] node(%p) shift=%u offset=%u index=0x%016lx  parent=%p\n", offset, nodep, nodep->shift, nodep->offset, nodep->index, nodep->parent);
	if (nodep->shift) {
		for (i = 0; i < RADIX_TREE_MAP_SIZE64; i++) {
			__radix_tree_node_dump((radix_tree_node_t *)nodep->slots[i], i);
		}
	} else {
		for (i = 0; i < RADIX_TREE_MAP_SIZE64; i++) {
			if (!nodep->slots[i]) {
				continue;
			}
			for(spc = 0; spc < (64 - nodep->shift); spc++) {
				printf(" ");
			}
			printf(" slot[%02d]=%p  parent=%p\n", i, nodep->slots[i], nodep);
		}
	}
}

void
radix_tree_node_dump(radix_tree_node_t *nodep)
{
	__radix_tree_node_dump(nodep, 0);
}
