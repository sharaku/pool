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
#include <poll.h>
#include <errno.h>
#include <wq/wq-event.h>
#include <log/log.h>

// コンテキスト
static struct wq_ev_ctx_linux {
	wq_item_t		item;
	wq_item_t		item_timer;
	int			epfd;		// epoll用のfd
	spinlock_t		lock;
	spinlock_t		epoll_lock;
	int			cnt;		// 登録数
} __ev_ctx;
#define WQ_EV_DEFAULT_PRIO (255)

__attribute__((constructor))
static void
__wq_ev_constructor(void)
{
	__ev_ctx.epfd = epoll_create(32);

	wq_init_item_prio(&(__ev_ctx.item), WQ_EV_DEFAULT_PRIO);
	wq_init_item_prio(&(__ev_ctx.item_timer), 0);
	init_spinlock(&(__ev_ctx.lock));
	init_spinlock(&(__ev_ctx.epoll_lock));
	__ev_ctx.cnt = 0;
}

// epollを使用してイベント有無をチェックする。
// タイミングは、IDLE時と1msごとの定期処理。
#define WQ_EV_POLL_MAX	32
static void
__wq_ev_poll_generic_linux(void)
{
	int i;
	int nfds;
	struct epoll_event ev_ret[WQ_EV_POLL_MAX];

	if (__ev_ctx.cnt) {
		// epoll中は登録禁止
		spin_lock(&(__ev_ctx.epoll_lock));
		nfds = epoll_wait(__ev_ctx.epfd,
				  ev_ret, WQ_EV_POLL_MAX, 0);
		for (i = 0; i < nfds; i++) {
			wq_ev_item_t *ev_item = NULL;
			ev_item = (wq_ev_item_t *)ev_ret[i].data.ptr;
			ev_item->events = ev_ret[i].events;

			ev_item->req_events = 0;
			epoll_ctl(__ev_ctx.epfd,
				       EPOLL_CTL_DEL, ev_item->id, NULL);

			// EPOLLONESHOT指定のため、ここに来る場合は、
			// 破棄された状態。
			spin_lock(&(__ev_ctx.lock));
			__ev_ctx.cnt--;
			spin_unlock(&(__ev_ctx.lock));

			wq_sched(&(ev_item->item),
				 ev_item->item.stage,
				 ev_item);
		}
		spin_unlock(&(__ev_ctx.epoll_lock));
	}
}

static void
__wq_ev_poll_linux(struct wq_item *item, wq_arg_t argv)
{
	__wq_ev_poll_generic_linux();

	// イベント登録数が0以上なら自分をスケジュールする
	// 0の場合は、イベントが登録されたタイミングでスケジュールする。
	spin_lock(&(__ev_ctx.lock));
	if (__ev_ctx.cnt) {
		wq_sched(item, __wq_ev_poll_linux, argv);
	}
	spin_unlock(&(__ev_ctx.lock));
}

static void
__wq_ev_polltimer_linux(struct wq_item *item, wq_arg_t argv)
{
	__wq_ev_poll_generic_linux();

	// イベントがある場合は1msで起動する。
	// 必ず1msごとにイベント発火できるようにタイマーを行う。
	spin_lock(&(__ev_ctx.lock));
	if (__ev_ctx.cnt) {
		wq_timer_sched(item, WQ_TIME_US(1000), __wq_ev_polltimer_linux, argv);
	}
	spin_unlock(&(__ev_ctx.lock));
}

// イベントを生成する
int
wq_ev_create(wq_ev_item_t *ev_item)
{
	return 0;
}

// イベントを破棄する。
int
wq_ev_destroy(wq_ev_item_t *ev_item)
{
	spin_lock(&(__ev_ctx.epoll_lock));
	epoll_ctl(__ev_ctx.epfd, EPOLL_CTL_DEL, ev_item->id, NULL);
	spin_unlock(&(__ev_ctx.epoll_lock));
	return 0;
}

int
wq_ev_sched(wq_ev_item_t *ev_item, int flg, wq_stage_t cb)
{
	int rc;
	struct epoll_event ev;

	spin_lock(&(__ev_ctx.epoll_lock));
	ev_item->item.stage = cb;
	if (ev_item->req_events != flg) {
		epoll_ctl(__ev_ctx.epfd,
			       EPOLL_CTL_DEL, ev_item->id, NULL);
	}
	ev_item->events = 0;
	ev_item->req_events =flg;
	ev_item->req_events |= EPOLLONESHOT;

	ev.events = ev_item->req_events;
	ev.data.fd = ev_item->id;
	ev.data.ptr = (void*)ev_item;

	rc = epoll_ctl(__ev_ctx.epfd, EPOLL_CTL_ADD,
		       ev_item->id, &ev);
	if (rc != 0) {
		spin_unlock(&(__ev_ctx.epoll_lock));
		wq_infolog64("epoll_ctl ADD error. errno=%d", errno);
		return -errno;
	}

	// 新規にepoll登録する
	// もし、イベント数が0から1になった場合はIDLE時のイベントと1msごとの
	// イベントを起動する。
	rc = epoll_ctl(__ev_ctx.epfd, EPOLL_CTL_MOD,
		       ev_item->id, &ev);
	if (rc != 0) {
		spin_unlock(&(__ev_ctx.epoll_lock));
		wq_infolog64("epoll_ctl ADD error. errno=%d", errno);
		return -errno;
	}

	spin_lock(&(__ev_ctx.lock));
	if (list_empty((struct list_head*)&(__ev_ctx.item.node))) {
		wq_sched(&(__ev_ctx.item),
			 __wq_ev_poll_linux, NULL);
	}
	if (list_empty((struct list_head*)&(__ev_ctx.item_timer.node))) {
		wq_timer_sched(&(__ev_ctx.item_timer), WQ_TIME_US(1000),
			       __wq_ev_polltimer_linux, NULL);
	}
	__ev_ctx.cnt++;
	spin_unlock(&(__ev_ctx.lock));
	spin_unlock(&(__ev_ctx.epoll_lock));
	return 0;
}

int
wq_ev_cancel(wq_ev_item_t *ev_item)
{
	int rc;

	// キャンセル時は、epoll対象から外し、登録数を減らす。
	spin_lock(&(__ev_ctx.epoll_lock));
	rc = epoll_ctl(__ev_ctx.epfd,
		       EPOLL_CTL_DEL, ev_item->id, NULL);
	if (rc != 0) {
		spin_unlock(&(__ev_ctx.epoll_lock));
		return -errno;
	}

	spin_lock(&(__ev_ctx.lock));
	__ev_ctx.cnt--;
	spin_unlock(&(__ev_ctx.lock));
	spin_unlock(&(__ev_ctx.epoll_lock));
	return 0;
}



