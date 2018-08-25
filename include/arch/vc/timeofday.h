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

#ifndef _VC_TIMEOFDAY_H_
#define _VC_TIMEOFDAY_H_

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

#ifndef _WIN32
#error "not supported."
#endif

#include <intrin.h>
#include <windows.h>
#pragma intrinsic(__rdtsc)

#define	GENERIC_FT2NSEC(FT)	(((int64_t)((FT)->dwHighDateTime) << 32) + (int64_t)((FT)->dwLowDateTime))
#define	GENERIC_FT2USEC(FT)	(GENERIC_FT2NSEC(FT) / 1000)
static inline int64_t
__generic_get_usec(void)
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return GENERIC_FT2USEC(&ft);

}

extern uint64_t __freq;

static inline uint64_t
generic_rdtsc(void)
{
	uint64_t counter;
	counter = __rdtsc();
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
	Sleep(ms);
}

#endif /* WQ_H_ */
