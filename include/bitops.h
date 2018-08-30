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
#ifndef _BIT_OPERATIONS_H_
#define _BIT_OPERATIONS_H_

#include <stdint.h>

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

CPP_SRC(extern "C" {)

static int
find_next_bit64(const uint64_t *addr, unsigned int bit)
{
	for (;bit < sizeof(uint64_t);++bit) {
		if (*addr & (1 << bit)) {
			break;
		}
	}
	return bit;
}

int
find_first_bit64(const uint64_t *addr)
{
	return find_next_bit64(addr, 0);
}

static int
find_next_zero_bit64(const uint64_t *addr, unsigned int bit)
{
	for (;bit < sizeof(uint64_t);++bit) {
		if (~(*addr) & (1 << bit)) {
			break;
		}
	}
	return bit;
}

int
find_first_zero_bit64(const uint64_t *addr)
{
	return find_next_zero_bit64(addr, 0);
}


// ON bitをループする
#define for_each_set_bit64(bit, addr)		\
	for((bit) = find_first_bit64(addr);	\
	    (bit) < sizeof(uint64_t);		\
	    (bit) = find_next_bit64((addr), (bit) + 1))

#define for_each_set_bit64_from(bit, addr)	\
	for((bit) = find_next_bit64(addr, 0);	\
	    (bit) < sizeof(uint64_t);		\
	    (bit) = find_next_bit64((addr), (bit) + 1))

// OFF bitをループする
#define for_each_clear_bit64(bit, addr)		\
	for((bit) = find_first_zero_bit64(addr);	\
	    (bit) < sizeof(uint64_t);		\
	    (bit) = find_next_zero_bit64((addr), (bit) + 1))

#define for_each_clear_bit64_from(bit, addr)	\
	for((bit) = find_next_zero_bit64(addr, 0);	\
	    (bit) < sizeof(uint64_t);		\
	    (bit) = find_next_zero_bit64((addr), (bit) + 1))

int
bit_ffs64(const uint64_t bits)
{
#ifdef __GNUC__
	return  __builtin_ffsll(bits);
#else
	uint64_t i = 0;

	if (bits == 0) {
		return 0;
	}

	for (i = 1; !(bits & (1 << i)); i++);
	return i;
#endif
}
int
bit_popcount64(const uint64_t bits)
{
#ifdef __GNUC__
	return __builtin_popcountll(bits);
#else
	bits = (bits & 0x5555555555555555) + (bits >> 1 & 0x5555555555555555);
	bits = (bits & 0x3333333333333333) + (bits >> 2 & 0x3333333333333333);
	bits = (bits & 0x0f0f0f0f0f0f0f0f) + (bits >> 4 & 0x0f0f0f0f0f0f0f0f);
	bits = (bits & 0x00ff00ff00ff00ff) + (bits >> 8 & 0x00ff00ff00ff00ff);
	bits = (bits & 0x0000ffff0000ffff) + (bits >>16 & 0x0000ffff0000ffff);
	return (bits & 0x00000000ffffffff) + (bits >>32 & 0x00000000ffffffff);
#endif
}

CPP_SRC(})

#endif /* _BIT_OPERATIONS_H_ */
