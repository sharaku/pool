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
#include <sys/time.h>
#include <wq.h>
#include <pthread.h>

uint32_t count = 0;

uint32_t old_us = 0;
wq_ctx_t	ctx;


void *output(void *arg)
{
	while(1);
	wq_run(&ctx);
}

static void
timer_sched_cb(wq_item_t *item, wq_arg_t arg)
{
	struct timeval tv;
	uint32_t us;

	wq_timer_sched(item, 100, timer_sched_cb, NULL);

	gettimeofday(&tv, NULL);
	us = tv.tv_sec * 1000000 + tv.tv_usec;
	printf("<%-10u, %-10u>: %-10u\n", us, us - old_us, count);
	old_us = us;
}

static void
sched_cb(wq_item_t *item, wq_arg_t arg)
{
	count ++;
	wq_sched(item, sched_cb, NULL);
}

int
main(void)
{
	wq_ctx_attr_t	attr;
	wq_item_t	item;
	wq_item_t	item_timer;
pthread_t thread_id = 0; 
	printf("<now us    ,interval us>: count\n");

	// 以下の属性を追加
	// - worker threadはMin=1, Max=2
	wq_attr_init(&attr);
	wq_attr_setthreads(&attr, 1, 2);

	wq_init(&ctx, &attr);

	// 2つのタスクをスケジュールする。
	wq_init_item(&item, &ctx);
	wq_init_item_prio(&item_timer, &ctx, 0);
	wq_sched(&item, sched_cb, NULL);
	wq_sched(&item_timer, timer_sched_cb, NULL);

	if (pthread_create(&thread_id, NULL, output, NULL) < 0) {
		perror("pthread_create error");
		exit(1);
	} 

	// 自信をworkerスレッドとする。
	wq_run(&ctx);

	return 0;
}

