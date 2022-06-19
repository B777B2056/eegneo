#ifndef WIGNER_H
#define WIGNER_H

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#include <QWidget>
#include <QtCharts>
#include <vector>
#include <algorithm>
#include "analysistools.h"

namespace Ui {
class Wigner;
}

class Wigner : public QWidget
{
    Q_OBJECT

public:
    explicit Wigner(int c, double s, QWidget *parent = nullptr);
    ~Wigner();
    void plotWigner(std::vector<double>& x);

private slots:
    void getBeginTime(double);
    void getEndTime(double);
    void getChannel(QString);

private:
    int m, channelNum;
    double beginTime, endTime, sampleFreq;
    QString channel;
    QChart **chart;
    QSplineSeries **series[2];
    QValueAxis **axisX, **axisY;
    void initChart(int index);
    int findMin2(int up);  // 找到与起始时间最接近的2的幂次方整数
    void analytic(double *x, double *y, int len);  // 产生解析信号
    void calc(std::vector<double>& x);

    Ui::Wigner *ui;
};

#endif // WIGNER_H
