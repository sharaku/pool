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

#ifndef _RADIX_TREE_H_
#define _RADIX_TREE_H_

#ifdef __cplusplus
	#ifndef CPP_SRC
		#define CPP_SRC(x) x
	#endif
	#if __cplusplus >= 201103L	// >= C++11
	
	#else				// < C++11

	#endif
#else
	#ifndef CPP_SRC
		#define CPP_SRC(x)
	#endif
#endif

CPP_SRC(extern "C" {)

#define RADIX_TREE_MAP_SHIFT64	(6)
#define RADIX_TREE_MAP_SIZE64	(1UL << RADIX_TREE_MAP_SHIFT64)
#define RADIX_TREE_MAP_MASK64	(RADIX_TREE_MAP_SIZE64 - 1)

typedef struct radix_tree_node
{
	uint8_t		shift;		// 0x00: Bits remaining in each slot
	uint8_t		offset;		// 0x01: 親ノードoffset
	uint8_t		count;		// 0x02: 参照カウント
	uint8_t		mapsize;	// 0x03: mapのサイズ
	uint32_t	rsv5;		// 0x04:
	uint64_t	index;		// 0x08: nodeが管理しているindex
	struct radix_tree_node *parent;	// 0x10: 親ノードのアドレス
	struct radix_tree_root *root;		// 0x18: rootノード
	void		*slots[RADIX_TREE_MAP_SIZE64];	// 0x20: 下位のアドレス
} radix_tree_node_t;

typedef struct radix_tree_root
{
	radix_tree_node_t	*rnode;		// 0x00: rootノード
} radix_tree_root_t;

void radix_tree_init(void);
int radix_tree_insert(struct radix_tree_root *root,unsigned long index, void *entry);
void * radix_tree_lookup(radix_tree_root_t *rootp, uint64_t index);
int radix_tree_delete(struct radix_tree_root *rootp, uint64_t index);
void radix_tree_node_dump(radix_tree_node_t *nodep);

CPP_SRC(})

#endif
