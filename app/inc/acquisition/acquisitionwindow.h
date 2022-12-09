#pragma once
#include <QMainWindow>
#include <QtCharts>
#include <QProcess>
#include <fstream>
#include <string>
#include <array>
#include "acquisition/wave_plotter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AcquisitionWindow; }
QT_END_NAMESPACE

#define GRAPH_FRESH 50  // 触发波形显示定时器的时间，单位为ms
#define MANUAL_MAKER 4 // 手动Mark数量

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

private:
    // 开始数据采集与记录进程
    void startDataSampler();
    // 创建Marker
    void createMark(const QString& event);
    // 设置Y轴范围
    void setVoltageAxisScale(int curMaxVoltage);
    // 设置X轴范围
    void setTimeAxisScale(std::int8_t t);
    // 绘图初始化
    void initChart();
    // 显示被试信息输入窗口
    void showParticipantInfoWindow();
    // 设置信号与槽的连接关系
    void setSignalSlotConnect();
    // 数据文件保存
    void setFilePath(int s, QString& path);
    // 保存行为学数据
    void saveBehavioralP300(const std::string path);

private:
    QString mFileName_;
    std::size_t mSampleRate_; 
    std::size_t mChannelNum_;
    QProcess mDataSampler_;
    // 绘图相关
    QTimer* mPlotTimer_;
    double* mBuf_;
    QSharedMemory* mSharedMemory_;
    eegneo::EEGWavePlotImpl* mChart_;
    // 滤波相关
    bool mIsFiltOn_;
    double mLowCutoff_ = -1.0;  // 低通频率
    double mHighCutoff_ = -1.0; // 高通频率
    double mNotchCutoff_ = -1.0;    // 陷波频率
    // 文件保存有关
    FileInfo _fileInfo;
    // 视觉刺激实验中的图片总数量
    int _p300OddballImgNum;
    // UI
    Ui::AcquisitionWindow *ui;

    /* 信号 */
signals:
    void returnMain();// 返回主界面
    void doRec();    // 开始记录
    void writeEvent(std::string);   // 记录事件
    void doneRec(); // 停止记录

    /* 槽 */
private slots:
    // 跳转到当前窗口
    void receiveJump2Accquisition() { this->showParticipantInfoWindow(); }
    // 返回主界面
    void onPushButtonClicked();
    // Marker
    void onPushButton2Clicked();
    void onPushButton3Clicked();
    void onPushButton4Clicked();
    void onPushButton5Clicked();
    void onComboBoxCurrentTextChanged(const QString &text);
    void onComboBox2CurrentTextChanged(const QString &text);
    void onComboBox3CurrentTextChanged(const QString &text);
    // 触发滤波
    void onFilterClicked();
    // 接收P300-Oddball实验图片总数量
    void getImgNum(int n) { _p300OddballImgNum = n; }
    // 设置各电导电压范围（单位：uV）
    void setVoltage10() { this->setVoltageAxisScale(10); }
    void setVoltage25() { this->setVoltageAxisScale(25); }
    void setVoltage50() { this->setVoltageAxisScale(50); }
    void setVoltage100() { this->setVoltageAxisScale(100); }
    void setVoltage200() { this->setVoltageAxisScale(200); }
    void setVoltage500() { this->setVoltageAxisScale(500); }
    void setVoltage1000() { this->setVoltageAxisScale(1000); }
    // 设置时间轴范围（单位：s）
    void setTime1() { this->setTimeAxisScale(1); }
    void setTime5() { this->setTimeAxisScale(5); }
    void setTime10() { this->setTimeAxisScale(10); }
    // 更新波形显示
    void updateWave();
    // 停止写入数据并保存缓存txt文件
    void stopRec();
    // 保存为EDF+文件
    void saveEdfPlus();
    // 保存为3个txt文档（样本数据点，事件信息，描述文档）
    void saveTxt();
    // 创建缓存的TXT文件
    void createTempTXT();
    // oddball范式p300实验
    void p300Oddball();
};
