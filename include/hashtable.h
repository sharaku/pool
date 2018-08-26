/* --
 *
 * MIT License
 * 
 * Copyright (c) 2018 Abe Takafumi
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
 * SOFTWARE. *
 *
 */

#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

#include <stdlib.h>
#include <string.h>
#include <list.h>

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

typedef struct hash_node
{
	list_head_t		list;
	uint64_t		index;
	void			*value;
} hash_node_t;

typedef struct hash_root
{
	uint32_t		hash_sz;
	hlist_head_t		rnode[0];
} hash_root_t;

static inline hash_root_t *
hash_root_create(uint32_t hash_sz)
{
	hash_root_t *rootp;
	rootp = malloc(sizeof(hash_root_t) + sizeof(hash_node_t*) * hash_sz);
	memset(rootp, 0, sizeof(hash_root_t) + sizeof(hash_node_t*) * hash_sz);
	rootp->hash_sz = hash_sz;
	return rootp;
}

static inline void
hash_root_delete(hash_root_t *rootp)
{
	free(rootp);
}

static inline uint64_t
__hash_get_key(hash_root_t *rootp, uint64_t index)
{
	uint64_t key = (index >> 32) ^ (index & 0xffffffff);
	key &= rootp->hash_sz - 1;
	return key;
}

static inline int
hash_insert(hash_root_t *rootp, uint64_t index, void *entry)
{
	hash_node_t *nodep = malloc(sizeof(hash_node_t));
	init_list_head(&(nodep->list));
	nodep->index = index;
	nodep->value = entry;

	uint64_t key = __hash_get_key(rootp, index);
	hlist_add_tail(&(nodep->list), &rootp->rnode[key]);
}

static inline void *
hash_lookup(hash_root_t *rootp, uint64_t index)
{
	hash_node_t	*nodep;
	uint64_t key = __hash_get_key(rootp, index);

	if (rootp->rnode[key].first) {
		hlist_for_each_entry(nodep, &rootp->rnode[key], hash_node_t, list) {
			if (nodep->index == index) {
				return nodep->value;
			}
		}
	} else {
		return NULL;
	}
}

static inline void
hash_delete(hash_root_t *rootp, uint64_t index)
{
	hash_node_t	*nodep;
	uint64_t key = __hash_get_key(rootp, index);

	if (rootp->rnode[key].first) {
		hlist_for_each_entry(nodep, &rootp->rnode[key], hash_node_t, list) {
			if (nodep->index == index) {
				hlist_del_init(&nodep->list, &rootp->rnode[key]);
				return;
			}
		}
	}
}

#define hash_for_each(bkt, obj, rootp)				\
	for ((bkt) = 0; (bkt) < rootp->hash_sz; (bkt)++)	\
		if (&rootp->rnode[bkt]) 			\
			hlist_for_each_entry((obj), &rootp->rnode[bkt], hash_node_t, list)

static inline void
hash_dump(hash_root_t *rootp)
{
	uint32_t i;
	hash_node_t *nodep;

	hash_for_each(i, nodep, rootp) {
		if (!nodep) {
			continue;
		}
		printf(" index=0x%016lx value=%p\n", nodep->index, nodep->value);
	}
}

CPP_SRC(})

#endif
