#ifndef PSD_H
#define PSD_H

#include <QWidget>
#include <QtCharts>
#include <QMessageBox>
#include <cmath>
#include <vector>
#include "analysistools.h"
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
