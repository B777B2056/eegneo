#include "psd.h"
#include "ui_psd.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

PSD::PSD(int c, double s, QWidget *parent) :
    QWidget(parent),
    channelNum(c), startFreq(-1.0), stopFreq(-1.0), sampleFreq(s), type(EMPTY),
    ui(new Ui::PSD)
{
    ui->setupUi(this);
    this->setWindowTitle("功率谱密度");
}

PSD::~PSD()
{
    delete chart;
    delete axisX;
    delete axisY;
    delete ui;
}

void PSD::getStartFreq(double a)
{
    this->startFreq = a;
}

void PSD::getStopFreq(double a)
{
    this->stopFreq = a;
}

void PSD::getPSDType(PSDType a)
{
    this->type = a;
}

void PSD::plot(double **x, int len)
{
    if((startFreq < 0) || (stopFreq < 0) || (startFreq >= stopFreq) || (type == EMPTY))
    {
        QMessageBox::critical(this, this->tr("错误"), "参数填写错误");
        return;
    }
    // 图像初始化
    initChart();
    // 计算功率谱估计并填充曲线流
    for(int i = 0; i < channelNum; i++)
        calcPSD(x[i], len, i);
    // 获取psd最大最小值以确定绘图纵坐标刻度
    double minP = minPSD[0], maxP = maxPSD[0];
    for(std::size_t i = 1; i < minPSD.size(); i++)
    {
        if(minP > minPSD[i])
            minP = minPSD[i];
        if(maxP < maxPSD[i])
            maxP = maxPSD[i];
    }
    axisY->setRange((int)minP - 1, (int)maxP + 1);
}

void PSD::initChart()
{
    chart = new QChart;
    axisX = new QValueAxis;
    axisY = new QValueAxis;
    series = new QSplineSeries*[channelNum];
    for(int i = 0; i < channelNum; i++)
        series[i] = new QSplineSeries;
    // 设置x轴
    axisX->setRange(startFreq, stopFreq);
    axisX->setTickCount(5);
    axisX->setTitleText("频率/Hz");
    chart->addAxis(axisX, Qt::AlignBottom);
    // 设置y轴
    axisY->setRange(-100, 40);
    axisY->setTitleText(this->type == Log ? "幅值/dB" : "幅值uV^2/Hz");
    chart->addAxis(axisY, Qt::AlignLeft);
    // 链接数据
    for(int i = 0; i < channelNum; i++)
    {
        series[i]->setUseOpenGL(true);
        QPen splinePen;
        splinePen.setBrush(Qt::blue);
        splinePen.setColor(Qt::blue);
        series[i]->setPen(splinePen);
        chart->addSeries(series[i]);
        chart->setAxisX(axisX, series[i]);
        chart->setAxisY(axisY, series[i]);
    }
    // 设置界面显示
    chart->legend()->hide();
    chart->setTheme(QChart::ChartThemeLight);
    chart->axisX()->setGridLineVisible(false);
    chart->axisY()->setGridLineVisible(false);
    chart->layout()->setContentsMargins(0, 0, 0, 0); // 设置外边界
    chart->setMargins(QMargins(0, 0, 0, 0)); //设置内边界
    chart->setBackgroundRoundness(0); // 设置背景区域无圆角
    ui->widget->setChart(chart);
}

void PSD::calcPSD(double *x, int len, int index)
{
    int i, j, k, s, m2, nrd, kmax, nsl, nsectp, nfft21, numSections, numUsed, start = -1, end = -1, m = 1024, fftLen = 2048;
    double u, fl, xsum, norm, twopi, rexmn, imxmn, xmean, minP, maxP, *xa, *xreal, *ximag, *window, *r, *psdFreq, *psdVal;
    xa = new double[fftLen];
    xreal = new double[fftLen];
    ximag = new double[fftLen];
    window = new double[m];
    nfft21 = fftLen / 2 + 1;
    r = new double[nfft21];
    psdFreq = new double[nfft21];
    psdVal = new double[nfft21];
    numSections = (len - m / 2) / (m / 2);
    numUsed = numSections * (m / 2) + m / 2;
    s = 0;
    xsum = 0.0;
    nsl = numSections + 1;
    m2 = m / 2;
    for(k = 0; k < nsl; k++)
    {
        for(i = 0; i < m2; i++)
            xa[i] = x[s + i];
        for(i = 0; i < m2; i++)
            xsum += xa[i];
        s += m2;
    }
    xmean = xsum / numUsed;
    rexmn = xmean;
    imxmn = xmean;
    u = 0.0;
    twopi = 8.0 * atan(1.0);
    fl = m - 1.0;
    for(i = 0; i < m; i++)
    {
        window[i] = 0.54 - 0.46 * cos(twopi * i / fl);
        u += (window[i] * window[i]);
    }
    s = 0;
    for(i = 0; i < nfft21; i++)
        psdVal[i] = 0.0;
    m2 = m / 2;
    for(i = 0; i < m2; i++)
        xa[i + m2] = x[s + i];
    s += m2;
    kmax = (numSections + 1) / 2;
    nsectp = (numSections + 1) / 2;
    nrd = m;
    for(k = 0; k < kmax; k++)
    {
        for(i = 0; i < m2; i++)
        {
            j = m2 + i;
            xreal[i] = xa[j];
            ximag[i] = 0.0;
        }
        if((k == (kmax - 1)) && (nsectp != numSections))
        {
            for(i = m2; i < nrd; i++)
                xa[i] = 0.0;
            nrd = m / 2;
        }
        for(i = 0; i < nrd; i++)
            xa[i] = x[s + i];
        for(i = 0; i < m2; i++)
        {
            j = m2 + i;
            xreal[j] = xa[i] - rexmn;
            ximag[j] = xa[j] - imxmn;
            xreal[i] = xreal[i] - rexmn;
            ximag[i] = xa[i] - imxmn;
        }
        if((k == (kmax - 1)) && (nsectp != numSections))
        {
            for(i = 0; i < m; i++)
                ximag[i] = 0.0;
        }
        s += nrd;
        for(i = 0; i < m; i++)
        {
            xreal[i] *= window[i];
            ximag[i] *= window[i];
        }
        if(m != fftLen)
        {
            for(i = m; i < fftLen; i++)
            {
                xreal[i] = 0.0;
                ximag[i] = 0.0;
            }
        }
        fft(xreal, ximag, fftLen, 1);
        for(i = 1; i < nfft21; i++)
        {
            j = fftLen - 1;
            psdVal[i] += (xreal[i] * xreal[i] + ximag[i] * ximag[i]);
            psdVal[i] += (xreal[j] * xreal[j] + ximag[j] * ximag[j]);
        }
        psdVal[0] += (2.0 * xreal[0] * xreal[0]);
        psdVal[0] += (2.0 * ximag[0] * ximag[0]);
    }
    norm = 2.0 * u * numSections;
    for(i = 0; i < nfft21; i++)
    {
        psdVal[i] = psdVal[i] / norm;
        xreal[i] = psdVal[i];
        ximag[i] = 0.0;
        j = fftLen - i;
        xreal[j] = xreal[i];
        ximag[j] = ximag[i];
    }
    fft(xreal, ximag, fftLen, -1);
    for(i = 0; i < nfft21; i++)
        r[i] = xreal[i];
    for(i = 0; i < nfft21; i++)
    {
        psdFreq[i] = i * sampleFreq / (double)fftLen;
        if(type == Log)
        {
            if(psdVal[i] == 0.0)
                psdVal[i] = 1.0e-15;
            psdVal[i] = 10.0 * log10(psdVal[i]);
        }
    }
    minP = psdVal[0];
    maxP = psdVal[0];
    for(i = 1; i < nfft21; i++)
    {
        if(minP > psdVal[i])
            minP = psdVal[i];
        if(maxP < psdVal[i])
            maxP = psdVal[i];
    }
    minPSD.push_back(minP);
    maxPSD.push_back(maxP);
    for(i = 0; i < nfft21; i++)
    {
        if(psdFreq[i] >= startFreq)
        {
            start = i;
            break;
        }
    }
    for(i = 0; i < nfft21; i++)
    {
        if(psdFreq[i] >= stopFreq)
        {
            end = i;
            break;
        }
    }
    while(start < end)
    {
        series[index]->append(QPointF(psdFreq[start], psdVal[start]));
        start++;
    }
    delete []xa;
    delete []xreal;
    delete []ximag;
    delete []window;
    delete []psdFreq;
    delete []psdVal;
}
