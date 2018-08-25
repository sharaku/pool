/* --
 *
 * MIT License
 * 
 * Copyright (c) 2014-2018 Abe Takafumi
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
 * SOFTWARE. *
 *
 */



#ifndef _STRING_FAST_H_
#define _STRING_FAST_H_

#include <generic.h>

CPP_SRC(extern "C" {)

// エラー判定しない、空白来ない、マイナス判定あり
static inline int
atoi_fast(const char* str)
{
	const char *work = str;
	int	result = 0;
	int	sign = 0;

	if (*work == '-') {
		sign = 1;
		work++;
	}

	while (1) {
		if (*work < '0' || *work > '9') {
			break;
		}
		result = result * 10 + *work - '0';
		work++;
	}

	if (sign) {
		result = -result;
	}

	return result;
}

static inline unsigned int
atou_fast(const char* str)
{
	const char *work = str;
	int	result = 0;

	while (1) {
		if (*work < '0' || *work > '9') {
			break;
		}
		result = result * 10 + *work - '0';
		work++;
	}

	return result;
}

// エラー判定しない、空白来ない、マイナス判定あり
// バッファは十分大きい
static inline int
itoa_fast(int i, char* str)
{
	char	work[16] = {0};
	char*	pwork = work;
	int	len = 0;

	if (i < 0) {
		*str = '-';
		str++;
		len++;
		i = -i;
	}
	while (1) {
		*pwork = '0' + i % 10;
		pwork++;
		i /= 10;
		if (!i) {
			break;
		}
	}
	// 反対に入っているので反転させる
	// 桁がわからないので、この方式とする。
	for (;pwork != work;) {
		pwork--;
		*str = *pwork;
		str++;
		len++;
	}
	*str = '\0';
	
	return len;
}

// エラー判定しない、空白来ない、マイナス判定あり
// バッファは十分大きい
static inline int
utoa_fast(unsigned int i, char* str)
{
	char	work[16] = {0};
	char*	pwork = work;
	int	len = 0;

	while (1) {
		*pwork = '0' + i % 10;
		pwork++;
		i /= 10;
		if (!i) {
			break;
		}
	}
	// 反対に入っているので反転させる
	for (;pwork != work;) {
		pwork--;
		*str = *pwork;
		str++;
		len++;
	}
	*str = '\0';
	
	return len;
}


CPP_SRC(})

#endif /* _STRING_FAST_H_ */
