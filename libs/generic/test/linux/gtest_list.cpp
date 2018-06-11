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

#include <list.h>
#include <gtest/gtest.h>

TEST(list, LIST_HEAD_INIT) {
	struct list_head	list = LIST_HEAD_INIT(list);

	EXPECT_EQ(list.next, &list);
	EXPECT_EQ(list.prev, &list);
}

TEST(list, LIST_HEAD) {
	LIST_HEAD(list);

	EXPECT_EQ(list.next, &list);
	EXPECT_EQ(list.prev, &list);
}

TEST(list, init_list_head) {
	struct list_head	list;

	list.next = (struct list_head *)0x00001111;
	list.prev = (struct list_head *)0x00002222;
	init_list_head(&list);

	EXPECT_EQ(list.next, &list);
	EXPECT_EQ(list.prev, &list);
}

TEST(list, list_empty) {
	LIST_HEAD(list);
	LIST_HEAD(head);
	LIST_HEAD(list2);
	list_add(&list2, &head);

	EXPECT_NE(list_empty(&list), 0);
	EXPECT_EQ(list_empty(&head), 0);
}

TEST(list, list_empty_careful) {
	LIST_HEAD(list);
	LIST_HEAD(head);
	LIST_HEAD(list2);
	list_add(&list2, &head);

	EXPECT_NE(list_empty_careful(&list), 0);
	EXPECT_EQ(list_empty_careful(&head), 0);
}

TEST(list, list_add) {
	LIST_HEAD(head);
	LIST_HEAD(list1);
	LIST_HEAD(list2);

	list_add(&list1, &head);
	list_add(&list2, &head);

	EXPECT_EQ(head.next, &list2);
	EXPECT_EQ(head.prev, &list1);
	EXPECT_EQ(list1.next, &head);
	EXPECT_EQ(list1.prev, &list2);
	EXPECT_EQ(list2.next, &list1);
	EXPECT_EQ(list2.prev, &head);
}

TEST(list, list_add_tail) {
	LIST_HEAD(head);
	LIST_HEAD(list1);
	LIST_HEAD(list2);

	list_add_tail(&list1, &head);
	list_add_tail(&list2, &head);

	EXPECT_EQ(head.next, &list1);
	EXPECT_EQ(head.prev, &list2);
	EXPECT_EQ(list1.next, &list2);
	EXPECT_EQ(list1.prev, &head);
	EXPECT_EQ(list2.next, &head);
	EXPECT_EQ(list2.prev, &list1);
}

TEST(list, list_del) {
	LIST_HEAD(head);
	LIST_HEAD(list1);
	LIST_HEAD(list2);

	list_add_tail(&list1, &head);
	list_add_tail(&list2, &head);
	list_del(&list2);

	EXPECT_EQ(list2.next, (struct list_head*)0x00001111);
	EXPECT_EQ(list2.prev, (struct list_head*)0x00002222);
}

TEST(list, list_replace) {
}

TEST(list, list_replace_init) {
}

TEST(list, list_del_init) {
}

TEST(list, list_move) {
}

TEST(list, list_move_tail) {
}

TEST(list, list_is_last) {
}

TEST(list, list_rotate_left) {
}

TEST(list, list_is_singular) {
}

TEST(list, list_cut_position) {
}

TEST(list, list_splice) {
}

TEST(list, list_splice_tail) {
}

TEST(list, list_splice_tail_init) {
}

TEST(list, list_entry) {
	struct list_entry_test {
		int			a;
		int			b;
		int			c;
		struct list_head	list;
	} list;
	struct list_head *lp1;
	struct list_entry_test *lp2;

	lp1 = &list.list;
	lp2 = list_entry(lp1, struct list_entry_test, list);

	EXPECT_EQ((void*)lp2, (void*)&list);
}

TEST(list, list_first_entry) {
}

TEST(list, list_last_entry) {
}

TEST(list, list_first_entry_or_null) {
}

TEST(list, list_next_entry) {
}

TEST(list, list_prev_entry) {
}

TEST(list, list_for_each) {
}

TEST(list, list_for_each_prev) {
}

TEST(list, list_for_each_safe) {
}

TEST(list, list_for_each_prev_safe) {
}

TEST(list, list_for_each_entry) {
}

TEST(list, list_for_each_entry_reverse) {
}

TEST(list, list_prepare_entry) {
}

TEST(list, list_for_each_entry_continue) {
}

TEST(list, list_for_each_entry_continue_reverse) {
}

TEST(list, list_for_each_entry_from) {
}

TEST(list, list_for_each_entry_safe) {
}

TEST(list, list_for_each_entry_safe_continue) {
}

TEST(list, list_for_each_entry_safe_from) {
}

TEST(list, list_for_each_entry_safe_reverse) {
}

TEST(list, list_safe_reset_next) {
}

