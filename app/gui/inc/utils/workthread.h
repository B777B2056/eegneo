﻿// #pragma once
// #include <QTime>
// #include <QTimer>
// #include <QThread>
// #include <QQueue>
// //#include <QProcess>
// #include <QPointF>
// #include <QUdpSocket>
// #include <QHostAddress>
// #include <QHostInfo>
// #include <QMutex>
// #include <vector>
// #include <cmath>
// #include <string>
// #include <fstream>
// #include <deque>
// #include <thread>
// #include <mutex>
// #include "filter.h"
// #include "qextserialport.h"
// #include "acquisition/utils/enum.h"

// #define FILTER_ORDER 51 // 滤波器阶数(只能为奇数)
// //#define SAMPLE_RATE 1000  // 采样率
// #define QESP_NO_PORTABILITY_WARN

// // 数据滤波与记录线程
// class DataProcessThread : public QThread
// {
//     Q_OBJECT
// public:
//     DataProcessThread(int channels_num, int sampleRate, BoardType b, QString c="");
//     virtual ~DataProcessThread() override;

// protected:
//     void run() override;

// private:
//     QMutex _mutex;
//     int sp, channels_num, filt_flag, cnt;
//     std::vector<double> sum;
//     const double cofe;
//     bool isFilt, isRec, isUseOther;
//     BoardType board;
//     QString com;
//     QUdpSocket *client;
//     std::ofstream samplesWrite;
//     std::ofstream eventsWrite;  // 标记缓存txt文件输出流
//     QextSerialPort *port;
//     std::vector<double> data, averData, filtData;
//     std::vector<QQueue<double> > bandPassBuffer, notchBuffer;  // 滤波时各通道数据缓存区，长度为滤波器阶数
//     std::vector<QQueue<QPointF> > averBuffer;
//     double *bandPassCoff, *notchCoff;  // FIR I型带通滤波器与陷波器冲激响应
//     void processData();  // 处理数据（滤波）
//     double conv(FilterType type, std::size_t index);
//     void boardInit();
//     double _turnBytes2uV(char byte1, char byte2, char byte3);
//     double _turnBytes2uV(unsigned char *bytes);

// signals:
//     void sendData(std::vector<double>);  // 发送数据至主线程
//     void inFilt();  // 滤波数据已经得到

// private slots:
//     void getDataFromShanxi();  // 获取实时更新的EEG数据(山西设备)
//     void getDataFromShanghai();  // 获取实时更新的EEG数据(上海设备)

// public slots:
//     void startRec(std::string);
//     void doEvent(std::string);
//     void stopRec();
//     void startFilt(int, int, int);
// };