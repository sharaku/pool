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

#ifndef _SLAB_H
#define _SLAB_H

#ifdef __cplusplus
	#ifndef CPP_SRC
		#define CPP_SRC(x) x
	#endif
#else
	#ifndef CPP_SRC
		#define CPP_SRC(x)
	#endif
#endif

#include <libsharaku/container/plist.h>

#ifndef MEMORY_ALLOC
#ifdef __malloc_and_calloc_defined
#define MEMORY_ALLOC	malloc
#else // __malloc_and_calloc_defined
#warning "malloc and calloc not defined"
#define MEMORY_ALLOC	NULL
#endif // __malloc_and_calloc_defined
#endif // MEMORY_ALLOC

#ifndef MEMORY_FREE
#ifdef __malloc_and_calloc_defined
#define MEMORY_FREE	free
#else // __malloc_and_calloc_defined
#warning "malloc and calloc not defined"
#define MEMORY_FREE	NULL
#endif // __malloc_and_calloc_defined
#endif // MEMORY_FREE

CPP_SRC(extern "C" {)

// SLABを使用してメモリを確保する。
// 確保するメモリの前後にはヘッダ、フッタが付属する。
// ヘッダにはメモリを確保したソースと行数が記録されるため、デバッグに役に立つ。
// フッタにはmagicが記録される。これにより、確保領域を超えて書き込んだ場合の
// データ破壊検出に役に立つ。
// 
// SLABは密度の濃いものから順に取得される。
// 密度は(max_cnt - alloc_cnt) * N / max_cntの値で求められる。
// 先にNを掛けることで、整数計算のみで0 ～ N値へ変換する。
// この密度を優先度としてplistを使用して優先度順に並べる。
// Nを大きくすると、細かく段階を分けられるが抜き差しが大きくなり性能が
// 劣化する原因となる。
// 小さくすると、SLABが開放されにくくなる。
//
// 指定により、nodeサイズ、bufの最大数を変更できる。
//
// SLABの作成方法
//  - グローバル変数としてstruct slab_cacheを作成し、SLAB_INITを使用して作成
//  - slab_create*を使用して動的に作成。
//  - mallocでメモリを獲得、INIT_SLAB*を使用して初期化。(非推奨)
//    この方法の場合、slab解体時の全バッファ開放チェックは利用者の責任となります。

// SLABのサイズは1MB単位とする
#define SLAB_PRIO		10
#define SLAB_DEFAULT_SZ		1048576
#define SLAB_NODE_SZ_MIN	4096

typedef void (*slab_constructor)(void *buf, size_t sz);
typedef void (*slab_destructor)(void *buf, size_t sz);
typedef void *(*slab_mem_alloc)(size_t size);
typedef void (*slab_mem_free)(void *buf);

struct slab_cache {
	struct plist_head	s_list;		// 密度ごとのlist
	struct plist_head	s_flist;	// full list
	uint32_t		s_node_cnt;
	size_t			s_size;
	size_t			s_node_size;
	uint64_t		s_max_buf_cnt;
	uint64_t		s_buf_cnt;
	slab_constructor	s_constructor;
	slab_destructor		s_destructor;
	slab_mem_alloc		s_mem_alloc;
	slab_mem_free		s_mem_free;
};

struct slab_node {
	struct plist_node		sn_plist;
	struct list_head		sn_alist;
	struct list_head		sn_flist;
	uint32_t			sn_alloc_cnt;
	uint32_t			sn_max_cnt;
	struct slab_cache		*sn_slab;
};

// スラブを初期化する。（静的初期化）
// SLAB_NODE_INIT	Nodeのサイズ、bufの数を指定する。
// SLAB_NODE_INIT_SZ	Nodeのサイズのみ指定する。bufの数は無限。
// SLAB_NODE_INIT_DEF	Nodeのサイズは1MB、bufの数は無限。
#define SLAB_INIT(slab, size, node_size, max_cnt)	\
	{						\
		PLIST_HEAD_INIT((slab).s_list),		\
		PLIST_HEAD_INIT((slab).s_flist),	\
		0,					\
		size,					\
		node_size,				\
		max_cnt,				\
		0,					\
		NULL,					\
		NULL,					\
		MEMORY_ALLOC,				\
		MEMORY_FREE				\
	}

#define SLAB_INIT_SZ(slab, size, node_size)	\
	SLAB_INIT(slab, size, node_size, 0)

#define SLAB_INIT_DEF(slab, size)	\
	SLAB_INIT(slab, size, SLAB_DEFAULT_SZ, 0)

// slabを初期化する。
#define INIT_SLAB(slab, size, node_size, max_cnt)	\
	{						\
		init_plist_head(&(slab)->s_list);	\
		init_plist_head(&(slab)->s_flist);	\
		(slab)->s_node_cnt = 0;			\
		(slab)->s_size = size;			\
		(slab)->s_node_size = node_size;	\
		(slab)->s_max_buf_cnt = max_cnt;	\
		(slab)->s_buf_cnt = 0;			\
		(slab)->s_constructor = NULL;		\
		(slab)->s_destructor = NULL;		\
		(slab)->s_mem_alloc = MEMORY_ALLOC;	\
		(slab)->s_mem_free = MEMORY_FREE;	\
	}

#define INIT_SLAB_SZ(slab, size, node_size)	\
	INIT_SLAB(slab, size, node_size, 0)

#define INIT_SLAB_DEF(slab, size)	\
	INIT_SLAB(slab, size, SLAB_DEFAULT_SZ, 0)

// スラブからメモリを獲得する。
extern void* _slab_alloc(struct slab_cache *slab,
				   const char *src, uint32_t line);
// スラブから獲得したメモリを開放する。(参照カウント無視)
extern int slab_free(void *buf);
#define slab_alloc(slab)	\
		_slab_alloc(slab, __FILE__, __LINE__)

// スラブの参照カウントを加算する。
extern int slab_get(void *buf);

// スラブの参照カウントを取得する。
extern int slab_get_refcnt(void *buf);

// スラブから獲得したメモリの参照カウントを減算する。
extern int slab_put(void *buf);

static inline void
slab_set_constructor(struct slab_cache *slab, slab_constructor constructor)
{
	slab->s_constructor = constructor;
}

static inline void
slab_set_destructor(struct slab_cache *slab, slab_destructor destructor)
{
	slab->s_destructor = destructor;
}

static inline void
slab_set_mem_allocator(struct slab_cache *slab,
		       slab_mem_alloc mem_alloc, slab_mem_free mem_free)
{
	slab->s_mem_alloc = mem_alloc;
	slab->s_mem_free = mem_free;
}

CPP_SRC(})

#endif /* _SLAB_H */

