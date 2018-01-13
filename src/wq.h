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


//
//  +---------+                  wq_async_sched                 +-------+
//  |  READY  | <---------------------------------------------- |  RUN  |
//  |         | ----------------------------------------------> |       |
//  +---------+                   スケジュール                  +-------+
//   ｜↑ ↑                                                      ｜｜
//   ｜｜ ｜                                                      ｜｜
//   ｜｜ ｜                                                      ｜｜
//   ｜｜ ｜                                                      ｜｜
//   ｜｜ ｜    時間経過        +---------+    wq_timer_sched     ｜｜
//   ｜｜  -------------------- |  WAIT   | <---------------------- ｜
//   ｜｜                       |         |                         ｜
//   ｜｜                       +---------+                         ｜
//   ｜｜                          ｜↑                             ｜
//   ｜｜                 wq_cancel｜｜wq_timer_sched               ｜
//   ｜｜                          ↓｜                             ｜
//   ｜｜    wq_async_sched     +---------+                         ｜
//   ｜ ----------------------- | SUSPEND | <------------------------
//    ------------------------> |         |   job関数終了後、スケジュールなし
//              wq_cancel       +---------+
//                              ↑ wq_init_item
//
//
//       +---------+    +---------+    +---------+    +---------+
//       |   CPU   |    |   CPU   |    |   CPU   |    |   CPU   |
//       +---------+    +---------+    +---------+    +---------+
//           ｜             ｜             ｜             ｜
//           ｜ 実行        ｜ 実行        ｜ 実行        ｜ 実行
//           ↓             ↓             ↓             ↓
//       +---------+    +---------+    +---------+    +---------+
//       |workqueue|    |workqueue|    |workqueue|    |workqueue|
//       +---------+    +---------+    +---------+    +---------+
//          ｜↑           ｜↑           ｜↑           ｜↑
//          ｜｜           ｜｜           ｜｜           ｜｜
//          ｜｜           ｜｜           ｜｜           ｜｜
//          ｜ ---------------------------------------------
//          ｜             ｜        ｜   ｜             ｜
//           ------------------------｜-------------------
//                                 ｜｜
//                                 ↓｜
//                     +------------------------+
//                     |スケジューラコンテキスト|
//                     | - job list             |
//                     | - timer list           |
//                     +------------------------+


#ifndef WQ_H_
#define WQ_H_

#include <list.h>
#include <plist.h>
#include <seqlock.h>
#include <threads.h>
#include <atomic.h>

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

struct wq_item;
typedef void *wq_arg_t;
typedef uint32_t wq_msec_t;
typedef void(*wq_stage_t)(struct wq_item*, wq_arg_t);

#define WQ_DEFAULT_PRIO (128)

typedef struct wq_ctx_attr {
	uint8_t			threads_min;
	uint8_t			threads_max;
} wq_ctx_attr_t;

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
	// 起動時の時間を0として、経過msで登録する。
	struct plist_head	timer_plist;

	seqlock_t	attr_lock;
	seqlock_t	sched_lock;
	seqlock_t	timer_lock;
	mutex_lock_t	task_lock;
	mutex_cond_t	task_cond;
} wq_ctx_t;

typedef struct wq_item {
	struct plist_node	node;		///< スケジューラのノード
	wq_stage_t		stage;
	wq_arg_t		arg;
	int64_t			milli_sec;	///< タイムアウト値
	wq_ctx_t*		context;	///< スケジューラコンテキスト
	int16_t			prio;		///< 優先度
	uint8_t			is_timeout:1;	///< タイムアウトしているか
	uint8_t			rsv_flg:7;	///< 
	uint8_t			stat;		///< 
	uint32_t		magic;		// マジックコード
} wq_item_t;

// コンテキスト制御
//  wq_ctx_t は wq_init() を使用して初期化を行う。
//  wq_start() を使用することで別スレッドとしてコンテキストを実行する。
//  wq_destroy() を使用することでコンテキストを停止する。
//  wq_run() を使用することでコンテキストを実行する。
extern int wq_init(wq_ctx_t *ctx, wq_ctx_attr_t *attr);
static inline void
wq_attr_init(wq_ctx_attr_t *attr)
{
	attr->threads_min = 1;
	attr->threads_max = 1;
}
static inline void
wq_attr_setthreads(wq_ctx_attr_t *attr, uint8_t min, uint8_t max)
{
	attr->threads_min = min;
	attr->threads_max = max;
}
//extern void wq_start(wq_ctx_t *);
//extern void wq_destroy(wq_ctx_t *);
//extern void wq_exit(wq_ctx_t *);
extern void wq_run(wq_ctx_t *ctx);
//extern void wq_is_idle(wq_ctx_t *);
//extern void wq_wait(wq_ctx_t *);

extern void wq_init_item(wq_item_t *item, wq_ctx_t *ctx);
static inline void
wq_init_item_prio(wq_item_t *item, wq_ctx_t *ctx, int16_t prio)
{
	wq_init_item(item, ctx);
	item->prio = prio;
}
//extern void wq_set_prio(wq_item_t *item, int16_t prio);
extern void wq_sched(wq_item_t *item, wq_stage_t cb, wq_arg_t arg);
//extern void wq_timer_sched(wq_item_t *, wq_msec_t, wq_stage_t, wq_arg_t *);
//extern void wq_cancel(wq_item_t *);

CPP_SRC(})

#endif /* WQ_H_ */

