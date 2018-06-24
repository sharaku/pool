/* --
 *
 * MIT License
 * 
 * Copyright (c) 2015 Abe Takafumi
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

#ifndef _DEVGRP_H_
#define _DEVGRP_H_

#include <stdint.h>
#include <devfile/devfile.hpp>
#include <wq/wq-event.h>
#include <bitops.h>


class devgrp
{
public:
	int32_t update(uint32_t flag);
	int32_t commit(uint32_t flag);
	int32_t is_complete(void) {
		int32_t result = 0;
		spin_lock(&_lock);
		if (_flag & FLG_IO_COMPLETE) {
			result = 1;
		}
		spin_unlock(&_lock);
		return result;
	}
	int32_t stop(void) {
		spin_lock(&_lock);
		_flag |= FLG_IO_CANCEL;
		spin_unlock(&_lock);
		return 0;
	}

	int regist(uint64_t id, devfile_rw *devfile) {
		// 引数idはbitが1立っている必要がある。
		if (bit_popcount64(id) != 1) {
			return EINVAL;
		}
		if (!devfile) {
			return EINVAL;
		}
		// どこかは立っているので、立っているbitを - 1すると
		// indexがわかる。
		_devices[bit_ffs64(id) - 1] = devfile;
	}
protected:
	// io submitのループ。完了するまでここが回る。
	// 実際にはビジーループではなくイベントで待ち合わせを行う。
	// これによりCPUを別の目的で使用可能。
	int __read_submit(void);
	int __write_submit(void);
	void __submit(void);
	static void __io_event(struct wq_item *item, wq_arg_t arg);

	enum {
		FLG_IO_EXEC         = 0x0001,
		FLG_IO_CANCEL       = 0x0002,
		FLG_IO_COMPLETE     = 0x0004,
	};

private:
	devfile_rw *_devices[64];
	uint64_t _upd_bits;
	uint64_t _cmt_bits;
	uint32_t _flag;
	spinlock_t _lock;
	wq_ev_item_t _item_ev;
};

#endif // _DEVGRP_H_
