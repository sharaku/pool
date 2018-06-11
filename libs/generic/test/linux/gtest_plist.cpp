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

#include <plist.h>
#include <gtest/gtest.h>

TEST(plist, PLIST_HEAD_INIT) {
	struct plist_head	pl = PLIST_HEAD_INIT(pl);

	EXPECT_EQ(pl.node_list.next, &pl.node_list);
	EXPECT_EQ(pl.node_list.prev, &pl.node_list);
}

TEST(plist, PLIST_NODE_INIT) {
	struct plist_node	node = PLIST_NODE_INIT(node, 1);

	EXPECT_EQ(node.node_list.next, &node.node_list);
	EXPECT_EQ(node.node_list.prev, &node.node_list);
	EXPECT_EQ(node.prio_list.next, &node.prio_list);
	EXPECT_EQ(node.prio_list.prev, &node.prio_list);
	EXPECT_EQ(node.prio, 1);
}

TEST(plist, init_plist_head) {
	struct plist_head	pl;
	init_plist_head(&pl);

	EXPECT_EQ(pl.node_list.next, &pl.node_list);
	EXPECT_EQ(pl.node_list.prev, &pl.node_list);
}

TEST(plist, init_plist_node) {
	struct plist_node	node;
	init_plist_node(&node, 1);

	EXPECT_EQ(node.node_list.next, &node.node_list);
	EXPECT_EQ(node.node_list.prev, &node.node_list);
	EXPECT_EQ(node.prio_list.next, &node.prio_list);
	EXPECT_EQ(node.prio_list.prev, &node.prio_list);
	EXPECT_EQ(node.prio, 1);
}

TEST(plist, plist_empty) {
	int	rc;
	struct plist_head	pl = PLIST_HEAD_INIT(pl);
	struct plist_node	node = PLIST_NODE_INIT(node, 1);

	rc = plist_empty(&pl);
	EXPECT_NE(rc, 0);

	plist_add(&node, &pl);
	rc = plist_empty(&pl);
	EXPECT_EQ(rc, 0);
}

TEST(plist, plist_add) {
	struct plist_head	pl = PLIST_HEAD_INIT(pl);
	struct plist_node	node = PLIST_NODE_INIT(node, 1);
	plist_add(&node, &pl);
}

TEST(plist, plist_del) {
	int	rc;
	struct plist_head	pl = PLIST_HEAD_INIT(pl);
	struct plist_node	node = PLIST_NODE_INIT(node, 1);

	plist_add(&node, &pl);
	rc = plist_empty(&pl);
	EXPECT_EQ(rc, 0);

	plist_del(&node, &pl);
	rc = plist_empty(&pl);
	EXPECT_NE(rc, 0);
}

TEST(plist, plist_for_each_entry_safe) {
	struct test {
		int			num;
		struct plist_node	n;
	};
	int	i;
	int	count;
	struct plist_head	pl = PLIST_HEAD_INIT(pl);
	struct test		t[23];
	struct test		*pos;
	struct test		*n;

	for (i = 0; i < 23; i++) {
		t[i].num = i;
		init_plist_node(&t[i].n, 23 - i);
		plist_add(&t[i].n, &pl);
	}

	count = 0;
	plist_for_each_entry_safe(pos, n, &pl, struct test, n) {
		count++;
		EXPECT_EQ(pos->num, 23 - count);
	}
	EXPECT_EQ(count, 23);
}
