#ifndef FILTER_H
#define FILTER_H

#include <math.h>
#include <assert.h>

#define PI 3.1415926535

/* 注：FIR滤波器阶数必须为奇数，这样才能使FIR滤波器为I型滤波器
 * 若滤波器非I型，则对于所设计的滤波器类型存在要求(例如不能设计某些类型滤波器)
 */
class MyFilter{
private:
    /*汉明窗函数计算*/
    double hammingWindow(int n, int N)
    {
        return 0.54 - 0.46 * cos(2 * PI * n / (N - 1));
    }
public:
    MyFilter() {}
    ~MyFilter() {}
    /*带通滤波器*/
    void countBandPassCoef(int order, int sample_frequency, double *h, double low_cut, double high_cut);
    /*带阻滤波器(此处特化为陷波器)*/
    void countNotchCoef(int order, int sample_frequency, double *h, double notch_cut);
};

#endif // FILTER_H
