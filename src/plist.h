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

//  lh:list_head
//  pl:prio_list
//  nl:node_list
// 
//  < 複数登録の状態 >
//      +-----------------------------------------------+
//      |                                               |
//      +->|lh|<->|nl|<->|nl|<-->|nl|<--->|nl|<-->|nl|<-+
//             +----------------------------------------+
//             |  |  |   |  |    |  |     |  |    |  |  |
//             +->|pl|<->|pl|<------------------->|pl|<-+
//                |  |   |  |  +------+ +------+  |  |
//                |  |   |  |  | |  | | | |  | |  |  |
//                |  |   |  |  +>|pl|<+ +>|pl|<+  |  |
//                |01|   |02|    |02|     |02|    |10|   (prioメンバ)
//
// list_for_eachでアクセスする場合、先頭の構造が同じであるため、
// キャストするだけで使える。
//      +-----------------------------------------------+
//      |                                               |
//      +->|lh|<->|nl|<->|nl|<-->|nl|<--->|nl|<-->|nl|<-+

#ifndef _PLIST_H
#define _PLIST_H

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

#include <list.h>

CPP_SRC(extern "C" {)

// 構造を合わせるため、plist_headを定義する。
// plist_head, plist_nodeをlist_headでキャストしてlist_for_eachできるように
// node_listを頭に持ってくる。
typedef struct plist_head {
	struct list_head	node_list;
} plist_head_t;

typedef struct plist_node {
	struct list_head	node_list;
	struct list_head	prio_list;
	int64_t			prio;
} plist_node_t;

#define PLIST_HEAD_INIT(head)			\
	{						\
		LIST_HEAD_INIT((head).node_list)	\
	}						\

#define PLIST_NODE_INIT(node, _prio)	\
	{						\
		LIST_HEAD_INIT((node).node_list),	\
		LIST_HEAD_INIT((node).prio_list),	\
		_prio					\
	}


#define init_plist_head(head) init_list_head(&(head)->node_list);
static inline void
init_plist_node(struct plist_node *node, int prio)
{
	node->prio = prio;
	init_list_head(&node->prio_list);
	init_list_head(&node->node_list);
}

static inline int
plist_empty(const struct plist_head *head)
{
	return head->node_list.next == &head->node_list;
}

#define plist_first_entry(hd, type, member)			\
	 list_entry((hd)->node_list.next, type, member)

static inline void
plist_add(struct plist_node *node, struct plist_head *head)
{
	struct plist_node *first = NULL;
	struct plist_node *iter = NULL;
	struct plist_node *prev = NULL;
	struct list_head *next = &head->node_list;

	if (list_empty(&head->node_list)) {
		// 最初の登録であれば無条件に登録する。
		// 
		// < 初期登録状態 >
		//  +---------------+
		//  |               |
		//  +->|lh|<->|nl|<-+
		//         +->|pl|<---------+
		//         |  |02| (prio)   |
		//         |                |
		//         +----------------+
		goto insert;
	}

	// 2回目以降の登録であれば挿入する位置を検索する挿入は同一prioの
	// 最後に行う。
	// これにより、prio順に昇順でソートされる。かつ、新規登録はprio内の
	// 最後に入る
	// prevがNULLの場合は先頭に挿入する。
	// iter == firstの場合は末尾に挿入する。
	// それ以外の場合はiterの直前に登録する。
	first = list_entry(head->node_list.next,
			struct plist_node, node_list);
	iter = first;
	do {
		if (node->prio < iter->prio) {
			next = &iter->node_list;
			break;
		}
		prev = iter;
		iter = list_entry(iter->prio_list.next,
					struct plist_node, prio_list);
	} while (iter != first);

	// prio_listを構築する。
	// iterは挿入位置のprioのlist_head。
	// つまり1つ前が今登録したprioでなければならない。
	// prioが一致しない場合は、prio_listの登録を行う。
	if (!prev || prev->prio != node->prio) {
		list_add_tail(&node->prio_list, &iter->prio_list);
	}
insert:
	list_add_tail(&node->node_list, next);
}

static inline void
plist_del(struct plist_node *node, struct plist_head *head)
{
	struct plist_node *next;
	// prioの先頭である場合、次のnodeに先頭をバトンタッチする。
	// ただし、末尾の場合（nodeのnextがhead）は抜くだけでよい。
	if (!list_empty(&node->prio_list)) {
		if (node->node_list.next != &head->node_list) {
			next = list_entry(node->node_list.next,
					struct plist_node, node_list);
			if (node->prio == next->prio) {
				list_add(&next->prio_list, &node->prio_list);
			}
		}
		list_del_init(&node->prio_list);
	}
	list_del_init(&node->node_list);
}

#define plist_for_each_entry_safe(pos, n, head, type, member)	\
	 list_for_each_entry_safe(pos, n, &(head)->node_list, type, member.node_list)

CPP_SRC(})

#endif // _PLIST_H_

