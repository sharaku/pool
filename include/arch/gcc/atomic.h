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
#ifndef _GCC_ATOMIC_H_
#define _GCC_ATOMIC_H_

#include <stdint.h>

typedef struct atomic32 {
	int32_t		counter;
} atomic32_t;
typedef struct atomic64 {
	int64_t		counter;
} atomic64_t;

#define ATOMIC_SET_OP(bit)								\
static inline void atomic##bit##_set(int##bit##_t i, atomic##bit##_t *v)		\
{											\
	return __atomic_store_n(&((v)->counter), i, __ATOMIC_RELAXED);			\
}

#define ATOMIC_RETURN_OP(bit)								\
static inline int##bit##_t atomic##bit##_return(atomic##bit##_t *v)			\
{											\
	return __atomic_load_n(&((v)->counter), __ATOMIC_RELAXED);			\
}

#define ATOMIC_OP(op, c_op, bit)							\
static inline void atomic##bit##_##op(int##bit##_t i, atomic##bit##_t *v)		\
{											\
	(void) __atomic_##c_op##_fetch(&((v)->counter), i, __ATOMIC_RELAXED);		\
}

#define ATOMIC_OP_RETURN(op, c_op, bit)							\
static inline int atomic##bit##_##op##_return(int##bit##_t i, atomic##bit##_t *v)	\
{											\
	return __atomic_##c_op##_fetch(&((v)->counter), i, __ATOMIC_RELAXED);		\
}

#define ATOMIC_FETCH_OP(op, c_op, bit)							\
static inline int atomic##bit##_fetch_##op(int##bit##_t i, atomic##bit##_t *v)		\
{											\
	return __atomic_fetch_##c_op(&((v)->counter), i, __ATOMIC_RELAXED);		\
}

#define ATOMIC32_INIT(i) { i }
#define ATOMIC64_INIT(i) { i }
#define init_atomic32(i, v)		atomic32_set(i, v)
#define init_atomic64(i, v)		atomic64_set(i, v)


ATOMIC_SET_OP(32)
ATOMIC_RETURN_OP(32)
ATOMIC_SET_OP(64)
ATOMIC_RETURN_OP(64)

ATOMIC_OP(add, add, 32)
ATOMIC_OP(sub, sub, 32)
ATOMIC_OP(and, and, 32)
ATOMIC_OP(xor, xor, 32)
ATOMIC_OP(or, or, 32)
ATOMIC_OP(nand, nand, 32)
ATOMIC_OP(add, add, 64)
ATOMIC_OP(sub, sub, 64)
ATOMIC_OP(and, and, 64)
ATOMIC_OP(xor, xor, 64)
ATOMIC_OP(or, or, 64)
ATOMIC_OP(nand, nand, 64)

ATOMIC_OP_RETURN(add, add, 32)
ATOMIC_OP_RETURN(sub, sub, 32)
ATOMIC_OP_RETURN(and, and, 32)
ATOMIC_OP_RETURN(xor, xor, 32)
ATOMIC_OP_RETURN(or, or, 32)
ATOMIC_OP_RETURN(nand, nand, 32)
ATOMIC_OP_RETURN(add, add, 64)
ATOMIC_OP_RETURN(sub, sub, 64)
ATOMIC_OP_RETURN(and, and, 64)
ATOMIC_OP_RETURN(xor, xor, 64)
ATOMIC_OP_RETURN(or, or, 64)
ATOMIC_OP_RETURN(nand, nand, 64)

ATOMIC_FETCH_OP(add, add, 32)
ATOMIC_FETCH_OP(sub, sub, 32)
ATOMIC_FETCH_OP(and, and, 32)
ATOMIC_FETCH_OP(xor, xor, 32)
ATOMIC_FETCH_OP(or, or, 32)
ATOMIC_FETCH_OP(nand, nand, 32)
ATOMIC_FETCH_OP(add, add, 64)
ATOMIC_FETCH_OP(sub, sub, 64)
ATOMIC_FETCH_OP(and, and, 64)
ATOMIC_FETCH_OP(xor, xor, 64)
ATOMIC_FETCH_OP(or, or, 64)
ATOMIC_FETCH_OP(nand, nand, 64)

#define atomic32_inc(v)		atomic32_add(1, v)
#define atomic32_dec(v)		atomic32_sub(1, v)
#define atomic64_inc(v)		atomic64_add(1, v)
#define atomic64_dec(v)		atomic64_sub(1, v)

// gccにおけるatomic
static inline void
mb(void)
{
	__sync_synchronize();
}


#endif // _GCC_ATOMIC_H_
