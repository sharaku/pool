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
**/

#ifndef _MUTEX_H_
#define _MUTEX_H_

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

#define	MTX_TV2MSEC(TV)	(TV.tv_sec * 1000 + TV.tv_usec / 1000)

#include <pthread.h>
#include <sys/time.h>
CPP_SRC(extern "C" {)

struct ___mutex_lock_pthread {
	pthread_mutex_t	mutex;
};
struct ___mutex_cond_pthread {
	pthread_cond_t	cond;
};

static inline void
__init_mutex_lock_pthread(struct ___mutex_lock_pthread *m)
{
	pthread_mutex_init(&m->mutex, NULL);
}

static inline void
__init_mutex_cond_pthread(struct ___mutex_cond_pthread *c)
{
	pthread_cond_init(&c->cond, NULL);
}

static inline void
__mutex_lock_pthread(struct ___mutex_lock_pthread *m)
{
	pthread_mutex_lock(&m->mutex);
}

static inline void
__mutex_unlock_pthread(struct ___mutex_lock_pthread *m)
{
	pthread_mutex_unlock(&m->mutex);
}

static inline void
__mutex_signal_pthread(struct ___mutex_lock_pthread *m,
		       struct ___mutex_cond_pthread *c)
{
	// cond_signalを発行する
	pthread_mutex_lock(&m->mutex);
	pthread_cond_signal(&c->cond);
	pthread_mutex_unlock(&m->mutex);
}

static inline void
__mutex_timedwait_pthread(struct ___mutex_lock_pthread *m,
			  struct ___mutex_cond_pthread *c,
			  int ms_timeo)
{
	struct timeval	now;
	struct timespec	timeout;
	int64_t	ms;

	// cond_waitで待ち合わせる
	gettimeofday(&now, NULL);
	ms = MTX_TV2MSEC(now);
	ms += ms_timeo;
	timeout.tv_sec =  ms / 1000;
	timeout.tv_nsec = (ms % 1000) * 1000000;

	pthread_mutex_lock(&m->mutex);
	pthread_cond_timedwait(&c->cond, &m->mutex, &timeout);
	pthread_mutex_unlock(&m->mutex);
}

static inline int
__cre_task_pthread(void * (*func)(void *), void *arg)
{
	pthread_t	*thread;
	pthread_attr_t	attr;
	int		rc;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	rc = pthread_create(thread, &attr, func, (void*)arg);
	if (rc) {
		return 1;
	}
	return 0;
}

CPP_SRC(})

typedef struct ___mutex_lock_pthread mutex_lock_t;
typedef struct ___mutex_cond_pthread mutex_cond_t;

#define init_mutex_lock(m)		__init_mutex_lock_pthread(m)
#define init_mutex_cond(c)		__init_mutex_cond_pthread(c)
#define mutex_lock(m)			__mutex_lock_pthread(m)
#define mutex_unlock(m)			__mutex_unlock_pthread(m)
#define mutex_signal(m, c)		__mutex_signal_pthread(m, c)
#define mutex_timedwait(m, c, ms)	__mutex_timedwait_pthread(m, c, ms)
#define cre_task(f, a)			__cre_task_pthread(f, a)


#endif // _SPINLOCK_H_
