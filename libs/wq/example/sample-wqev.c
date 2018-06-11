/* --

MIT License

Copyright (c) 2017 Abe Takafumi

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

 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <wq/wq.h>
#include <wq/wq-event.h>
#include <pthread.h>
#include <sys/eventfd.h>

static void
timer_sched_cb(wq_item_t *item, wq_arg_t arg)
{
	uint64_t u = 1;
	wq_ev_item_t	*item_ev = (wq_ev_item_t*)arg;
	int efd = item_ev->id;

	write(efd, &u, sizeof(uint64_t));
	printf("write !!\n");
	wq_timer_sched(item, 1000, timer_sched_cb, arg);
}

static void
event_sched_cb(wq_item_t *item, wq_arg_t arg)
{
	uint64_t u;
	wq_ev_item_t	*item_ev = (wq_ev_item_t*)arg;
	int efd = item_ev->id;

	// イベントを受け取ったら出力して、もう一度イベント待ちにする。
	read(efd, &u, sizeof(uint64_t));
	printf("event !!\n");
	wq_ev_sched(item_ev, event_sched_cb, (void*)item_ev);
}

int
main(void)
{
	wq_ev_item_t	item_ev;
	wq_item_t	item_timer;
	int efd;

	// イベントFDを用意
	efd = eventfd(0, 0);

	wq_init_item_prio(&item_timer, 0);
	wq_timer_sched(&item_timer, 1000, timer_sched_cb, (void*)&item_ev);

	wq_ev_init(&item_ev, efd, WQ_EVFL_FDIN);
	wq_ev_sched(&item_ev, event_sched_cb, (void*)&item_ev);

	wq_run();

	return 0;
}

