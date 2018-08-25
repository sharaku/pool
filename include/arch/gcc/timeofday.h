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

#ifndef _GCC_TIMEOFDAY_H_
#define _GCC_TIMEOFDAY_H_

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

// 周波数取得は変数代入と同等の速度である。
// 周波数からus, msへの変換には除算が使用されるが、これはとても遅い。
// 時刻を気にしないのであれば周波数のまま使用するとよい。


extern uint64_t __freq;

#ifndef __linux__
#error "not supported."
#endif

#include <unistd.h>
#include <stddef.h>
#include <sys/time.h>

#define	GENERIC_TV2USEC(TV)	((TV)->tv_sec * 1000000 + (TV)->tv_usec)
static inline int64_t
__generic_get_usec(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return GENERIC_TV2USEC(&tv);

}

static inline uint64_t
generic_rdtsc(void)
{
	uint64_t  counter;
#if defined  __x86_64__
	unsigned hi, lo;
	__asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
	counter = ((uint64_t)hi) << 32 | (uint64_t)lo;
#elif defined __arm__
	__asm__ volatile ("isb; mrs %0, cntvct_el0" : "=r" (counter));
#else
#error "It is an incompatible build environment."
#endif
	return counter;
}

// rdtscで取得できた値の1秒の周波数
static inline uint64_t
generic_getfreq(void)
{
	return __freq;
}

static inline void
generic_msleep(uint64_t ms)
{
	usleep(ms * 1000);
}

#endif /* WQ_H_ */
