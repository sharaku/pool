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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <timeofday.h>
#include "wq-log.h"

static struct wq_log_header *__wq_log;
static struct wq_trace_type_info __wq_log_sys_info;

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static int __mmap_fd;
#define WQ_LOG_FIME "./log.bin"
#endif

__attribute__((constructor))
static void
_wq_log_init(void)
{
	// ログ領域の獲得
	int size = WQ_LOGHD_SIZE + WQ_SYSINFO_LOG_SIZE;
#ifdef __linux__
	__mmap_fd = open(WQ_LOG_FIME, O_CREAT|O_RDWR, 0644);
	ftruncate(__mmap_fd, size);
	__wq_log = (struct wq_log_header*)mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, __mmap_fd, 0);
#else
	__wq_log = malloc(size);
#endif
	memset(__wq_log, 0, size);

	__wq_log->version = 0;
	__wq_log->base_usec = generic_get_usec();

	// system info log領域の初期化
	// idxは32byte単位となる。
	__wq_log->sys_info.log_sz = WQ_SYSINFO_LOG_SIZE;
	__wq_log->sys_info.head_idx = 0;
	__wq_log->sys_info.tail_idx = 0;
	__wq_log_sys_info.header = &(__wq_log->sys_info);
	__wq_log_sys_info.log_top = ((char *)__wq_log) + WQ_LOGHD_SIZE;
}

static inline void
___wq_trace_internal(struct wq_trace *trc, const char *func,
		    uint16_t line, int8_t type, int8_t u8)
{
	trc->func = func;
	trc->usec = (int32_t)(generic_get_usec_fast() - __wq_log->base_usec);
	trc->line = line;
	trc->type = type;
	trc->u8 = u8;
}

static inline void
__wq_infolog_internal_idxadd(int idx)
{
	// 一番大きなログが入らなければローテーションする。
	if (__wq_log_sys_info.header->head_idx > (WQ_SYSINFO_LOG_SIZE / WQ_SYSINFO_LOG_BASE_SZ) - WQ_LOG_MAXBLKS) {
		__wq_log_sys_info.header->head_idx = 0;
	} else {
		__wq_log_sys_info.header->head_idx += idx;
	}
	if (__wq_log_sys_info.header->head_idx > __wq_log->sys_info.tail_idx) {
		__wq_log->sys_info.tail_idx = __wq_log_sys_info.header->head_idx;
	}
}

void
__wq_trace_internal(const char *func, uint16_t line, int8_t arg0)
{
	uint32_t idx = 0;
	struct wq_trace *info = NULL;

	idx = __wq_log_sys_info.header->head_idx;
	__wq_infolog_internal_idxadd(WQ_LOG_BLKS(struct wq_trace));
	info = (struct wq_trace*)(__wq_log_sys_info.log_top + idx * WQ_SYSINFO_LOG_BASE_SZ);

	___wq_trace_internal(info, func, line, WQ_LOGTYPE_TRACE, arg0);
}

void
__wq_infolog_internal_wq_log64_32(const char *fmt, const char *func,
				  uint16_t line, int64_t arg0)
{
	uint32_t idx = 0;
	struct wq_log64_32 *info = NULL;

	idx = __wq_log_sys_info.header->head_idx;
	__wq_infolog_internal_idxadd(WQ_LOG_BLKS(struct wq_log64_32));
	info = (struct wq_log64_32*)(__wq_log_sys_info.log_top + idx * WQ_SYSINFO_LOG_BASE_SZ);

	___wq_trace_internal(&(info->header), func, line, WQ_LOGTYPE_INFO64_32, 0);
	info->format = fmt;
	info->arg[0] = arg0;
}

void
__wq_infolog_internal_wq_log64_64(const char *fmt, const char *func, uint16_t line,
				  int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4)
{
	uint32_t idx = 0;
	struct wq_log64_64 *info = NULL;

	idx = __wq_log_sys_info.header->head_idx;
	__wq_infolog_internal_idxadd(WQ_LOG_BLKS(struct wq_log64_64));
	info = (struct wq_log64_64*)(__wq_log_sys_info.log_top + idx * WQ_SYSINFO_LOG_BASE_SZ);

	___wq_trace_internal(&(info->header), func, line, WQ_LOGTYPE_INFO64_64, 0);
	info->format = fmt;
	info->arg[0] = arg0;
	info->arg[1] = arg1;
	info->arg[2] = arg2;
	info->arg[3] = arg3;
	info->arg[4] = arg4;
}

