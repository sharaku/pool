/* --
MIT License

Copyright (c) 2004 Abe Takafumi

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

#include <string.h>
#include <errno.h>
#include <timeofday.h>
#include <stdio.h>
#include <wq/wq.h>
#include <log/log.h>

#ifdef WQ_ASSERT_ENABLE
	#define WQ_ASSERT(x) assert((x));
#else
	#define WQ_ASSERT(x)
#endif

#define	WQ_IS_CXFL_INIT(C)	((C)->flags & WQ_CXFL_INIT)
#define	WQ_IS_CXFL_STOP(C)	((C)->flags & WQ_CXFL_STOP)

#define	WQ_SET_CXFL_INIT(C)	((C)->flags |= WQ_CXFL_INIT)
#define	WQ_SET_CXFL_STOP(C)	((C)->flags |= WQ_CXFL_STOP)

typedef struct wq_ctx {
	uint16_t	magic;
	uint8_t		padding;
	uint8_t		flags;
	uint8_t		threads;
	uint8_t		idle_threads;
	uint8_t		threads_min;
	uint8_t		threads_max;
	int64_t		base_time;

	// 優先度付きjobリスト
	// 一般ユーザが使用可能な優先度は 0 ～ 255
	// システムは-99 ～ 0
	// default = 128
	struct plist_head	sched_plist;
	// 時間順のjobリスト
	// 起動時の時間を0として、経過usで登録する。
	struct plist_head	timer_plist;

	seqlock_t	attr_lock;
	seqlock_t	sched_lock;
	seqlock_t	timer_lock;
	mutex_lock_t	task_lock;
	mutex_cond_t	task_cond;
} wq_ctx_t;

static wq_ctx_t __ctx = {
	WQ_CTX_MAGIC,				// magic;
	0,					// padding;
	0,					// flags;
	0,					// threads;
	0,					// idle_threads;
	1,					// threads_min;
	1,					// threads_max;
	0,					// base_time;
	PLIST_HEAD_INIT(__ctx.sched_plist),	// sched_plist;
	PLIST_HEAD_INIT(__ctx.timer_plist),	// timer_plist;
	SEQLOCK_INIT,				// attr_lock;
	SEQLOCK_INIT,				// sched_lock;
	SEQLOCK_INIT,				// timer_lock;
	// task_lock;
	// task_cond;

};

__attribute__((constructor))
static void
__wq_constructor(void)
{
	generic_upd_generictime();
	init_mutex_lock(&__ctx.task_lock);
	init_mutex_cond(&__ctx.task_cond);
	__ctx.base_time = generic_get_usec_fast();
	WQ_SET_CXFL_INIT(&__ctx);
	mb();
	return;
}

// スケジュールで順番が来たitemを処理する。
// 処理対象がなければ1を応答する。
static inline void
__do_sched_timer(void)
{
	wq_ctx_t *ctx = &__ctx;
	wq_item_t *item = NULL;
	wq_item_t *n = NULL;

	struct list_head	list = LIST_HEAD_INIT(list);

	// 初期化が完了していない場合は何もしない。
	// これは、wq_ctx_tがグローバル変数として定義された場合の対策である。
	if (!WQ_IS_CXFL_INIT(ctx)) {
		goto out;
	}

	// 時間経過しているものを list へ取り出す。
	// 排他区間を最低限にするために一度 list へ受けるようにしている。
	write_seqlock(&ctx->timer_lock);
	plist_for_each_entry_safe(item, n, &ctx->timer_plist,
					 wq_item_t, node) {
		if (item->usec > ctx->base_time) {
			break;
		}
		item->stat = WQ_STAT_SUSPEND;
		plist_del(&(item->node), &(ctx->timer_plist));
		list_add_tail(&item->node.node_list, &list);
	}
	write_sequnlock(&ctx->timer_lock);

	// 優先度が異なる場合も想定されるため、 list をそのまま
	// スケジュールできない。
	// ここでは 1 item づつスケジュールする。
	list_for_each_entry_safe(item, n, &list, wq_item_t, node.node_list) {
		list_del_init(&(item->node.node_list));
		wq_sched(item, item->stage, item->arg);
	}

out:
	return;
}

// スケジュールで順番が来たitemを処理する。
// 処理対象がなければ1を応答する。
static inline int
__do_sched(void)
{
	wq_ctx_t *ctx = &__ctx;
	wq_item_t *item;

	write_seqlock(&ctx->sched_lock);
	if (list_empty(&(ctx->sched_plist.node_list))) {
		// 実行すべきjobがない
		WQ_SET_CXFL_INIT(ctx);
		write_sequnlock(&ctx->sched_lock);
		return 1;
	}
	// 先頭を取り出す
	item = list_entry(ctx->sched_plist.node_list.next, wq_item_t, node.node_list);
	plist_del(&(item->node), &(ctx->sched_plist));

	// 実行時は stat を WQ_STAT_RUN へ遷移させる。
	item->stat = WQ_STAT_RUN;
	write_sequnlock(&ctx->sched_lock);

	item->stage(item, item->arg);

	// WQ_STAT_RUN のまま終了したものは停止状態にする
	// callback の中でキューイングしたものは WQ_STAT_RUN 状態
	// となる。
	if (WQ_STAT_RUN == item->stat) {
		item->stat = WQ_STAT_SUSPEND;
	}
	return 0;
}

// workerスレッドのエントリ。
// argはコンテキストアドレス。
static void
__wq_worker(void *arg)
{
	wq_ctx_t *ctx = &__ctx;

	// スレッド数加算
	write_seqlock(&ctx->attr_lock);
	++ctx->threads;
	write_sequnlock(&ctx->attr_lock);

	wq_infolog64("start worker threads=%ld", ctx->threads);

	for (;;) {
		int ret;
		int64_t us;

		// 停止指示の場合は終了する
		if (WQ_IS_CXFL_STOP(ctx)) {
			break;
		}

		// us単位で変化がない場合は何もしない。
		generic_upd_generictime();
		us = generic_get_usec_fast();
		if (us != ctx->base_time) {
			write_seqlock(&ctx->attr_lock);
			ctx->base_time = us;
			write_sequnlock(&ctx->attr_lock);

			__do_sched_timer();
		}

		// コンテキストを実行する。
		ret = __do_sched();
		if (ret) {
			int ms_diff = -1;

			// threadsは厳密でなくてよい。
			// スレッド数が閾値よりも多い場合は、
			// スレッドを終了させる。
			mb();
			if (ctx->threads > ctx->threads_min) {
				break;
			}

			write_seqlock(&ctx->attr_lock);
			++ctx->idle_threads;
			write_sequnlock(&ctx->attr_lock);

			read_seqlock_excl(&ctx->timer_lock);
			if (plist_empty(&ctx->timer_plist)) {
				// デフォルト値はWQ_DEFAULT_TIMEO_MSとする。
				// 10秒後にはタイムアウトするように設定。
				ms_diff = WQ_DEFAULT_TIMEO_MS;
			} else {
				wq_item_t *item
					= plist_first_entry(&ctx->timer_plist,
							    wq_item_t, node);
				ms_diff = item->usec - ctx->base_time;
				if (ms_diff > WQ_DEFAULT_TIMEO_MS) {
					ms_diff = WQ_DEFAULT_TIMEO_MS;
				}
			}
			read_sequnlock_excl(&ctx->timer_lock);

			// スケジュールの必要もなく、かつ woker thread も
			// 減らさないので、寝る。
			// ただし、timer 起動用に time outを指定する。
			// ぎりぎりで寝るとコンテキスト切り替えのコストのほうが
			// 高くつくので1ms以下の場合は寝ない。
			// かつ、寝るときは1ms早めに起きる。
			if (ms_diff > 1) {
				mutex_timedwait(&ctx->task_lock,
						&ctx->task_cond, ms_diff - 1);
			}

			write_seqlock(&ctx->attr_lock);
			--ctx->idle_threads;
			write_sequnlock(&ctx->attr_lock);
		}
	}

	write_seqlock(&ctx->attr_lock);
	--ctx->threads;
	write_sequnlock(&ctx->attr_lock);

	wq_infolog64("stop worker threads=%ld", ctx->threads);
}

static inline int
__push_sched(wq_item_t *item)
{
	wq_ctx_t *ctx = &__ctx;
	int seq;
	int ret = 0;

	write_seqlock(&ctx->sched_lock);
	// 2重登録の場合はエラーにする。
	if (!list_empty((struct list_head*)&(item->node))) {
		write_sequnlock(&ctx->sched_lock);
		ret = EBUSY;
		wq_infolog64("EBUSY. item=%p", item);
		goto end;
	}

	item->node.prio = item->prio;

	// キューに積む
	plist_add(&(item->node), &(ctx->sched_plist));
	if (!WQ_IS_CXFL_INIT(ctx)) {
		write_sequnlock(&ctx->sched_lock);
		goto end;
	}
	write_sequnlock(&ctx->sched_lock);

	// idleのスレッドがあれば起動する
	do {
		seq = read_seqbegin(&ctx->attr_lock);
		if (ctx->idle_threads) {
			mutex_signal(&ctx->task_lock, &ctx->task_cond);
			goto end;
		}
	} while(read_seqretry(&ctx->attr_lock, seq));

end:
	return ret;
}

static inline int
__push_timer(wq_item_t *item)
{
	wq_ctx_t *ctx = &__ctx;
	int seq;
	int ret = 0;

	write_seqlock(&ctx->timer_lock);
	// 2重登録の場合はエラーにする。
	if (!list_empty((struct list_head*)&(item->node))) {
		write_sequnlock(&ctx->timer_lock);
		ret = EBUSY;
		wq_infolog64("EBUSY. item=%p", item);
		goto end;
	}

	item->node.prio = item->usec;

	// キューに積む
	plist_add(&(item->node), &(ctx->timer_plist));
	if (!WQ_IS_CXFL_INIT(ctx)) {
		write_sequnlock(&ctx->timer_lock);
		goto end;
	}
	write_sequnlock(&ctx->timer_lock);

	// idleのスレッドがあれば起動する
	do {
		seq = read_seqbegin(&ctx->attr_lock);
		if (ctx->idle_threads) {
			mutex_signal(&ctx->task_lock, &ctx->task_cond);
			goto end;
		}
	} while(read_seqretry(&ctx->attr_lock, seq));

end:
	return ret;
}

void
wq_run(void)
{
	// runが指定された場合は、自スレッドがworkerスレッドになる。
	__wq_worker(NULL);
}

int
wq_sched(wq_item_t *item, wq_stage_t cb, wq_arg_t arg)
{
	// Jobの初期化
	item->stat	= WQ_STAT_READY;
	item->stage 	= cb;
	item->arg 	= arg;
	item->usec	= 0;

	// スケジューラの末尾へ登録後、待ち合わせているスケジューラを起動する
	// singleスケジューラの場合はこの起動に意味はなく、処理がスケジューラへ
	// 戻された段階で次を処理する。
	// 多重スケジュールの場合はEBUSYを応答する。
	return __push_sched(item);
}

int
wq_timer_sched(wq_item_t *item, wq_usec_t us, wq_stage_t cb, wq_arg_t *arg)
{
	wq_ctx_t	*ctx = &__ctx;

	// Jobの初期化
	item->stat	= WQ_STAT_WAIT;
	item->stage 	= cb;
	item->arg 	= arg;
	item->usec	= ctx->base_time + us;

	// スケジューラの末尾へ登録後、待ち合わせているスケジューラを起動する
	// singleスケジューラの場合はこの起動に意味はなく、処理がスケジューラへ
	// 戻された段階で次を処理する
	// 多重スケジュールの場合はEBUSYを応答する。
	return __push_timer(item);
}
