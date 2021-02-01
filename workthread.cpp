#include "workthread.h"
#include <iostream>

DataThread::DataThread(int channels_num) : isFilt(false), isRec(false)
{
    this->channels_num = channels_num;
    for(int i = 0; i < this->channels_num; i++)
    {
        data.push_back(0.0);
        filtData.push_back(0.0);
        bandPassBuffer.push_back(QQueue<double>());
        notchBuffer.push_back(QQueue<double>());
        bandPassCoff.push_back(new double[channels_num]);
        notchCoff.push_back(new double[channels_num]);
    }
}

DataThread::~DataThread()
{
    delete dataTimer;
    for(std::size_t i = 0; i < bandPassCoff.size(); i++)
    {
        delete []bandPassCoff[i];
        delete []notchCoff[i];
    }
}

void DataThread::run()
{
    dataTimer = new QTimer();
    dataTimer->setInterval(20);
    dataTimer->start();
    connect(dataTimer, SIGNAL(timeout()), this, SLOT(generateData()));
    //启动子线程消息循环
    this->exec();
}

void DataThread::generateData()
{
    for(std::size_t i = 0; i < data.size(); i++)
    {
        data[i] = 45 * sin(msecCnt * (2 * 3.1415926535 / 10)) + (rand() % 10 - 5);
        if(isRec)
        {
            /*写入缓存txt文件*/
            if(i < data.size() - 1)
                samplesWrite << data[i] << " ";
            else
                samplesWrite << data[i] << std::endl;
        }
        if(isFilt)
        {
            Filter f;
            /*原始数据进入带通滤波缓冲区*/
            if(bandPassBuffer[i].size() < FILTER_ORDER + 1)
            {
                bandPassBuffer[i].enqueue(data[i]);
            }
            /*带通滤波*/
            else
            {
                double bp_y_n;
                /*计算滤波后的值*/
                bp_y_n = conv(BandPass, i);
                /*队列左移一位*/
                bandPassBuffer[i].dequeue();
                /*原始值入队*/
                bandPassBuffer[i].enqueue(data[i]);
                /*带通滤波后的值入队*/
                if(notchBuffer[i].size() < FILTER_ORDER + 1)
                {
                    notchBuffer[i].enqueue(bp_y_n);
                    data[i] = bp_y_n;
                }
                else
                {
                    double y_n = conv(Notch, i);
                    notchBuffer[i].dequeue();
                    notchBuffer[i].enqueue(bp_y_n);
                    data[i] = y_n;
                }
                emit inFilt();
            }
        }
    }
    msecCnt++;
    emit sendData(data);
}

void DataThread::startRec(std::string tempFile)
{
    samplesWrite.open(tempFile);
    samplesWrite.close();
    samplesWrite.open(tempFile, std::ios::app);
    for(int i = 0; i < channels_num; i++)
    {
        if(i < channels_num - 1)
            samplesWrite << i + 1 << " ";
        else
            samplesWrite << i + 1 << std::endl;
    }
    isRec = true;
}

void DataThread::stopRec()
{
    isRec = false;
    samplesWrite.close();
}

void DataThread::startFilt(int lowCut, int highCut, int notchCut)
{
    this->isFilt = true;
    Filter f;
    for(std::size_t i = 0; i < data.size(); i++)
    {
        /*计算带通滤波器冲激响应*/
        double *cur_bp_h = new double[FILTER_ORDER + 1];
        f.countBandPassCoef(FILTER_ORDER, SAMPLE_RATE, cur_bp_h, lowCut, highCut);
        bandPassCoff[i] = cur_bp_h;
        /*计算陷波器冲激响应*/
        double *cur_n_h = new double[FILTER_ORDER + 1];
        f.countNotchCoef(FILTER_ORDER, SAMPLE_RATE, cur_n_h, notchCut);
        notchCoff[i] = cur_n_h;
    }
}

/*时域序列卷积*/
double DataThread::conv(filter type, int index)
{
    double y_n = 0.0;
    if(type == BandPass)
    {
        for(int k = 0; k <= FILTER_ORDER; k++)
        {
            y_n += bandPassCoff[index][k] * bandPassBuffer[index][FILTER_ORDER - k];
        }
    }
    else
    {
        for(int k = 0; k <= FILTER_ORDER; k++)
        {
            y_n += notchCoff[index][k] * notchBuffer[index][FILTER_ORDER - k];
        }
    }
    return y_n;
}
