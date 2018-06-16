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

struct wq_item;
typedef void *wq_arg_t;
typedef uint32_t wq_usec_t;
typedef void(*wq_stage_t)(struct wq_item*, wq_arg_t);

#define WQ_DEFAULT_PRIO (128)

typedef struct wq_ctx_attr {
	uint8_t			threads_min;
	uint8_t			threads_max;
} wq_ctx_attr_t;

typedef struct wq_item {
	struct plist_node	node;		///< スケジューラのノード
	wq_stage_t		stage;
	wq_arg_t		arg;
	int64_t			usec;		///< タイムアウト値
	int16_t			prio;		///< 優先度
	uint8_t			is_timeout:1;	///< タイムアウトしているか
	uint8_t			rsv_flg:7;	///< 
	uint8_t			stat;		///< 
	uint32_t		magic;		// マジックコード
} wq_item_t;

#define	WQ_STAT_SUSPEND	(0)
#define	WQ_STAT_READY	(1)
#define	WQ_STAT_RUN	(2)
#define	WQ_STAT_WAIT	(3)

#define	WQ_CXFL_INIT	(1 << 0)
#define	WQ_CXFL_STOP	(1 << 1)

#define	WQ_ITMFL_TIMEO		(1 << 0)
#define	WQ_DEFAULT_TIMEO_MS	(10000)

#define	WQ_CTX_MAGIC	('W' << 8 | 'Q')

// コンテキスト制御
//  wq_ctx_t は wq_init() を使用して初期化を行う。
//  wq_start() を使用することで別スレッドとしてコンテキストを実行する。
//  wq_destroy() を使用することでコンテキストを停止する。
//  wq_run() を使用することでコンテキストを実行する。

//extern void wq_start(wq_ctx_t *);
//extern void wq_destroy(wq_ctx_t *);
//extern void wq_exit(wq_ctx_t *);
extern void wq_run(void);
//extern void wq_is_idle(wq_ctx_t *);
//extern void wq_wait(wq_ctx_t *);

static void
wq_init_item(wq_item_t *item)
{
	memset(item, 0, sizeof *item);
	item->stat = WQ_STAT_SUSPEND;
	init_plist_node(&item->node, 0);
	item->prio = WQ_DEFAULT_PRIO;
	item->magic = WQ_CTX_MAGIC;
}

static inline void
wq_init_item_prio(wq_item_t *item, int16_t prio)
{
	wq_init_item(item);
	item->prio = prio;
}
//extern void wq_set_prio(wq_item_t *item, int16_t prio);
extern int wq_sched(wq_item_t *item, wq_stage_t cb, wq_arg_t arg);
extern int wq_timer_sched(wq_item_t *item, wq_usec_t us, wq_stage_t cb, wq_arg_t *arg);
//extern void wq_cancel(wq_item_t *);

#define WQ_TIME_MS(ms)	((ms) * 1000)
#define WQ_TIME_US(us)	(us)

CPP_SRC(})

#endif /* WQ_H_ */

