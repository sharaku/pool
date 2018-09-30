/*-
 *
 * MIT License
 * 
 * Copyright (c) 2014 Abe Takafumi
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
// Linuxカーネルのlist.h上と互換のあるインタフェースを移植。
// Linux, Itron, Windowsでビルドできるように対応。
// C/C++でビルドできるように対応
// entry系の処理は互換性がない。

#ifndef _LIST_H_
#define _LIST_H_

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

#include <stdint.h>
#include <stddef.h>

CPP_SRC(extern "C" {)

typedef struct list_head {
	struct list_head *next, *prev;
} list_head_t;

#ifndef container_of
#define container_of(ptr, type, member) \
      ((type *)(((char*)ptr) - offsetof(type, member)))
#endif

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void
init_list_head(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline int
list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline int
list_empty_careful(const struct list_head *head)
{
	struct list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

static inline void
__list_add(struct list_head *new_,
	   struct list_head *prev,
	   struct list_head *next)
{
	next->prev = new_;
	new_->next = next;
	new_->prev = prev;
	prev->next = new_;
}

static inline void
list_add(struct list_head *new_, struct list_head *head)
{
	__list_add(new_, head, head->next);
}

static inline void
list_add_tail(struct list_head *new_, struct list_head *head)
{
	__list_add(new_, head->prev, head);
}

static inline void
__list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void
__list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void
list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = (struct list_head*)0x00001111;
	entry->prev = (struct list_head*)0x00002222;
}

static inline void
list_replace(struct list_head *old,
	     struct list_head *new_)
{
	new_->next = old->next;
	new_->next->prev = new_;
	new_->prev = old->prev;
	new_->prev->next = new_;
}

static inline void
list_replace_init(struct list_head *old,
		  struct list_head *new_)
{
	list_replace(old, new_);
	init_list_head(old);
}

static inline void
list_del_init(struct list_head *entry)
{
	__list_del_entry(entry);
	init_list_head(entry);
}

static inline void
list_move(struct list_head *list, struct list_head *head)
{
	__list_del_entry(list);
	list_add(list, head);
}

static inline void
list_move_tail(struct list_head *list,
	       struct list_head *head)
{
	__list_del_entry(list);
	list_add_tail(list, head);
}

static inline int
list_is_last(const struct list_head *list,
	     const struct list_head *head)
{
	return list->next == head;
}

static inline void
list_rotate_left(struct list_head *head)
{
	struct list_head *first;

	if (!list_empty(head)) {
		first = head->next;
		list_move_tail(first, head);
	}
}

static inline int
list_is_singular(const struct list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

static inline void
__list_cut_position(struct list_head *list,
		    struct list_head *head, struct list_head *entry)
{
	struct list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

static inline void
list_cut_position(struct list_head *list,
		  struct list_head *head,
		  struct list_head *entry)
{
	if (list_empty(head))
		return;
	if (list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		init_list_head(list);
	else
		__list_cut_position(list, head, entry);
}

static inline void
__list_splice(const struct list_head *list,
	      struct list_head *prev,
	      struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

static inline void
list_splice(const struct list_head *list,
	    struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}

static inline void
list_splice_tail(struct list_head *list,
		 struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head->prev, head);
}

static inline void
list_splice_init(struct list_head *list,
		 struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head, head->next);
		init_list_head(list);
	}
}

static inline void
list_splice_tail_init(struct list_head *list,
		      struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head->prev, head);
		init_list_head(list);
	}
}

#define list_entry(ptr, type, member)				\
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member)			\
	list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member)			\
	list_entry((ptr)->prev, type, member)

#define list_first_entry_or_null(ptr, type, member)		\
	(!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)

#define list_next_entry(pos, type, member)			\
	list_entry((pos)->member.next, type, member)

#define list_next_entry_or_null(pos, head, type, member)	\
	((pos)->next != (head) ? list_first_entry(pos, type, member) : NULL)

#define list_prev_entry(pos, type, member)			\
	list_entry((pos)->member.prev, type, member)

#define list_for_each(pos, head)				\
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head)				\
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_for_each_safe(pos, n, head)			\
	for (pos = (head)->next, n = pos->next; pos != (head);	\
		pos = n, n = pos->next)

#define list_for_each_prev_safe(pos, n, head)			\
	for (pos = (head)->prev, n = pos->prev;			\
	     pos != (head); \
	     pos = n, n = pos->prev)

#define list_for_each_entry(pos, head, type, member)		\
	for (pos = list_first_entry(head, type, member);	\
	     &pos->member != (head);				\
	     pos = list_next_entry(pos, type, member))

#define list_for_each_entry_reverse(pos, head, type, member)	\
	for (pos = list_last_entry(head, type, member);		\
	     &pos->member != (head); 				\
	     pos = list_prev_entry(pos, type, member))

#define list_prepare_entry(pos, head, type, member) \
	((pos) ? : list_entry(head, type, member))

#define list_for_each_entry_continue(pos, head, type, member) 	\
	for (pos = list_next_entry(pos, type, member);		\
	     &pos->member != (head);				\
	     pos = list_next_entry(pos, type, member))

#define list_for_each_entry_continue_reverse(pos, head, type, member) \
	for (pos = list_prev_entry(pos, type, member);		\
	     &pos->member != (head);				\
	     pos = list_prev_entry(pos, type, member))

#define list_for_each_entry_from(pos, head, type, member) 	\
	for (; &pos->member != (head);				\
	     pos = list_next_entry(pos, type, member))

#define list_for_each_entry_safe(pos, n, head, type, member)	\
	for (pos = list_first_entry(head, type, member),	\
		n = list_next_entry(pos, type, member);		\
	     &pos->member != (head); 				\
	     pos = n, n = list_next_entry(n, type, member))

#define list_for_each_entry_safe_continue(pos, n, head, type, member) \
	for (pos = list_next_entry(pos, type, member), 		\
		n = list_next_entry(pos, type, member);		\
	     &pos->member != (head);				\
	     pos = n, n = list_next_entry(n, member))

#define list_for_each_entry_safe_from(pos, n, head, type, member) \
	for (n = list_next_entry(pos, type, member);		\
	     &pos->member != (head);				\
	     pos = n, n = list_next_entry(n, type, member))

#define list_for_each_entry_safe_reverse(pos, n, head, type, member) \
	for (pos = list_last_entry(head, type, member),		\
		n = list_prev_entry(pos, type, member);		\
	     &pos->member != (head); 				\
	     pos = n, n = list_prev_entry(n, type, member))

#define list_safe_reset_next(pos, n, type, member)		\
	n = list_next_entry(pos, type, member)

// ハッシュリスト制御
typedef struct hlist_head {
	list_head_t *first;
} hlist_head_t;

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) hlist_head_t name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)

static inline int hlist_empty(const hlist_head_t *h)
{
	return !(h->first);
}

// ハッシュの次のエントリを取得する。
// 最後のノードならNULLを返す。
#define hlist_first_entry_or_null(pos, head, type, member)	\
	((hlist_empty(head)) ? NULL : list_entry((head)->first, type, member))
#define hlist_next_entry_or_null(pos, head, type, member)	\
	(((pos)->member.next == (head)->first) ? \
	  NULL : list_entry((pos)->member.next, type, member))

static inline void
hlist_add_tail(list_head_t *new_, hlist_head_t *head)
{
	if (head->first) {
		list_add_tail(new_, head->first);
	} else {
		head->first = new_;
	}
}

static inline void
hlist_del_init(list_head_t *entry, hlist_head_t *head)
{
	
	if (!list_empty(entry)) {
		// 空でなければ、削除。
		if (head->first == entry) {
			head->first = entry->next;
		}
		list_del_init(entry);
	} else {
		// 空かつhead->firstとどういつならhead->firstはNULLにする。
		if (head->first == entry) {
			head->first = NULL;
		}
	}
}

// ハッシュエントリをループする
#define hlist_for_each_entry(pos, head, type, member)			\
	for (pos = list_entry((head)->first, type, member);		\
	     pos;							\
	     pos = hlist_next_entry_or_null(pos, head, type, member))

#define hlist_for_each_entry_from(pos, head, type, member)		\
	for (;								\
	     pos;							\
	     pos = hlist_next_entry_or_null(pos, head, type, member))

#define hlist_for_each_entry_safe(pos, n, head, type, member)		\
	for (pos = list_entry((head)->first, type, member),		\
	     n = (pos) ? hlist_next_entry_or_null(pos, head, type, member) : NULL; \
	     pos;							\
	     pos = n, n = (pos) ? hlist_next_entry_or_null(pos, head, type, member) : NULL)

CPP_SRC(})

#endif // _LIST_H_
