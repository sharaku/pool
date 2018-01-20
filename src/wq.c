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

#include "wq.h"
#include <string.h>
#include <errno.h>

#define	WQ_TV2MSEC(TV)	(TV.tv_sec * 1000 + TV.tv_usec / 1000)

#ifdef WQ_ASSERT_ENABLE
	#define WQ_ASSERT(x) assert((x));
#else
	#define WQ_ASSERT(x)
#endif

#define	WQ_STAT_SUSPEND	(0)
#define	WQ_STAT_READY	(1)
#define	WQ_STAT_RUN	(2)
#define	WQ_STAT_WAIT	(3)

#define	WQ_CXFL_INIT	(1 << 0)
#define	WQ_CXFL_STOP	(1 << 1)

#define	WQ_IS_CXFL_INIT(C)	((C)->flags & WQ_CXFL_INIT)
#define	WQ_IS_CXFL_STOP(C)	((C)->flags & WQ_CXFL_STOP)

#define	WQ_SET_CXFL_INIT(C)	((C)->flags |= WQ_CXFL_INIT)
#define	WQ_SET_CXFL_STOP(C)	((C)->flags |= WQ_CXFL_STOP)

#define	WQ_ITMFL_TIMEO		(1 << 0)
#define	WQ_DEFAULT_TIMEO_MS	(10000)
#define	WQ_CTX_MAGIC	('W' << 8 | 'Q')


static void _wq_worker(void *arg);


// スケジュールで順番が来たitemを処理する。
// 処理対象がなければ1を応答する。
static inline void
_do_sched_timer(wq_ctx_t *ctx)
{
	wq_item_t	*item = NULL;
	wq_item_t	*n = NULL;

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
		if (item->milli_sec > ctx->base_time) {
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
_do_sched(wq_ctx_t *ctx)
{
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
_wq_worker(void *arg)
{
	wq_ctx_t *ctx = arg;
	struct timeval tv;

	// スレッド数加算
	write_seqlock(&ctx->attr_lock);
	++ctx->threads;
	write_sequnlock(&ctx->attr_lock);

	for (;;) {
		int ret;
		int64_t ms;

		// 停止指示の場合は終了する
		if (WQ_IS_CXFL_STOP(ctx)) {
			break;
		}

		// ms単位で変化がない場合は何もしない。
		gettimeofday(&tv, NULL);
		ms = WQ_TV2MSEC(tv);
		if (ms != ctx->base_time) {
			write_seqlock(&ctx->attr_lock);
			ctx->base_time = ms;
			write_sequnlock(&ctx->attr_lock);

			_do_sched_timer(ctx);
		}

		// コンテキストを実行する。
		ret = _do_sched(ctx);
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
				plist_first_entry(&ctx->timer_plist,
						  wq_item_t, node);
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
}

static inline void
_push_sched(wq_item_t *item, wq_ctx_t *ctx)
{
	int seq;

	// 初期化前の場合はミューテックスをかけられないため、
	// 排他せずに登録する。
	item->node.prio = item->prio;

	if (!WQ_IS_CXFL_INIT(ctx)) {
		plist_add(&(item->node), &(ctx->sched_plist));
		goto end;
	}

	// キューに積む
	write_seqlock(&ctx->sched_lock);
	plist_add(&(item->node), &(ctx->sched_plist));
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
	return;
}

static inline void
_push_timer(wq_item_t *item, wq_ctx_t *ctx)
{
	int seq;

	// 初期化前の場合はミューテックスをかけられないため、排他せずに
	// 登録する。
	item->node.prio = item->milli_sec;

	if (!WQ_IS_CXFL_INIT(ctx)) {
		plist_add(&(item->node), &(ctx->timer_plist));
		goto end;
	}

	// キューに積む
	write_seqlock(&ctx->timer_lock);
	plist_add(&(item->node), &(ctx->timer_plist));
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
	return;
}

int
wq_init(wq_ctx_t *ctx, wq_ctx_attr_t *attr)
{
	memset(ctx, 0, sizeof *ctx);
	ctx->magic		= WQ_CTX_MAGIC;

	if (!attr) {
		ctx->threads_min = 1;
		ctx->threads_max = 1;
	} else {
		if (!attr->threads_min) {
			ctx->threads_min = attr->threads_min;
		} else {
			ctx->threads_min = 1;
		}
		if (!attr->threads_max) {
			ctx->threads_max = attr->threads_max;
		} else {
			ctx->threads_max = 1;
		}
	}
	if (ctx->threads_min > ctx->threads_max) {
		return EINVAL;
	}

	init_plist_head(&ctx->sched_plist);
	init_plist_head(&ctx->timer_plist);

	init_seqlock(&ctx->attr_lock);
	init_seqlock(&ctx->sched_lock);
	init_seqlock(&ctx->timer_lock);
	init_mutex_lock(&ctx->task_lock);
	init_mutex_cond(&ctx->task_cond);
	mb();
	WQ_SET_CXFL_INIT(ctx);
	return 0;
}

void
wq_run(wq_ctx_t *ctx)
{
	// runが指定された場合は、自スレッドがworkerスレッドになる。
	_wq_worker(ctx);
}


void
wq_init_item(wq_item_t *item, wq_ctx_t *ctx)
{
	memset(item, 0, sizeof *item);
	item->stat = WQ_STAT_SUSPEND;
	init_plist_node(&item->node, 0);
	item->context = ctx;
	item->prio = WQ_DEFAULT_PRIO;
	item->magic = WQ_CTX_MAGIC;
}

void
wq_sched(wq_item_t *item, wq_stage_t cb, wq_arg_t arg)
{
	// Jobの初期化
	item->stat	= WQ_STAT_READY;
	item->stage 	= cb;
	item->arg 	= arg;
	item->milli_sec	= 0;

	// スケジューラの末尾へ登録後、待ち合わせているスケジューラを起動する
	// singleスケジューラの場合はこの起動に意味はなく、処理がスケジューラへ
	// 戻された段階で次を処理する
	_push_sched(item, item->context);
}

void
wq_timer_sched(wq_item_t *item, wq_msec_t ms, wq_stage_t cb, wq_arg_t *arg)
{
	// Jobの初期化
	item->stat	= WQ_STAT_WAIT;
	item->stage 	= cb;
	item->arg 	= arg;
	mb();
	item->milli_sec	= item->context->base_time + ms;

	// スケジューラの末尾へ登録後、待ち合わせているスケジューラを起動する
	// singleスケジューラの場合はこの起動に意味はなく、処理がスケジューラへ
	// 戻された段階で次を処理する
	_push_timer(item, item->context);
}
