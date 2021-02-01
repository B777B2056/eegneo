#ifndef WORKTHREAD_H
#define WORKTHREAD_H

#include <QTimer>
#include <QThread>
#include <QQueue>
#include <vector>
#include <fstream>
#include <filter.h>

#define NO_BOARD  // 没板子宏

#define FILTER_ORDER 31 // 滤波器阶数(只能为奇数)
#define SAMPLE_RATE 50  // 采样率

/*滤波器枚举*/
enum filter
{BandPass, Notch};

class DataThread : public QThread
{
    Q_OBJECT
public:
    DataThread(int channels_num);
    ~DataThread();

protected:
    void run();

private:
    int msecCnt = 0;
    int channels_num;
    bool isFilt, isRec;
    std::ofstream samplesWrite;
    QTimer *dataTimer;
    std::vector<double> data, filtData;
    std::vector<QQueue<double>> bandPassBuffer, notchBuffer;  // 滤波时各通道数据缓存区，长度为滤波器阶数
    std::vector<double *> bandPassCoff, notchCoff;  // FIR I型带通滤波器与陷波器冲激响应
    double conv(filter type, int index);
    void saveDataTEMP();
#ifndef NO_BOARD
    void getDataFromBoard();  // 获取实时更新的EEG数据
#endif

signals:
    void sendData(std::vector<double>);  // 发送数据至主线程
    void inFilt();  // 滤波数据已产生的信号

#ifdef NO_BOARD
private slots:
    void generateData();  // 随机数产生随机数据
#endif

public slots:
    void startRec(std::string);
    void stopRec();
    void startFilt(int, int, int);
};

#endif // WORKTHREAD_H
