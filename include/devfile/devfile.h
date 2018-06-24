/* --
 *
 * MIT License
 * 
 * Copyright (c) 2015 Abe Takafumi
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
 * SOFTWARE. *
 *
 */

#ifndef _DEVFILE_H_
#define _DEVFILE_H_

#include <sys/types.h>
#include <sys/stat.h>

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <generic.h>
#include <atoi_fast.h>
#include <log/log.h>

#ifndef DEVICE_FILEPATH_MAXLEN
#define DEVICE_FILEPATH_MAXLEN		128
#endif
#ifndef DEVICE_BUFFER_LEN
#define DEVICE_BUFFER_LEN		64
#endif

enum {
	DEVFILE_TYPE_STRING,		// 文字列
	DEVFILE_TYPE_STR_INT32,		// 文字列をバイナリへ変換する
	DEVFILE_TYPE_RAW_INT32,		// バイナリを直接取得
};

CPP_SRC(extern "C" {)

typedef struct devfile_accessor
{
	int		fd;
	int		open_flag;
	char		file_path[DEVICE_FILEPATH_MAXLEN];
	char		buffer[DEVICE_BUFFER_LEN];
	int32_t		buffer_int32;
} devfile_accessor_t;

// devfile_accessorを初期化する
static int
init_devfile_accessor(devfile_accessor_t *da, const char *path, int flag)
{
	struct stat buf;
	int result;

	da->buffer_int32 = 0;
	da->fd = -1;
	da->open_flag = 0;

	result = stat(path, &buf);
	if (result) {
		wq_infolog64("stat error errno=%d.", errno);
		return result;
	}
	strncpy(da->file_path, path, DEVICE_FILEPATH_MAXLEN);
	da->open_flag = flag;
	return 0;
}

// devfile_accessorの共通open処理。
// 直接呼出し禁止
static inline int
__devfile_accessor_open(devfile_accessor_t *da, int open_flag)
{
	int fd = 0;

	fd = open(da->file_path, open_flag);
	if (fd == -1) {
		wq_infolog64("open error errno=%d.", errno);
	}
	wq_infolog64("open complete fd=%d.", fd);
	return fd;
}

// devfile_accessorのopen処理。永続的にopenを行う。
static int32_t
devfile_accessor_open(devfile_accessor_t *da)
{
	int result = 0;
	if (da->fd == -1) {
		da->fd = __devfile_accessor_open(da, da->open_flag);
	}
	if (da->fd == -1) {
		result = -1;
	}
	return result;
}

// devfile_accessorのclose処理。永続的にcloseを行う。
static int32_t
devfile_accessor_close(devfile_accessor_t *da)
{
	if (da->fd != -1) {
		wq_infolog64("close. fd=%d", da->fd);
		close(da->fd);
		da->fd = -1;
	}
	return 0;
}


// ファイルディスクリプタを取得する。openされていなければopenを行う。
static inline int
__devfile_accessor_fd_get_or_open(devfile_accessor_t *da, int flag)
{
	int		fd = da->fd;
	if (fd == -1) {
		fd = __devfile_accessor_open(da, flag);
		if (fd == -1) {
			fd = -errno;
		}
	}
	return fd;
}

// ファイルディスクリプタのcloseを行う。
// open済みの場合はopen状態を維持する。
static inline void
__devfile_accessor_fd_close(devfile_accessor_t *da, int fd)
{
	if (fd != da->fd) {
		close(fd);
		wq_infolog64("close complete fd=%d.", fd);
	}
}

// ファイルディスクリプタのcloseを行う。
// open済みの場合はopen状態を維持する。
static inline void
__devfile_accessor_fd_close_or_sync(devfile_accessor_t *da, int fd)
{
	if (fd != da->fd) {
		close(fd);
		wq_infolog64("close complete fd=%d.", fd);
	} else {
		// closeしない場合は、更新を行う
		fsync(fd);
	}
}


static int32_t
devfile_accessor_update_str(devfile_accessor_t *da)
{
	int fd;
	int result = 0;

	fd = __devfile_accessor_fd_get_or_open(da, O_NONBLOCK|O_RDONLY);
	if (fd < 0) {
		return -fd;
	}
	if (pread(fd, da->buffer, DEVICE_BUFFER_LEN, 0) == -1) {
		result = errno;
		wq_infolog64("pread error errno=%d.", errno);
	} else {
		result = 0;
	}
	__devfile_accessor_fd_close(da, fd);
	return result;
}

static int32_t
devfile_accessor_update_raw(devfile_accessor_t *da)
{
	int fd;
	int result = 0;

	fd = __devfile_accessor_fd_get_or_open(da, O_NONBLOCK|O_RDONLY);
	if (fd < 0) {
		return -fd;
	}
	if (pread(fd, &(da->buffer_int32), sizeof(da->buffer_int32), 0) == -1) {
		result = errno;
		wq_infolog64("pread error errno=%d.", errno);
	} else {
		result = 0;
	}
	__devfile_accessor_fd_close(da, fd);
	return result;
}

static int32_t
devfile_accessor_update_str_int(devfile_accessor_t *da)
{
	int fd;
	int result = 0;

	fd = __devfile_accessor_fd_get_or_open(da, O_NONBLOCK|O_RDONLY);
	if (fd < 0) {
		return -fd;
	}
	if (pread(fd, da->buffer, DEVICE_BUFFER_LEN, 0) == -1) {
		result = errno;
		wq_infolog64("pread error errno=%d.", errno);
	} else {
		result = 0;
		da->buffer_int32 = atoi_fast(da->buffer);
	}
	__devfile_accessor_fd_close(da, fd);
	return result;
}

static int32_t
devfile_accessor_commit_str(devfile_accessor_t *da)
{
	int fd;
	int result = 0;

	fd = __devfile_accessor_fd_get_or_open(da, O_NONBLOCK|O_WRONLY);
	if (fd < 0) {
		return -fd;
	}
	if (pwrite(fd, da->buffer, strlen(da->buffer), 0) == -1) {
		result = errno;
		wq_infolog64("pwrite error errno=%d.", errno);
	} else {
		result = 0;
	}
	__devfile_accessor_fd_close_or_sync(da, fd);
	return result;
}

static int32_t
devfile_accessor_commit_raw(devfile_accessor_t *da)
{
	int fd;
	int result = 0;

	fd = __devfile_accessor_fd_get_or_open(da, O_NONBLOCK|O_WRONLY);
	if (fd < 0) {
		return -fd;
	}
	if (pwrite(fd, &(da->buffer_int32), sizeof(da->buffer_int32), 0) == -1) {
		result = errno;
		wq_infolog64("pwrite error errno=%d.", errno);
	} else {
		result = 0;
	}
	__devfile_accessor_fd_close_or_sync(da, fd);
	return result;
}

static int32_t
devfile_accessor_commit_str_int(devfile_accessor_t *da)
{
	int fd;
	int result = 0;

	fd = __devfile_accessor_fd_get_or_open(da, O_NONBLOCK|O_WRONLY);
	if (fd < 0) {
		return -fd;
	}
	itoa_fast(da->buffer_int32, da->buffer);
	if (pwrite(fd, da->buffer, strlen(da->buffer), 0) == -1) {
		result = errno;
		wq_infolog64("pwrite error errno=%d.", errno);
	} else {
		result = 0;
	}
	__devfile_accessor_fd_close_or_sync(da, fd);
	return result;
}


CPP_SRC(})

#endif // _DEVFILE_H_
