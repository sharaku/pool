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

#include <libsharaku/pool/slab.h>
#include <gtest/gtest.h>

TEST(slab, SLAB_INIT) {
	struct slab_cache slab = SLAB_INIT(slab, sizeof(int), 1048576, 101);

	EXPECT_EQ(slab.s_list.node_list.next, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_list.node_list.prev, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_flist.node_list.next, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_flist.node_list.prev, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_node_cnt, 0);
	EXPECT_EQ(slab.s_size, sizeof(int));
	EXPECT_EQ(slab.s_node_size, 1048576);
	EXPECT_EQ(slab.s_max_buf_cnt, 101);
	EXPECT_EQ(slab.s_buf_cnt, 0);
}

TEST(slab, SLAB_INIT_SZ) {
	struct slab_cache slab = SLAB_INIT_SZ(slab, sizeof(int), 1048576);

	EXPECT_EQ(slab.s_list.node_list.next, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_list.node_list.prev, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_flist.node_list.next, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_flist.node_list.prev, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_node_cnt, 0);
	EXPECT_EQ(slab.s_size, sizeof(int));
	EXPECT_EQ(slab.s_node_size, 1048576);
	EXPECT_EQ(slab.s_max_buf_cnt, 0);
	EXPECT_EQ(slab.s_buf_cnt, 0);
}

TEST(slab, SLAB_INIT_DEF) {
	struct slab_cache slab = SLAB_INIT_DEF(slab, sizeof(int));

	EXPECT_EQ(slab.s_list.node_list.next, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_list.node_list.prev, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_flist.node_list.next, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_flist.node_list.prev, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_node_cnt, 0);
	EXPECT_EQ(slab.s_size, sizeof(int));
	EXPECT_EQ(slab.s_node_size, 1048576);
	EXPECT_EQ(slab.s_max_buf_cnt, 0);
	EXPECT_EQ(slab.s_buf_cnt, 0);
}

TEST(slab, INIT_SLAB) {
	struct slab_cache slab;
	INIT_SLAB(&slab, sizeof(int), 1048576, 101);

	EXPECT_EQ(slab.s_list.node_list.next, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_list.node_list.prev, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_flist.node_list.next, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_flist.node_list.prev, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_node_cnt, 0);
	EXPECT_EQ(slab.s_size, sizeof(int));
	EXPECT_EQ(slab.s_node_size, 1048576);
	EXPECT_EQ(slab.s_max_buf_cnt, 101);
	EXPECT_EQ(slab.s_buf_cnt, 0);
}

TEST(slab, INIT_SLAB_SZ) {
	struct slab_cache slab;
	INIT_SLAB_SZ(&slab, sizeof(int), 1048576);

	EXPECT_EQ(slab.s_list.node_list.next, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_list.node_list.prev, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_flist.node_list.next, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_flist.node_list.prev, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_node_cnt, 0);
	EXPECT_EQ(slab.s_size, sizeof(int));
	EXPECT_EQ(slab.s_node_size, 1048576);
	EXPECT_EQ(slab.s_max_buf_cnt, 0);
	EXPECT_EQ(slab.s_buf_cnt, 0);
}

TEST(slab, INIT_SLAB_DEF) {
	struct slab_cache slab;
	INIT_SLAB_DEF(&slab, sizeof(int));

	EXPECT_EQ(slab.s_list.node_list.next, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_list.node_list.prev, &slab.s_list.node_list);
	EXPECT_EQ(slab.s_flist.node_list.next, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_flist.node_list.prev, &slab.s_flist.node_list);
	EXPECT_EQ(slab.s_node_cnt, 0);
	EXPECT_EQ(slab.s_size, sizeof(int));
	EXPECT_EQ(slab.s_node_size, 1048576);
	EXPECT_EQ(slab.s_max_buf_cnt, 0);
	EXPECT_EQ(slab.s_buf_cnt, 0);
}

TEST(slab, slab_create) {
	struct slab_cache *slab;
	slab = slab_create(sizeof(int), 1048576, 101);

	EXPECT_EQ(slab->s_list.node_list.next, &slab->s_list.node_list);
	EXPECT_EQ(slab->s_list.node_list.prev, &slab->s_list.node_list);
	EXPECT_EQ(slab->s_flist.node_list.next, &slab->s_flist.node_list);
	EXPECT_EQ(slab->s_flist.node_list.prev, &slab->s_flist.node_list);
	EXPECT_EQ(slab->s_node_cnt, 0);
	EXPECT_EQ(slab->s_size, sizeof(int));
	EXPECT_EQ(slab->s_node_size, 1048576);
	EXPECT_EQ(slab->s_max_buf_cnt, 101);
	EXPECT_EQ(slab->s_buf_cnt, 0);

	slab_destroy(slab);
}

TEST(slab, slab_create_sz) {
	struct slab_cache *slab;
	slab = slab_create_sz(sizeof(int), 1048576);

	EXPECT_EQ(slab->s_list.node_list.next, &slab->s_list.node_list);
	EXPECT_EQ(slab->s_list.node_list.prev, &slab->s_list.node_list);
	EXPECT_EQ(slab->s_flist.node_list.next, &slab->s_flist.node_list);
	EXPECT_EQ(slab->s_flist.node_list.prev, &slab->s_flist.node_list);
	EXPECT_EQ(slab->s_node_cnt, 0);
	EXPECT_EQ(slab->s_size, sizeof(int));
	EXPECT_EQ(slab->s_node_size, 1048576);
	EXPECT_EQ(slab->s_max_buf_cnt, 0);
	EXPECT_EQ(slab->s_buf_cnt, 0);

	slab_destroy(slab);
}

TEST(slab, slab_create_def) {
	struct slab_cache *slab;
	slab = slab_create_def(sizeof(int));

	EXPECT_EQ(slab->s_list.node_list.next, &slab->s_list.node_list);
	EXPECT_EQ(slab->s_list.node_list.prev, &slab->s_list.node_list);
	EXPECT_EQ(slab->s_flist.node_list.next, &slab->s_flist.node_list);
	EXPECT_EQ(slab->s_flist.node_list.prev, &slab->s_flist.node_list);
	EXPECT_EQ(slab->s_node_cnt, 0);
	EXPECT_EQ(slab->s_size, sizeof(int));
	EXPECT_EQ(slab->s_node_size, 1048576);
	EXPECT_EQ(slab->s_max_buf_cnt, 0);
	EXPECT_EQ(slab->s_buf_cnt, 0);

	slab_destroy(slab);
}

TEST(slab, slab_destroy) {
	struct slab_cache *slab;
	slab = slab_create(sizeof(int), 1048576, 101);

	EXPECT_EQ(slab->s_list.node_list.next, &slab->s_list.node_list);
	EXPECT_EQ(slab->s_list.node_list.prev, &slab->s_list.node_list);
	EXPECT_EQ(slab->s_flist.node_list.next, &slab->s_flist.node_list);
	EXPECT_EQ(slab->s_flist.node_list.prev, &slab->s_flist.node_list);
	EXPECT_EQ(slab->s_node_cnt, 0);
	EXPECT_EQ(slab->s_size, sizeof(int));
	EXPECT_EQ(slab->s_node_size, 1048576);
	EXPECT_EQ(slab->s_max_buf_cnt, 101);
	EXPECT_EQ(slab->s_buf_cnt, 0);

	slab_destroy(slab);
}

TEST(slab, _slab_alloc) {
	struct slab_cache *slab;
	char *slab_bufer[1024];
	int i;
	int rc;

	slab = slab_create(256, 1048576, 128);
	for (i = 0; i < 128; i++) {
		slab_bufer[i] = (char*)_slab_alloc(slab, __FILE__, __LINE__);
	}
	for (i = 128; i < 256; i++) {
		slab_bufer[i] = (char*)_slab_alloc(slab, __FILE__, __LINE__);
	}

	for (i = 0; i < 128; i++) {
		rc = slab_free(slab_bufer[i]);
		EXPECT_EQ(rc, 0);
	}

	slab_destroy(slab);
}

TEST(slab, slab_free) {
	struct slab_cache *slab;
	char *slab_bufer[1024];
	int i;
	int rc;

	slab = slab_create(256, 1048576, 128);
	for (i = 0; i < 128; i++) {
		slab_bufer[i] = (char*)_slab_alloc(slab, __FILE__, __LINE__);
	}

	for (i = 0; i < 128; i++) {
		rc = slab_free(slab_bufer[i]);
		EXPECT_EQ(rc, 0);
	}

	slab_destroy(slab);
}

TEST(slab, slab_alloc) {
	struct slab_cache *slab;
	char *slab_bufer[1024];
	int i;
	int rc;

	slab = slab_create(256, 1048576, 128);
	for (i = 0; i < 128; i++) {
		slab_bufer[i] = (char*)slab_alloc(slab);
	}

	for (i = 0; i < 128; i++) {
		rc = slab_free(slab_bufer[i]);
		EXPECT_EQ(rc, 0);
	}

	slab_destroy(slab);
}

TEST(slab, slab_set_constructor) {
}

TEST(slab, slab_set_destructor) {
}


