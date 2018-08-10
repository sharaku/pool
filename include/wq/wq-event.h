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

#ifndef WQEV_H_
#define WQEV_H_

// event待ち合わせを行う。
// イベントを処理するために、wq1スレッドを占有する。
// linuxの場合は
//  - pollをイベントを待ち合わせる。
//    poll対象が追加された場合、eventfdを使用してpoll待ちを中断して
//    再度待ちに入る。
// itronの場合は
//  - event flagを使用して待ち合わせる。
#include <wq/wq.h>
#include <sys/epoll.h>
#include <string.h>

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

typedef struct wq_ev_item_linux {
	int 			id;
	uint32_t		events;
	wq_item_t		item;
} wq_ev_item_t;
#define WQ_EVFL_FDIN	(EPOLLIN)
#define WQ_EVFL_FDOUT	(EPOLLOUT)
#define WQ_EVFL_FDERR	(EPOLLERR)

extern int wq_ev_create(wq_ev_item_t *ev_item);

// event itemを初期化する。
// id はイベント識別子
// flg はイベントのフラグ
static inline int
__wq_ev_init(wq_ev_item_t *ev_item, int id)
{
	ev_item->id = id;
	return wq_ev_create(ev_item);
}

static inline int
wq_ev_init(wq_ev_item_t *ev_item, int id)
{
	memset(ev_item, 0, sizeof *ev_item);
	wq_init_item(&(ev_item->item));
	return __wq_ev_init(ev_item, id);
}

static inline int
wq_ev_init_item_prio(wq_ev_item_t *ev_item, int id, int16_t prio)
{
	memset(ev_item, 0, sizeof *ev_item);
	wq_init_item_prio(&(ev_item->item), prio);
	return __wq_ev_init(ev_item, id);
}

extern int wq_ev_destroy(wq_ev_item_t *ev_item);
extern int wq_ev_sched(wq_ev_item_t *ev_item, int flg, wq_stage_t cb);
extern int wq_ev_cancel(wq_ev_item_t *item);

inline void *
wq_get_evitem_arg(wq_ev_item_t *ev_item)
{
	return ev_item->item.arg;
}
inline void
wq_set_evitem_arg(wq_ev_item_t *ev_item, void *arg)
{
	ev_item->item.arg = arg;
}

CPP_SRC(})

#endif /* WQEV_H_ */
