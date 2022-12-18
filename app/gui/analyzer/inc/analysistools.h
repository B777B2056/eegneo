#pragma once

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#include <cmath>

/* 快速傅里叶变换
 * x 数据的实部
 * y 数据虚部
 * n 数据长度
 * sign 1 离散傅里叶正变换DFT
 *     -1 离散傅里叶反变换IDFT
 */
void fft(double *x, double *y, int n, int sign);

/* 离散小波变换
 * g 尺度系数，长度为wlen
 * h 小波系数，长度为wlen
 * wlen 小波序列长度
 * c 原始信号(输入)与小波分解的平滑信号(输出)
 * d 小波分解的细节信号
 * m 小波分解级数
 * sca 小波分解时每级数据长度，sca[0]是原始信号长度，sca[i]是小波分解时第i级数据长度，长度为m+1
 */
void dwt(double *g, double *h, int wlen, double *c, double *d, int m, double *sca);
