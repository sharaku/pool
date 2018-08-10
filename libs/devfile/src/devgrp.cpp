/* --
 *
 * MIT License
 * 
 * Copyright (c) 2014 Abe Takafumi
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


#include <bitops.h>
#include <log/log.h>
#include <devfile/devgrp.hpp>
#include <generic.h>
#include <wq/wq-event.h>
#include <wq/wq.h>

int32_t
devgrp::update(uint32_t flag)
{
	int exec = 0;

	spin_lock(&_lock);
	_upd_bits |= flag;
	_flag &= ~(FLG_IO_COMPLETE | FLG_IO_CANCEL);
	if (likely(!(_flag & FLG_IO_EXEC))) {
		// 起動していないなら起動する。
		_flag |= FLG_IO_EXEC;
		exec = 1;
	}
	spin_unlock(&_lock);

	if (exec) {
		__submit();
	}
	return 0;
}

int32_t
devgrp::commit(uint32_t flag)
{
	int exec = 0;

	spin_lock(&_lock);
	_cmt_bits |= flag;
	_flag &= ~(FLG_IO_COMPLETE | FLG_IO_CANCEL);
	if (likely(!(_flag & FLG_IO_EXEC))) {
		// 起動していないなら起動する。
		_flag |= FLG_IO_EXEC;
		exec = 1;
	}
	spin_unlock(&_lock);

	if (exec) {
		__submit();
	}
	return 0;
}


int
devgrp::__read_submit(void)
{
	uint32_t bit;
	int rc;

	// 立っているbitを元にしてreadを発行する。
	// もし、EAGAINであった場合はイベント待ちキューに積む。
	for_each_set_bit64(bit, &(_upd_bits)) {
		if (unlikely(_flag == FLG_IO_CANCEL)) {
			rc = ECANCELED;
			wq_infolog64("update canceled. rc=%d", rc);
			break;
		}
		rc = _devices[bit]->update();
		if (unlikely(rc == EAGAIN)) {
			wq_ev_init(&_item_ev, _devices[bit]->get_id());
			wq_set_evitem_arg(&_item_ev, (void*)this);
			wq_ev_sched(&_item_ev, WQ_EVFL_FDIN, devgrp::__io_event);
			wq_infolog64("update retryed. rc=%d", rc);
			break;
		} else if (unlikely(rc != 0)) {
			// readできない場合はログをとって次へ。
			wq_infolog64("update error. rc=%d", rc);

			// ひとまず正常扱いにしておく。
			// ここで正常にしないと、
			// 最後の場合はエラーが返ってしまう。
			rc = 0;
		} else {
			// 正常に読み込めた場合は次へ。
		}
		spin_lock(&_lock);
		_upd_bits &= ~(1 << bit);
		spin_unlock(&_lock);
	}
	return rc;
}

int
devgrp::__write_submit(void)
{
	uint32_t bit;
	int rc;

	// 立っているbitを元にしてwriteを発行する。
	// もし、EAGAINであった場合はイベント待ちキューに積む。
	for_each_set_bit64(bit, &(_cmt_bits)) {
		if (unlikely(_flag == FLG_IO_CANCEL)) {
			rc = ECANCELED;
			wq_infolog64("commit canceled. rc=%d", rc);
			break;
		}
		rc = _devices[bit]->commit();
		if (unlikely(rc == EAGAIN)) {
			wq_ev_init(&_item_ev, _devices[bit]->get_id());
			wq_set_evitem_arg(&_item_ev, (void*)this);
			wq_ev_sched(&_item_ev, WQ_EVFL_FDOUT, devgrp::__io_event);
			wq_infolog64("commit retryed. rc=%d", rc);
			break;
		} else if (unlikely(rc != 0)) {
			// writeできない場合はログをとって次へ。
			wq_infolog64("commit error. rc=%d", rc);

			// ひとまず正常扱いにしておく。
			// ここで正常にしないと、
			// 最後の場合はエラーが返ってしまう。
			rc = 0;
		} else {
			// 正常に書き込めた場合は次へ。
		}
		spin_lock(&_lock);
		_cmt_bits &= ~(1 << bit);
		spin_unlock(&_lock);
	}
	return rc;
}


void
devgrp::__submit(void)
{
	int rc;
	int retry = 0;

	do {
		rc = __read_submit();
		if (unlikely(rc)) {
			// EAGAIN, ECANCELEDを検出。ここでいったん終了する。
			// EAGAINの場合はイベント起動済み
			// ECANCELEDの場合は即時停止
			break;
		}

		rc = __write_submit();
		if (unlikely(rc)) {
			// EAGAIN, ECANCELEDを検出。ここでいったん終了する。
			// EAGAINの場合はイベント起動済み
			// ECANCELEDの場合は即時停止
			break;
		}

		// 状態を変更する
		spin_lock(&_lock);
		retry = _upd_bits | _cmt_bits;
		if (likely(!retry)) {
			_flag &= ~FLG_IO_EXEC;
			_flag |= FLG_IO_COMPLETE;
		}
		spin_unlock(&_lock);

		// 処理中にbitが追加された場合は再実行する。
	} while (retry);
}

void
devgrp::__io_event(struct wq_item *item, wq_arg_t arg)
{
	wq_ev_item_t *ev_item = (wq_ev_item_t *)arg;
	devgrp *_this = (devgrp *)wq_get_evitem_arg(ev_item);
	_this->__submit();
}


