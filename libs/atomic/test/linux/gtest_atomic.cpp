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
 * SOFTWARE.
 *
 */

#include <atomic.h>
#include <gtest/gtest.h>

TEST(atomic, atomic32) {
	atomic32_t	a32 = ATOMIC32_INIT(0);
	EXPECT_EQ(atomic32_return(&a32), 0);

	init_atomic32(1, &a32);
	EXPECT_EQ(atomic32_return(&a32), 1);

	atomic32_add(2, &a32);
	EXPECT_EQ(atomic32_return(&a32), 3);

	atomic32_sub(3, &a32);
	EXPECT_EQ(atomic32_return(&a32), 0);

	EXPECT_EQ(atomic32_add_return(4, &a32), 4);
	EXPECT_EQ(atomic32_sub_return(5, &a32), -1);

	EXPECT_EQ(atomic32_fetch_add(6, &a32), -1);
	EXPECT_EQ(atomic32_fetch_sub(7, &a32), 5);

	// クリア
	init_atomic32(0, &a32);
	EXPECT_EQ(atomic32_return(&a32), 0);

	atomic32_inc(&a32);
	EXPECT_EQ(atomic32_return(&a32), 1);

	atomic32_dec(&a32);
	EXPECT_EQ(atomic32_return(&a32), 0);

	EXPECT_EQ(1, 1);
}

