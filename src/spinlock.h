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

#ifdef __GNUC__
#include <stdint.h>
CPP_SRC(extern "C" {)

// gccにおけるspinlock
struct ___spinlock_gcc {
	uint32_t	lock;
};

static inline void
init_spinlock_gcc(struct ___spinlock_gcc *sl)
{
	sl->lock = 0;
}

static inline int
spin_try_lock_gcc(struct ___spinlock_gcc *sl)
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
spin_lock_gcc(struct ___spinlock_gcc *sl)
{
	int32_t ret;

retry:
	ret = spin_try_lock_gcc(sl);
	if (ret) {
		goto retry;
	}
}

static inline void
spin_unlock_gcc(struct ___spinlock_gcc *sl)
{
	__sync_lock_release(&sl->lock);
}
#elif _WIN32
#include <intrin.h>
CPP_SRC(extern "C" {)

// gccにおけるspinlock
struct ___spinlock_win32 {
	long	lock;
};

static inline void
init_spinlock_win32(struct ___spinlock_win32 *sl)
{
	sl->lock = 0;
}

static inline int
spin_try_lock_win32(struct ___spinlock_win32 *sl)
{
	// lock変数が0の場合に1へ変更する。
	int32_t ret = _InterlockedExchange(&sl->lock, 1);
	if (ret) {
		// ロック失敗
		return 1;
	} else {
		// ロック成功
		return 0;
	}
}

static inline void
spin_lock_win32(struct ___spinlock_win32 *sl)
{
	int32_t ret;

retry:
	ret = spin_try_lock_win32(sl);
	if (ret) {
		goto retry;
	}
}

static inline void
spin_unlock_win32(struct ___spinlock_win32 *sl)
{
	_InterlockedExchange(&sl->lock, 0);
}
#else
#error "It is an incompatible build environment."
#endif

#ifdef __GNUC__
typedef struct ___spinlock_gcc spinlock_t;
#define SPINLOCK_INIT { 0 }
#define init_spinlock(l)	init_spinlock_gcc(l)
#define spin_try_lock(l)	spin_try_lock_gcc(l)
#define spin_lock(l)		spin_lock_gcc(l)
#define spin_unlock(l)		spin_unlock_gcc(l)

#elif _WIN32
typedef struct ___spinlock_win32 spinlock_t;
#define SPINLOCK_INIT { 0 }
#define init_spinlock(l)	init_spinlock_win32(l)
#define spin_try_lock(l)	spin_try_lock_win32(l)
#define spin_lock(l)		spin_lock_win32(l)
#define spin_unlock(l)		spin_unlock_win32(l)
#endif

CPP_SRC(})

#endif // _SPINLOCK_H_
