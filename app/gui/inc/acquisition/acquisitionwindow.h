#pragma once
#include <QMainWindow>
#include <QtCharts>
#include <QProcess>
#include <fstream>
#include <string>
#include <array>
#include "acquisition/wave_plotter.h"
#include "utils/ipc.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AcquisitionWindow; }
QT_END_NAMESPACE

#define GRAPH_FRESH 50  // 触发波形显示定时器的时间，单位为ms
#define MANUAL_MAKER 4 // 手动Mark数量

class QTcpSocket;

/* 文件保存相关 */
struct FileInfo
{
    int eventCount = 0; // 事件数量计数器
    long long curLine = 0;  // 数据缓存txt当前行数（用于计算marker标记的时间）
    bool isRec = false;  // 是否开始记录数据
    bool isSaveP300BH = false;  // 是否保存P300行为学数据
    int isFinish = -1;  // EDF文件写入操作是否完成(-1:未开始保存edf文件，0:正在保存edf文件，1:保存完毕)
    QTime startTime;  // EDF文件开始记录数据的时间
    int flag;  // 创建EDF文件时的标志位
    std::ofstream samplesWrite;  // 8通道缓存txt文件输出流
    std::string tempFiles;  // 临时文件路径
    std::vector<std::string> channelNames;  // 通道名称
};

class AcquisitionWindow : public QMainWindow
{
    Q_OBJECT

public:
    AcquisitionWindow(QWidget *parent = nullptr);
    ~AcquisitionWindow();
    // 显示信息输入窗口并启动数据采集进程
    void start();

private:
    // 启动数据采集
    void startDataSampler();
    // 停止数据采集
    void stopDataSampler();
    // 创建Marker
    void createMark(const QString& event);
    // 更新波形显示
    void updateWave();
    // 设置Y轴范围
    void setVoltageAxisScale(int curMaxVoltage);
    // 设置X轴范围
    void setTimeAxisScale(std::int8_t t);
    // 绘图初始化
    void initChart();
    // 设置信号与槽的连接关系
    void connectSignalAndSlot();
    // 数据文件保存
    void setFilePath(int s, QString& path);
    // 保存行为学数据
    void saveBehavioralP300(const std::string path);

private:
    QString mFileName_;
    std::size_t mSampleRate_; 
    std::size_t mChannelNum_;
    QProcess mBackend_;
    // 绘图相关
    QTimer* mPlotTimer_;
    double* mBuf_;
    QSharedMemory* mSharedMemory_;
    eegneo::EEGWavePlotImpl* mChart_;
    // 滤波相关
    eegneo::utils::IpcWriter* mIpcWriter_;
    eegneo::RecordCmd mRecCmd_;
    eegneo::FiltCmd mFiltCmd_;
    // 文件保存有关
    FileInfo _fileInfo;
    // 视觉刺激实验中的图片总数量
    int _p300OddballImgNum;
    // UI
    Ui::AcquisitionWindow *ui;

    /* 信号 */
signals:
    void closeAll();// 返回主界面

    /* 槽 */
private slots:
    // 保存为EDF+文件
    void saveEdfPlus();
    // 保存为3个txt文档（样本数据点，事件信息，描述文档）
    void saveTxt();
    // oddball范式p300实验
    void p300Oddball();
};
