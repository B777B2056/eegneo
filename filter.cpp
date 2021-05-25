#include "filter.h"

void MyFilter::countBandPassCoef(int order, int sample_frequency, double *h, double low_cut, double high_cut)
{
    assert(order % 2);
    for (int i=0; i <= order / 2; i++)
    {
        double s = (double)i - (double)order / 2.0;
        h[i] = (sin(2 * PI * high_cut * s / sample_frequency) - sin(2 * PI * low_cut * s / sample_frequency)) / (PI * s);
        h[i] = h[i] * hammingWindow(i, order + 1);
        h[order - i] = h[i];
    }
}

/*《数字信号处理C语言程序集》P230带阻滤波器处有错误，带阻滤波器的计算公式应为
 * h[i] = (sin(pi * s) - sin(wc1 * s) - sin(wc2 * s)) / (pi * s)
 */
void MyFilter::countNotchCoef(int order, int sample_frequency, double *h, double notch_cut)
{
    assert(order % 2);
    for (int i=0; i <= order / 2; i++)
    {
        double s = (double)i - (double)order / 2.0;
        h[i] = (-sin(2 * PI * notch_cut * s / sample_frequency) + sin(PI * s) - sin(2 * PI * (notch_cut + 0.01) * s / sample_frequency)) / (PI * s);
        h[i] = h[i] * hammingWindow(i, order + 1);
        h[order - i] = h[i];
    }
}

