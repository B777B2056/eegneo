#include "wigner.h"
#include "ui_wigner.h"
#include <iostream>

Wigner::Wigner(int c, double s, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Wigner)
{
    ui->setupUi(this);
    this->channelNum = c;
    this->sampleFreq = s;
    chart = new QChart*[2];
    axisX = new QValueAxis*[2];
    axisY = new QValueAxis*[2];
    series = new QSplineSeries*[2];
}

Wigner::~Wigner()
{
    delete ui;
}

void Wigner::getBeginTime(double a)
{
    this->beginTime = a;
}

void Wigner::getEndTime(double a)
{
    this->endTime = a;
}

void Wigner::getChannel(QString a)
{
    this->channel = a;
}

void Wigner::plotWigner(std::vector<double>& x)
{

    if((beginTime >= 0.0) && (endTime > beginTime))
    {
        initChart(0);
        initChart(1);        calc(x);
    }
    else
        QMessageBox::critical(this, this->tr("错误"), "参数填写错误！");
}

void Wigner::initChart(int index)
{
    chart[index] = new QChart;
    axisX[index] = new QValueAxis;
    axisY[index] = new QValueAxis;
    series[index] = new QSplineSeries;
    //设置x轴
    axisX[index]->setRange(beginTime, endTime);
    axisX[index]->setTickCount(5);
    chart[index]->addAxis(axisX[index], Qt::AlignBottom);
    //设置y轴
    axisY[index]->setRange(beginTime, endTime);
    chart[index]->addAxis(axisY[index], Qt::AlignLeft);
    //链接数据
    series[index]->setUseOpenGL(true);
    QPen splinePen;
    splinePen.setBrush(Qt::blue);
    splinePen.setColor(Qt::blue);
    series[index]->setPen(splinePen);
    chart[index]->addSeries(series[index]);
    chart[index]->setAxisX(axisX[index], series[index]);
    chart[index]->setAxisY(axisY[index], series[index]);
    //设置界面显示
    chart[index]->legend()->hide();
    chart[index]->setTheme(QChart::ChartThemeLight);
    chart[index]->axisX()->setGridLineVisible(false);
    chart[index]->axisY()->setGridLineVisible(false);
    chart[index]->layout()->setContentsMargins(0, 0, 0, 0);//设置外边界
    chart[index]->setMargins(QMargins(0, 0, 0, 0));//设置内边界
    chart[index]->setBackgroundRoundness(0);//设置背景区域无圆角
    if(index == 0)
        ui->widget->setChart(chart[index]);
    else
        ui->widget_2->setChart(chart[index]);
}

int Wigner::findMin2(int up)
{
    int i;
    for(i = 0; pow(2, i) < up; i++);
    return pow(2, i - 1);
}

void Wigner::analytic(double *x, double *y, int len)
{
    int i, n1 = len / 2;
    for(i = 0; i < len; i++)
        y[i] = 0.0;
    fft(x, y, len, 1);
    for(i = 1; i < n1; i++)
    {
        x[i] *= 2;
        y[i] *= 2;
    }
    for(i = n1; i < len; i++)
    {
        x[i] = 0.0;
        y[i] = 0.0;
    }
    fft(x, y, len, -1);
}

/*m必须小于起始时间且必须是2的整数次幂*/
void Wigner::calc(std::vector<double>& x)
{
    int i, j, len = findMin2(x.size()), m = findMin2(this->beginTime);
    double am, freqy, min_freqy = 65536.0, max_freqy = 0.0, min_am = 65536.0, max_am = 0.0, *xc, *y, *sr, *si;
    sr = new double[m];
    si = new double[m];
    y = new double[len];
    xc = new double[len];
    x.erase(x.begin() + len, x.end());
    std::copy(x.begin(), x.end(), xc);
    std::cout << len;
    analytic(xc, y, len);
    for(i = (int)this->beginTime; i < (int)this->endTime; i++)
    {
        for(j = 0; j < m / 2; j++)
        {
            sr[j] = xc[i + j] * xc[i - j] + y[i + j] * y[i - j];
            si[j] = y[i + j] * xc[i - j] - xc[i + j] * y[i - j];
        }
        for(j = m / 2; j < m; j++)
        {
            sr[j] = xc[i + j - m] * xc[i - j + m] + y[i + j - m] * y[i - j + m];
            si[j] = y[i + j - m] * xc[i - j + m] - xc[i + j - m] * y[i - j + m];
        }
        fft(sr, si, m, 1);
        // 第i时刻的Wigner分布
        for(j = 0; j < m; j++)
        {
            am = 2 * sqrt(sr[j] * sr[j] + si[j] * si[j]);
            freqy = j * this->sampleFreq / (2.0 * m);
            if(freqy < min_freqy)
                min_freqy = freqy;
            if(freqy > max_freqy)
                max_freqy = freqy;
            if(am < min_am)
                min_am = am;
            if(am > max_am)
                max_am = am;
            series[0]->append(QPointF(freqy, am));
            series[1]->append(QPointF(i, am));
        }
    }
    axisX[0]->setRange(min_freqy, max_freqy);
    axisY[0]->setRange(min_am, max_am);
    axisY[1]->setRange(min_am, max_am);
}
