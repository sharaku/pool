/* --
 *
 * MIT License
 * 
 * Copyright (c) 2017 Abe Takafumi
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <stdlib.h>
#include <errno.h>
#include <slab.h>

#define _SLAB_MAGIC	0xF324ABE3
// メモリバッファのヘッダ。
// 利用者からは参照できない領域
typedef struct smem_header {
	uint32_t		h_magic;
	uint32_t		h_line;
	const char		*h_src;
	struct slab_node	*h_node;
	struct list_head	h_list;
} smem_header_t;

// メモリバッファのフッタ。
// 利用者からは参照できない領域
typedef struct mem_footer {
	uint32_t		f_magic;
} smem_footer_t;

// slab獲得の優先度を計算する。
static inline int64_t
__get_slab_prio(struct slab_node *node)
{
	return (node->sn_max_cnt - node->sn_alloc_cnt)
			 * SLAB_PRIO / node->sn_max_cnt;
}

// ヘッダからフッタを取得する
static inline smem_footer_t*
__slab_h2f(smem_header_t *h)
{
	return (smem_footer_t*)
			((char *)(h)
				 + h->h_node->sn_slab->s_size
				 + sizeof(smem_header_t));
}

// ヘッダからバッファを取得する
static inline void*
__slab_h2b(smem_header_t *h)
{
	return (void*)(h + 1);
}

// バッファからヘッダを取得する
static inline smem_header_t*
__slab_b2h(void *buf)
{
	return (smem_header_t *)(((char *)buf) - sizeof(smem_header_t));
}

// slab獲得の優先度キューの再登録を行う。
static inline void
__slab_resched(struct slab_cache *slab,
			struct slab_node *node)
{
	int64_t	prio;

	plist_del(&node->sn_plist, &slab->s_list);
	// 空きがある場合は空きリストに入れる
	// fullの場合はfullリストに入れる
	// リストは同一優先度の末端に挿入されるため、抜き差しが頻発する
	// 可能性は少ない。
	prio = __get_slab_prio(node);
	node->sn_plist.prio = prio;
	if (prio) {
		plist_add(&node->sn_plist, &slab->s_list);
	} else {
		plist_add(&node->sn_plist, &slab->s_flist);
	}
}

// nodeからメモリを獲得する
static inline void *
__slab_alloc(struct slab_node *node,
			const char *src, uint32_t line)
{
	smem_header_t *h;
	smem_footer_t *f;

	h = list_first_entry_or_null(&node->sn_flist, smem_header_t, h_list);
	if (!h) {
		return NULL;
	}
	list_del_init(&h->h_list);
	list_add_tail(&h->h_list, &node->sn_alist);
	h->h_magic = _SLAB_MAGIC;
	h->h_src = src;
	h->h_line = line;
	h->h_node = node;

	f = __slab_h2f(h);
	f->f_magic = _SLAB_MAGIC;

	node->sn_alloc_cnt++;
	return __slab_h2b(h);
}

// nodへメモリを返却する
static inline int
__slab_free_nochk(smem_header_t *h, struct slab_node *node)
{
	list_del_init(&h->h_list);
	list_add_tail(&h->h_list, &node->sn_flist);
	node->sn_alloc_cnt--;

	return 0;
}

// nodeへメモリを返却する
static inline int
__slab_free(void *buf)
{
	struct slab_node *node;
	smem_header_t *h;
	smem_footer_t *f;

	h = __slab_b2h(buf);
	if (h->h_magic != _SLAB_MAGIC) {
		// 不正アクセス。
		return -EFAULT;
	}

	f = __slab_h2f(h);
	node = h->h_node;

	if (f->f_magic != _SLAB_MAGIC) {
		// 不正アクセス。
		return -EFAULT;
	}

	__slab_free_nochk(h, node);
	return 0;
}

static inline void
__slab_node_alloc(struct slab_cache *slab)
{
	struct slab_node *node;
	smem_header_t *h;
	char *buf;
	int buf_sz;
	int i;

	buf_sz = slab->s_size + sizeof(smem_header_t) + sizeof(smem_footer_t);
	node = (struct slab_node *)malloc(slab->s_node_size);

	init_plist_node(&node->sn_plist, 0);
	init_list_head(&node->sn_alist);
	init_list_head(&node->sn_flist);
	node->sn_max_cnt
		 = (slab->s_node_size - sizeof(struct slab_node))
		 						 / buf_sz;
	node->sn_slab = slab;

	// 最初にすべての領域を獲得しているものとして初期化。
	// その後、全領域をfreeすることでリストにつなげつつallocカウンタを
	// 適切に集計する。
	node->sn_alloc_cnt	= node->sn_max_cnt;
	buf = (char *)(node + 1);
	for (i = 0; i < node->sn_max_cnt; i++) {
		h = (smem_header_t *)buf;
		init_list_head(&h->h_list);
		__slab_free_nochk(h, node);
		buf += buf_sz;
	}
	__slab_resched(slab, node);

	slab->s_node_cnt++;
}

// node1枚を開放する。
// ただし、1つでもallocされている場合は開放しない。
static int
__slab_node_free(struct slab_node *node)
{
	struct slab_cache *slab;

	// allocされている場合は開放してはならない。
	// -EBUSYを応答する。
	if (node->sn_alloc_cnt) {
		return -EBUSY;
	}
	slab = node->sn_slab;
	plist_del(&node->sn_plist, &slab->s_list);
	free(node);

	slab->s_node_cnt--;
	return 0;
}

// slabからメモリを獲得する。
// 空きがない場合は新しいslabを獲得する。
void*
_slab_alloc(struct slab_cache *slab,
		   const char *src, uint32_t line)
{
	struct slab_node *node;
	char *buf = NULL;
	int prio = -1;

	// 最大バッファ数を超える場合は獲得させない。
	if (slab->s_max_buf_cnt &&
	    slab->s_max_buf_cnt <= slab->s_buf_cnt) {
		return (void*)-EINVAL;
	}

	if (plist_empty(&slab->s_list)) {
		__slab_node_alloc(slab);
	}

	node = list_entry(slab->s_list.node_list.next,
				struct slab_node,
				sn_plist.node_list);
	if (node) {
		prio = __get_slab_prio(node);
		buf = __slab_alloc(node, src, line);
		if (slab->s_constructor) {
			slab->s_constructor((void*)buf, slab->s_size);
		}
		node->sn_slab->s_buf_cnt++;
	} else {
		// このルートを通ることはない。もし通過する場合バグである。
	}
	// もしslabの獲得により優先度に変化が発生した時は、nodeを入れなおす。
	if (prio != __get_slab_prio(node)) {
		__slab_resched(slab, node);
	}
	return buf;
}

int
slab_free(void *buf)
{
	struct slab_node *node;
	smem_header_t *h;
	int prio;
	int rc;

	if (!buf) {
		// 不正アクセス。
		return -EFAULT;
	}
	h = __slab_b2h(buf);
	if (h->h_magic != _SLAB_MAGIC) {
		// 不正アクセス。
		return -EFAULT;
	}
	node = h->h_node;
	if (!node) {
		// 不正アクセス。
		return -EFAULT;
	}
	prio = __get_slab_prio(node);
	if (node->sn_slab->s_destructor) {
		node->sn_slab->s_destructor((void*)buf, node->sn_slab->s_size);
	}
	rc = __slab_free(buf);
	node->sn_slab->s_buf_cnt--;
	if (rc) {
		return rc;
	}
	// もしslabの獲得により優先度に変化が発生した時は、nodeを入れなおす。
	if (prio != __get_slab_prio(node)) {
		__slab_resched(node->sn_slab, node);
	}
	return 0;
}

struct slab_cache*
slab_create(size_t size, size_t node_size, uint32_t max_cnt)
{
	struct slab_cache *slab;

	if (node_size < SLAB_NODE_SZ_MIN) {
		return NULL;
	}

	slab = (struct slab_cache *)malloc(sizeof(struct slab_cache));
	INIT_SLAB(slab, size, node_size, max_cnt);
	return slab;
}

int
slab_destroy(struct slab_cache *slab)
{
	if (slab->s_node_cnt) {
		return -EBUSY;
	}
	free(slab);
	return 0;
}

