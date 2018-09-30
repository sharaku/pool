/*-
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
 * SOFTWARE. *
 *
 */

#ifndef _OBJ_POOL_HPP_
#define _OBJ_POOL_HPP_

#ifdef __cplusplus
	#ifndef CPP_SRC
		#define CPP_SRC(x) x
	#endif
#else
	#ifndef CPP_SRC
		#define CPP_SRC(x)
	#endif
#endif

// ゲームオブジェクトpoolを管理するコンテナ
#include <errno.h>
#include <libsharaku/container/list.h>

template<typename T>
struct obj_pool {
	typedef void (*constructor_t)(T *object);
	typedef void (*destructor_t)(T *object);

	obj_pool() {
		__constructor = NULL;
		__destructor = NULL;
		init_list_head(&__pool);
		__objp = NULL;
	}

	obj_pool(int32_t cnt, T *addr = NULL) {
		__constructor = NULL;
		__destructor = NULL;
		init_list_head(&__pool);
		__objp = NULL;

		initialize(cnt, addr);
	}

	~obj_pool() {
		if (__objp) {
			free(__objp);
		}
		init_list_head(&__pool);
	}

	int initialize(int32_t cnt, T *addr = NULL) {
		int idx;
		init_list_head(&__pool);

		if (!addr) {
			__objp = (T*)malloc(sizeof(T) * cnt);
		} else {
			__objp = addr;
		}
		if (!__objp) {
			return -errno;
		}
		for (idx = 0; idx < cnt; idx++) {
			__free(&__objp[idx]);
		}
		return 0;
	}

	// poolからメモリを獲得する
	T *alloc(void) {
		list_head_t *new_;
		if (!list_empty(&__pool)) {
			new_ = __pool.next;
			list_del_init(new_);
			if (__constructor) {
				__constructor((T *)new_);
			}
			return (T *)new_;
		} else {
			return NULL;
		}
	}
	void free(T *obj) {
		if (__destructor) {
			__destructor(obj);
		}
		__free(obj);
	}

protected:
	void __free(T *obj) {
		list_head_t *listp = (list_head_t *)obj;
		init_list_head(listp);
		list_add_tail(listp, &__pool);
	}

	constructor_t	__constructor;
	destructor_t	__destructor;
	list_head_t	__pool;
	T		*__objp;
};

#endif // _OBJ_POOL_HPP_
