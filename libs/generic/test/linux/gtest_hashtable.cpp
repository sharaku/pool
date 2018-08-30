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

#include <hashtable.h>
#include <gtest/gtest.h>

TEST(hashtable, hash_insert) {
	hash_root_t *hashrootp;
	hashrootp = hash_root_create(32);

	hash_insert(hashrootp, 0, (void*)1000);
	EXPECT_EQ(hash_lookup(hashrootp, 0), (void*)1000);

	hash_insert(hashrootp, 1, (void*)2000);
	EXPECT_EQ(hash_lookup(hashrootp, 1), (void*)2000);

	hash_insert(hashrootp, 2, (void*)3000);
	EXPECT_EQ(hash_lookup(hashrootp, 2), (void*)3000);

	hash_insert(hashrootp, 4, (void*)4000);
	EXPECT_EQ(hash_lookup(hashrootp, 4), (void*)4000);

	hash_root_delete(hashrootp);
}
