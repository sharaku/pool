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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <devfile/devgrp.hpp>
#include <wq/wq.h>

class device_exsample
 : public devgrp
{
public:
	enum {
		EXS_DEVICE1 = 0x00000001,
		EXS_DEVICE2 = 0x00000002,
		EXS_DEVICE3 = 0x00000004, 
		EXS_DEVICE4 = 0x00000008,
		EXS_DEVICE5 = 0x00000010,
		EXS_DEVICE6 = 0x00000020,
	};

	device_exsample() {
		DEVFILE_REGIST(EXS_DEVICE1, "./testdevice.1", device1);
		DEVFILE_REGIST(EXS_DEVICE2, "./testdevice.2", device2);
		DEVFILE_REGIST(EXS_DEVICE3, "./testdevice.3", device3);
		DEVFILE_REGIST(EXS_DEVICE4, "./testdevice.4", device4);
		DEVFILE_REGIST(EXS_DEVICE5, "./testdevice.5", device5);
		DEVFILE_REGIST(EXS_DEVICE6, "./testdevice.6", device6);
	}

	devfile_str_int_rw	device1;
	devfile_str_int_rw	device2;
	devfile_str_int_rw	device3;
	devfile_str_int_rw	device4;
	devfile_str_int_rw	device5;
	devfile_str_int_rw	device6;
};

device_exsample	dev;

static void
timer_sched_cb(wq_item_t *item, wq_arg_t arg)
{
	struct timeval tv;
	uint32_t us;

	// 4ms周期で動かす
	wq_timer_sched(item, WQ_TIME_MS(4), timer_sched_cb, NULL);

	dev.update(0x0000003f);
	dev.device1 = dev.device1 + 1;
	dev.device2 = dev.device2 + 1;
	dev.device3 = dev.device3 + 1;
	dev.device4 = dev.device4 + 1;
	dev.device5 = dev.device5 + 1;
	dev.device6 = dev.device6 + 1;
	dev.commit(0x0000003f);
}

int
main(void)
{
	wq_item_t	item_timer;

	// アクセス用ファイルを再生する。
	//	パス			初期値
	//	./test-device.1		""
	//	./test-device.2		""
	//	./test-device.3		""
	//	./test-device.4		""
	//	./test-device.5		""
	//	./test-device.6		""
	int fd;
	int i;
	char path[64];
	for (i = 1; i < 7; i++) {
		sprintf(path, "./testdevice.%d", i);
		fd = open(path, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
		close(fd);
	}

	wq_init_item_prio(&item_timer, 0);
	wq_sched(&item_timer, timer_sched_cb, NULL);

	// 自信をworkerスレッドとする。
	printf("wq_run\n");
	wq_run();
	return 0;
}

