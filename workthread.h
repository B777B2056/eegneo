#ifndef WORKTHREAD_H
#define WORKTHREAD_H

#include <QTime>
#include <QTimer>
#include <QThread>
#include <QQueue>
//#include <QProcess>
#include <QUdpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <filter.h>
#include <iostream>
#include "qextserialport.h"
#include "enum.h"

#define FILTER_ORDER 51 // 滤波器阶数(只能为奇数)
//#define SAMPLE_RATE 1000  // 采样率
#define QESP_NO_PORTABILITY_WARN

/*数据滤波与记录线程*/
class DataProcessThread : public QThread
{
    Q_OBJECT
public:
    DataProcessThread(int channels_num, int sampleRate, BoardType b, QString c="");
    ~DataProcessThread();

protected:
    void run() override;

private:
    QTime qtime;
    int sp, channels_num, filt_flag, cnt = 0;
    const double cofe = 0.022351744455307063;
    bool isFilt, isRec;
    BoardType board;
    QTimer *d;
    QString com;
//    QProcess *process;
    QUdpSocket *client;
    std::ofstream samplesWrite;
    QextSerialPort *port;
    std::vector<double> data, filtData;
    std::vector<QQueue<double>> bandPassBuffer, notchBuffer;  // 滤波时各通道数据缓存区，长度为滤波器阶数
    std::vector<double *> bandPassCoff, notchCoff;  // FIR I型带通滤波器与陷波器冲激响应
    double conv(FilterType type, int index);
    void saveDataTEMP();
    void boardInit();
    QString getLocalIP();
    double _turnBytes2uV(char byte1, char byte2, char byte3);
    double _turnBytes2uV(unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4);

signals:
    void sendData(std::vector<double>);  // 发送数据至主线程
    void inFilt();  // 滤波数据已产生的信号

private slots:
    void processData();  // 处理数据
    void getDataFromShanxi();  // 获取实时更新的EEG数据(山西设备)
    void getDataFromShanghai();  // 获取实时更新的EEG数据(上海设备)

public slots:
    void startRec(std::string);
    void stopRec();
    void startFilt(int, int, int);
};

#endif // WORKTHREAD_H
