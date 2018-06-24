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

#ifndef _DEVFILE_HPP_
#define _DEVFILE_HPP_

#include <devfile/devfile.h>

class devfile_rw {
public:
	devfile_rw() {}
	virtual int init(const char *path) = 0;
	virtual ~devfile_rw() {
		devfile_accessor_close(&da);
	}
	int32_t open(void) {
		return devfile_accessor_open(&da);
	}
	int32_t close(void) {
		return devfile_accessor_close(&da);
	}
	virtual int32_t update(void) = 0;
	virtual int32_t commit(void) = 0;
	virtual int get_id(void) {
		return da.fd;
	}

protected:
	devfile_accessor_t	da;
};

class devfile_str_rw
 : public devfile_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_RDWR);
	}
	int32_t update(void) {
		return devfile_accessor_update_str(&da);
	}
	int32_t commit(void) {
		return devfile_accessor_commit_str(&da);
	}
	operator const char*(void) {
		return (const char*)da.buffer;
	}
	devfile_str_rw& operator=(const char *val) {
		strncpy(da.buffer, val, DEVICE_BUFFER_LEN);
		return *this;
	}
};
class devfile_raw_rw
 : public devfile_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_RDWR);
	}
	int32_t update(void) {
		return devfile_accessor_update_raw(&da);
	}
	int32_t commit(void) {
		return devfile_accessor_commit_raw(&da);
	}
	operator int32_t(void) {
		return (int32_t)da.buffer_int32;
	}
	devfile_raw_rw& operator=(int32_t val) {
		da.buffer_int32 = val;
		return *this;
	}
};
class devfile_str_int_rw
 : public devfile_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_RDWR);
	}
	int32_t update(void) {
		return devfile_accessor_update_str_int(&da);
	}
	int32_t commit(void) {
		return devfile_accessor_commit_str_int(&da);
	}
	operator int32_t(void) {
		return (int32_t)da.buffer_int32;
	}
	devfile_str_int_rw& operator=(int32_t val) {
		da.buffer_int32 = val;
		return *this;
	}
};

class devfile_str_r
 : public devfile_str_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_RDONLY);
	}
	int32_t update(void) {
		return devfile_accessor_update_str(&da);
	}
	operator const char*(void) {
		return (const char*)da.buffer;
	}
private:
	int32_t commit(void) {
		return devfile_accessor_commit_str(&da);
	}
	devfile_str_r& operator=(const char *val) {
		strncpy(da.buffer, val, DEVICE_BUFFER_LEN);
		return *this;
	}
};
class devfile_raw_r
 : public devfile_raw_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_RDONLY);
	}
	int32_t update(void) {
		return devfile_accessor_update_raw(&da);
	}
	operator int32_t(void) {
		return (int32_t)da.buffer_int32;
	}
private:
	int32_t commit(void) {
		return devfile_accessor_commit_raw(&da);
	}
	devfile_raw_r& operator=(int32_t val) {
		da.buffer_int32 = val;
		return *this;
	}
};
class devfile_str_int_r
 : public devfile_str_int_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_RDONLY);
	}
	int32_t update(void) {
		return devfile_accessor_update_str_int(&da);
	}
	operator int32_t(void) {
		return (int32_t)da.buffer_int32;
	}
private:
	int32_t commit(void) {
		return devfile_accessor_commit_str_int(&da);
	}
	devfile_str_int_r& operator=(int32_t val) {
		da.buffer_int32 = val;
		return *this;
	}
};

class devfile_str_w
 : public devfile_str_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_WRONLY);
	}
	int32_t commit(void) {
		return devfile_accessor_commit_str(&da);
	}
	devfile_str_w& operator=(const char *val) {
		strncpy(da.buffer, val, DEVICE_BUFFER_LEN);
		return *this;
	}
private:
	int32_t update(void) {
		return devfile_accessor_update_str(&da);
	}
	operator const char*(void) {
		return (const char*)da.buffer;
	}
};
class devfile_raw_w
 : public devfile_raw_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_WRONLY);
	}
	int32_t commit(void) {
		return devfile_accessor_commit_raw(&da);
	}
	devfile_raw_rw& operator=(int32_t val) {
		da.buffer_int32 = val;
		return *this;
	}
private:
	int32_t update(void) {
		return devfile_accessor_update_raw(&da);
	}
	operator int32_t(void) {
		return (int32_t)da.buffer_int32;
	}
};
class devfile_str_int_w
 : public devfile_str_int_rw {
public:
	virtual int init(const char *path) {
		return init_devfile_accessor(&da, path, O_NONBLOCK|O_WRONLY);
	}
	int32_t commit(void) {
		return devfile_accessor_commit_str_int(&da);
	}
	devfile_str_int_w& operator=(int32_t val) {
		da.buffer_int32 = val;
		return *this;
	}
private:
	int32_t update(void) {
		return devfile_accessor_update_str_int(&da);
	}
	operator int32_t(void) {
		return (int32_t)da.buffer_int32;
	}
};


#define DEVFILE_REGIST(id, path, df) \
	do {				\
		(df).init(path);	\
		regist(id, &(df));	\
		(df).open();		\
	} while (0)

#endif // _DEVFILE_HPP_
