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

#ifndef _GENERIC_H
#define _GENERIC_H

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

#define HELPER_NUMtoSTR(x) _HELPER_NUMtoSTR(x)
#define _HELPER_NUMtoSTR(x) #x

#define __GET_FUNC2_NAME(_0, _1, NAME, ...) NAME
#define __GET_FUNC3_NAME(_0, _1, _2, NAME, ...) NAME
#define __GET_FUNC4_NAME(_0, _1, _2, _3, NAME, ...) NAME
#define __GET_FUNC5_NAME(_0, _1, _2, _3, _4, NAME, ...) NAME
#define __GET_FUNC6_NAME(_0, _1, _2, _3, _4, _5, NAME, ...) NAME

#define container_of(ptr, type, member) \
      ((type *)(((char*)ptr) - offsetof(type, member)))

#endif /* _WQ_HELPER_H */
