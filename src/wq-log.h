/* --
 *
 * MIT License
 * 
 * Copyright (c) 2014-2018 Abe Takafumi
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

#ifndef _WQ_LOG_H
#define _WQ_LOG_H

// info logの変換方法
//   wq_log::formatは
//     1. objdump -h {オブジェクト名} で.rodataの先頭アドレスとファイル
//        のオフセットを取得。
//     2. wq_log::formatアドレスから.rodataの先頭アドレスを引いた値を元に
//        オブジェクトファイルを読んで文字列を取得する。
//   wq_log::funcは
//     1. objdump -t {オブジェクト名} でシンボルを出して、関数アドレスと
//        マッチングする。
//   で文字列に変換できる。
// 


#include <helper.h>

CPP_SRC(extern "C" {)

enum {
	WQ_LOGTYPE_TRACE,
	WQ_LOGTYPE_INFO64_32,
	WQ_LOGTYPE_INFO64_64,
};

struct wq_log_type_header
{
	uint32_t	log_sz;
	uint32_t	head_idx;
	uint32_t	tail_idx;
};

struct wq_trace_type_info
{
	struct wq_log_type_header	*header;
	char				*log_top;
};

struct wq_log_header
{
	uint32_t			version;	// ログバージョン
	uint32_t			padding4;	// padding
	int64_t				base_usec;	// ログのベース時間
	struct wq_log_type_header	sys_info;
};
#define WQ_LOGHD_SIZE		128
#define WQ_SYSINFO_LOG_BASE_SZ	16
#define WQ_SYSINFO_LOG_SIZE	(256 * 1024)

// 16byte
struct wq_trace {
	const char	*func;
	uint32_t	usec;
	uint16_t	line;
	uint8_t		type;
	uint8_t		u8;
};


// 64byte
struct wq_log64_64
{
	struct wq_trace	header;
	const char	*format;
	int64_t		arg[5];
};

// 32byte
struct wq_log64_32
{
	struct wq_trace	header;
	const char	*format;
	int64_t		arg[1];
};

union __wq_log
{
	struct wq_trace		trace;
	struct wq_log64_32	log64_32;
	struct wq_log64_64	log64_64;
};
#define WQ_LOG_MAXBLKS (sizeof(union __wq_log) / WQ_SYSINFO_LOG_BASE_SZ)
#define WQ_LOG_BLKS(type) (sizeof(type) / WQ_SYSINFO_LOG_BASE_SZ)

// ログ採取
extern void __wq_infolog_internal_wq_log64_32(const char *fmt, const char *func, uint16_t line, int64_t arg0);
extern void __wq_infolog_internal_wq_log64_64(const char *fmt, const char *func, uint16_t line, int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4);
#define ____infolog64_0( func, line, fmt) \
		__wq_infolog_internal_wq_log64_32(fmt, func, line, 0)
#define ____infolog64_1(func, line, fmt, a1) \
		__wq_infolog_internal_wq_log64_32(fmt, func, line, a1)
#define ____infolog64_2(func, line, fmt, a1, a2) \
		__wq_infolog_internal_wq_log64_64(fmt, func, line, a1, a2, 0, 0, 0)
#define ____infolog64_3(func, line, fmt, a1, a2, a3) \
		__wq_infolog_internal_wq_log64_64(fmt, func, line, a1, a2, a3, 0, 0)
#define ____infolog64_4(func, line, fmt, a1, a2, a3, a4) \
		__wq_infolog_internal_wq_log64_64(fmt, func, line, a1, a2, a3, a4, 0)
#define ____infolog64_5(func, line, fmt, a1, a2, a3, a4, a5) \
		__wq_infolog_internal_wq_log64_64(fmt, func, line, a1, a2, a3, a4, a5)
#define ____infolog64(...)		\
	__GET_FUNC6_NAME(__VA_ARGS__, ____infolog64_5, ____infolog64_4, ____infolog64_3,	\
			  ____infolog64_2, ____infolog64_1, ____infolog64_0)			\
			  (__func__, __LINE__, ##__VA_ARGS__)

#define wq_infolog64(fmt, ...) ____infolog64(fmt, ##__VA_ARGS__)

CPP_SRC(})

#endif /* _WQ_LOG_H */

