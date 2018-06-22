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

#ifndef _PID_H_
#define _PID_H_

#include <stdint.h>
#include <generic.h>

//-----------------------------------------------------------------------------
// PID制御の実装
//  PID特性
//   Kp
//     targetとnowとの差分に比例する。
//     目標との差が大きいほど出力値も大きくなる。
//     目標値との差分がそのまま出力値に反映される。
//   Ki
//     targetとnowとの差分を積算した値の比例する。
//     前回の出力値が大きいほど出力値も大きくなる
//     出力値の変化が緩やかになる。
//   Kd
//    targetとnowとの差分と前回差分との差の大きさに比例する。
//    変化が大きいときに出力値も大きくなる。
//    瞬間的な変化が出力値に反映される。


CPP_SRC(extern "C" {)

struct pid
{
	float	kp;
	float	ki;
	float	kd;
	float	ei;		// 誤差積分
	float	el;		// 前回誤差
};


#define PID_INIT(kp, ki, kd) { (float)kp, (float)ki, (float)kd, 0.0f, 0.0f }


static inline void
pid_set(struct pid *p, float kp, float ki, float kd)
{
	p->kp = (float)kp;
	p->ki = (float)ki;
	p->kd = (float)kd;
}

static inline void
pid_clear(struct pid *p)
{
	p->ei = 0.0f;
	p->el = 0.0f;
}

static inline void
init_pid(struct pid *p, float kp, float ki, float kd)
{
	pid_set(p, kp, ki, kd);
	pid_clear(p);
}

static inline float
pid_update(struct pid *p, float delta_ms, int32_t now, int32_t target)
{
	register float	u = 0.0f;
	register float	e = 0.0f;
	register float	ed = 0.0f;

	// Δ時間T
	// 誤差e      = 目標値 - 現在値
	// 誤差積分ei = ei + e * T
	// 誤差微分ed = (e - 前回誤差el) / T
	// 前回誤差el = e
	e  = target - now;		// 誤差
	p->ei = p->ei + e * delta_ms;	// 誤差積分
	ed = (e - p->el) / delta_ms;	// 誤差微分
	p->el = e;

	// PID計算
	// 操作量u = 比例ゲインKP * e
	//           + 積分ゲインKI * ei
	//           + 微分ゲインKD * ed
	u = p->kp * e + p->ki * p->ei + p->kd * ed;

	return u;
}

CPP_SRC(})

#endif // _PID_H_
