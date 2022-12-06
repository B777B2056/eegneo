#pragma once
#include <cmath>
#include <cassert>

/* 汉明窗函数计算 */
#define hammingWindow(n, N) \
    (   \
        0.54 - 0.46 * cos(2 * (M_PI) * (n) / ((N) - 1)) \
    )

// 注：FIR滤波器阶数必须为奇数，这样才能使FIR滤波器为I型滤波器
// 若滤波器非I型，则对于所设计的滤波器类型存在要求(例如不能设计某些类型滤波器)
namespace Filter{
    // 带通滤波器
    void countBandPassCoef(int order, int sample_frequency, double *h, double low_cut, double high_cut);
    // 带阻滤波器(此处特化为陷波器)
    void countNotchCoef(int order, int sample_frequency, double *h, double notch_cut);
};
