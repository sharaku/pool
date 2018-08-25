/* --
 *
 * MIT License
 * 
 * Copyright (c) 2018 Abe Takafumi
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <log/log.h>

static int __log_mmap_fd;
static int __bin_mmap_fd;
static struct wq_log_header *__wq_log;
static char *__bin;
static int64_t __bin_offset_base = 0;
static int64_t __addr_base = 0;

static void
__init(char *log_filename, char *bin_filename)
{
	struct stat stat_buf;

	__log_mmap_fd = open(log_filename, O_RDONLY);
	if (__log_mmap_fd < 0) {
		printf("open error. errno=%d", errno);
		exit(1);
	}
	fstat(__log_mmap_fd, &stat_buf);
	__wq_log = (struct wq_log_header*)mmap(NULL, stat_buf.st_size, PROT_READ, MAP_SHARED, __log_mmap_fd, 0);
	if (__wq_log == NULL) {
		printf("mmap error. errno=%d", errno);
		exit(1);
	}

	__bin_mmap_fd = open(bin_filename, O_RDONLY);
	if (__bin_mmap_fd < 0) {
		printf("open error. errno=%d", errno);
		exit(1);
	}
	fstat(__bin_mmap_fd, &stat_buf);
	
	__bin = (char*)mmap(NULL, stat_buf.st_size, PROT_READ, MAP_SHARED, __bin_mmap_fd, 0);
	if (__bin == NULL) {
		printf("mmap error. errno=%d", errno);
		exit(1);
	}

	char command[256];
	char	buf[256];
	FILE	*fp = 0;

	sprintf(command, "objdump -h %s | grep .rodata | sed -e 's/\\s\\+/ /g' | cut -d' ' -f 6", bin_filename);
	fp = popen(command, "r");
	fgets(buf, 256, fp);
	__addr_base = strtol(buf, NULL, 16);
	pclose(fp);
	sprintf(command, "objdump -h %s | grep .rodata | sed -e 's/\\s\\+/ /g' | cut -d' ' -f 7", bin_filename);
	fp = popen(command, "r");
	fgets(buf, 256, fp);
	__bin_offset_base = strtol(buf, NULL, 16);
	pclose(fp);
}

const char *
fmtaddr2addr(const char *fmt)
{
	int64_t offset = __bin_offset_base + (fmt - (char*)__addr_base);
	return (const char *)(__bin + offset);
}

void
__wq_trace_print(struct wq_trace *trace)
{
	struct tm *time_st;
	time_t sec = (__wq_log->base_usec + trace->usec) / 1000000;
	int32_t usec = (__wq_log->base_usec + trace->usec) % 1000000;

	time_st = localtime(&sec);

	printf("%02d.%02d.%02d-%02d:%02d:%02d.%06d %24s:%-4d ",
		time_st->tm_year + 1900, time_st->tm_mon + 1, time_st->tm_mday,
		time_st->tm_hour, time_st->tm_min, time_st->tm_sec, usec,
		fmtaddr2addr(trace->func), trace->line);
}

void
__wq_infolog_print(void)
{
	uint32_t idx;
	uint32_t blks = 0;
	struct wq_trace		*trace;
	struct wq_log64_32	*_log64_32;
	struct wq_log64_64	*_log64_64;
	char *base_addr = ((char *)__wq_log) + WQ_LOGHD_SIZE;

	if (__wq_log->sys_info.head_idx !=  __wq_log->sys_info.tail_idx) {
		for (idx = __wq_log->sys_info.head_idx; idx < __wq_log->sys_info.tail_idx; idx += blks) {
			trace = (struct wq_trace*)(base_addr + idx * WQ_SYSINFO_LOG_BASE_SZ);
			switch (trace->type) {
			case WQ_LOGTYPE_TRACE:
				_log64_32 = (struct wq_log64_32*)trace;
				__wq_trace_print(trace);
				blks = WQ_LOG_BLKS(struct wq_trace);
				break;
			case WQ_LOGTYPE_INFO64_32:
				_log64_32 = (struct wq_log64_32*)trace;
				__wq_trace_print(trace);
				printf(fmtaddr2addr(_log64_32->format), _log64_32->arg[0]);
				blks = WQ_LOG_BLKS(struct wq_log64_32);
				break;
			case WQ_LOGTYPE_INFO64_64:
				_log64_64 = (struct wq_log64_64*)trace;
				__wq_trace_print(trace);
				printf(fmtaddr2addr(_log64_64->format), _log64_64->arg[0], _log64_64->arg[1], _log64_64->arg[2], _log64_64->arg[3], _log64_64->arg[4]);
				blks = WQ_LOG_BLKS(struct wq_log64_64);
				break;
			default:
				blks = WQ_SYSINFO_LOG_SIZE;
			}
			printf("\n");
		}
	}

	for (idx = 0; idx < __wq_log->sys_info.head_idx; idx += blks) {
		trace = (struct wq_trace*)(base_addr + idx * WQ_SYSINFO_LOG_BASE_SZ);
		switch (trace->type) {
		case WQ_LOGTYPE_TRACE:
			_log64_32 = (struct wq_log64_32*)trace;
			__wq_trace_print(trace);
			blks = WQ_LOG_BLKS(struct wq_trace);
			break;
		case WQ_LOGTYPE_INFO64_32:
			_log64_32 = (struct wq_log64_32*)trace;
			__wq_trace_print(trace);
			printf(fmtaddr2addr(_log64_32->format), _log64_32->arg[0]);
			blks = WQ_LOG_BLKS(struct wq_log64_32);
			break;
		case WQ_LOGTYPE_INFO64_64:
			_log64_64 = (struct wq_log64_64*)trace;
			__wq_trace_print(trace);
			printf(fmtaddr2addr(_log64_64->format), _log64_64->arg[0], _log64_64->arg[1], _log64_64->arg[2], _log64_64->arg[3], _log64_64->arg[4]);
			blks = WQ_LOG_BLKS(struct wq_log64_64);
			break;
		default:
			blks = WQ_SYSINFO_LOG_SIZE;
		}
		printf("\n");
	}
}

int
main(int argc, char *argv[])
{
	__init(argv[2], argv[1]);
	__wq_infolog_print();
	return 0;
}

