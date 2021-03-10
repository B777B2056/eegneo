#ifndef ANALYSISTOOLS_H
#define ANALYSISTOOLS_H

#include <cmath>

/* 快速傅里叶变换
 * x 数据的实部
 * y 数据虚部
 * n 数据长度
 * sign 1 离散傅里叶正变换DFT
 *     -1 离散傅里叶反变换IDFT
 */
void fft(double *x, double *y, int n, int sign);

#endif // ANALYSISTOOLS_H
