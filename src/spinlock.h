/* --
MIT License

Copyright (c) 2018 Abe Takafumi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**/

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

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

CPP_SRC(extern "C" {)

// gccにおけるspinlock
typedef struct spinlock {
	uint32_t	lock;
} spinlock_t;

#define SPINLOCK_INIT { 0 }

static inline void
init_spinlock(spinlock_t *sl)
{
	sl->lock = 0;
}

static inline int
spin_try_lock(spinlock_t *sl)
{
	// lock変数が0の場合に1へ変更する。
	int32_t ret = __sync_lock_test_and_set(&sl->lock, 1);
	if (ret) {
		// ロック失敗
		return 1;
	} else {
		// ロック成功
		return 0;
	}
}

static inline void
spin_lock(spinlock_t *sl)
{
	int32_t ret;

retry:
	ret = spin_try_lock(sl);
	if (ret) {
		goto retry;
	}
}

static inline void
spin_unlock(spinlock_t *sl)
{
	__sync_lock_release(&sl->lock);
}

CPP_SRC(})

#endif // _SPINLOCK_H_
