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

#ifndef _SEQLOCK_H_
#define _SEQLOCK_H_
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


#include <spinlock.h>
#include <atomic.h>

CPP_SRC(extern "C" {)

typedef struct seqcount {
	// カウンタはCPUが一度に読めるサイズにする。
	unsigned int sequence;
	// 後から拡張できるように構造体にしておく。
} seqcount_t;

typedef struct seqlock {
	seqcount_t seqcount;
	spinlock_t lock;
} seqlock_t;


#define SEQLOCK_INIT { {0}, SPINLOCK_INIT }

static inline void
init_seqlock(seqlock_t *sl)
{
	sl->seqcount.sequence = 0;
	init_spinlock(&sl->lock);
}


static inline void
write_seqlock(seqlock_t *sl)
{
	spin_lock(&sl->lock);
	sl->seqcount.sequence++;
	mb();
}

// 排他できなければ1を応答する。
static inline int
write_try_seqlock(seqlock_t *sl)
{
	int rc = spin_try_lock(&sl->lock);
	if (rc) {
		return 1;
	}
	sl->seqcount.sequence++;
	mb();
	return 0;
}

static inline void
write_sequnlock(seqlock_t *sl)
{
	mb();
	sl->seqcount.sequence++;
	spin_unlock(&sl->lock);
}

// writeが終了するまでwaitする。
static inline unsigned int
__read_seqcount_begin_or_wait(const seqcount_t *s)
{
	unsigned int ret;

repeat:
	ret = s->sequence;
	if (ret & 1) {
		mb();
		goto repeat;
	}
	return ret;
}

// writeが終了するまでwaitする。
static inline unsigned int
__read_seqcount_begin(const seqcount_t *s)
{
	unsigned int ret;
	mb();
	ret = s->sequence;

	// Write中にreadした場合の対策。
	// begin値は最下位のflagを落とした状態で応答する。
	return ret & (~1);
}

// 書き込みしていても構わずに応答する。
static inline unsigned int
read_seqbegin(const seqlock_t *sl)
{
	return __read_seqcount_begin(&sl->seqcount);
}

// 書き込みが終了するまで待つ
static inline unsigned int
read_seqbegin_or_wait(const seqlock_t *sl)
{
	return __read_seqcount_begin_or_wait(&sl->seqcount);
}

// 書き込み中は1を返す
static inline unsigned int
read_try_seqbegin(const seqlock_t *sl, unsigned int *seq)
{
	*seq = __read_seqcount_begin_or_wait(&sl->seqcount);
	return *seq & 1;
}

static inline unsigned int
read_seqretry(const seqlock_t *sl, unsigned int start)
{
	return sl->seqcount.sequence != start;
}

static inline void
read_seqlock_excl(seqlock_t *sl)
{
	spin_lock(&sl->lock);
}

static inline int
read_try_seqlock_excl(seqlock_t *sl)
{
	int rc = spin_try_lock(&sl->lock);
	if (rc) {
		return 1;
	} else {
		return 0;
	}
}

static inline void
read_sequnlock_excl(seqlock_t *sl)
{
	spin_unlock(&sl->lock);
}

CPP_SRC(})

#endif // _SEQLOCK_H_
