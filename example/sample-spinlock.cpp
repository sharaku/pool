/* --

MIT License

Copyright (c) 2018 Abe Takafumi

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

// gcc -I../src -Wl,--no-as-needed -lpthread sample-spinlock.cpp

#include <pthread.h>
#include <stdio.h>
#include <seqlock.h>


spinlock_t	lock = SPINLOCK_INIT;
int		g = 0;

void *
thread_func(void *param)
{
	int i;
	spin_lock(&lock);
	for (i = 0; i < 1000; i++) {
		g = g + 1;
	}
	printf("thread_func %d\n", g);
	spin_unlock(&lock);
}

int
main(int argc, char *argv[])
{
	pthread_t thread[64];
	int rc;
	int i;

	for (i = 0; i < 64; i++) {
		rc = pthread_create(&thread[i], NULL, thread_func, NULL);
		if (rc != 0) {
			return 1;
		}
	}
	for (i = 0; i < 64; i++) {
		pthread_join(thread[i], NULL);
	}

	return 0;
}
