#ifndef PSD_H
#define PSD_H

#include <QWidget>
#include <QtCharts>
#include <QMessageBox>
#include <cmath>
#include <vector>
#include "psdinfo.h"

namespace Ui {
class PSD;
}

class PSD : public QWidget
{
    Q_OBJECT

public:
    explicit PSD(int c, double s, QWidget *parent = nullptr);
    ~PSD();
    void plot(double **x, int len);

private slots:
    void getStartFreq(double);
    void getStopFreq(double);
    void getPSDType(PSD_Type);

private: 
    int channelNum;
    double startFreq, stopFreq, sampleFreq;
    std::vector<double> minPSD, maxPSD;
    PSD_Type type;
    QChart *chart;
    QSplineSeries **series;
    QValueAxis *axisX, *axisY;
    /*绘图初始化*/
    void initChart();
    /* 快速傅里叶变换
     * x 数据的实部
     * y 数据虚部
     * n 数据长度
     * sign 1 离散傅里叶正变换DFT
     *     -1 离散傅里叶反变换IDFT
     */
    void fft(double *x, double *y, int n, int sign);
    /* 计算功率谱面密度
     * x 输入信号数组
     * len 输入信号数组长度
     * m 分段的长度
     * fflLen 功率谱变换所用的傅里叶变换长度
     * type 功率谱类型：线性谱还是对数谱
     */
    void calcPSD(double *x, int len, int i);

    Ui::PSD *ui;
};

#endif // PSD_H
