#pragma once
#include <QMainWindow>
#include <QtCharts>
#include <QLabel>
#include <QProcess>
#include <fstream>
#include <string>
#include <array>
#include "acquisition/wave_plotter.h"
#include "utils/enum.h"
#include "utils/charthelp.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AcquisitionWindow; }
QT_END_NAMESPACE

#define GRAPH_FRESH 100  // 触发波形显示定时器的时间，单位为ms
#define MANUAL_MAKER 4 // 手动Mark数量

class ChartHelp;

/* 滤波频率 */
struct FiltParam
{
    double lowCut = -1.0;  // 低通频率
    double highCut = -1.0; // 高通频率
    double notchCut = -1.0;    // 陷波频率
};

/* Marker信息 */
struct MarkerInfo
{
    std::map<QLineSeries*, std::pair<qint64, QGraphicsSimpleTextItem*>> marks;     // key=marker绘图直线，value=（直线高度，文本信息）
    std::string markerNames[MANUAL_MAKER];  // 手动marker标记名称
};

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
    // 设置Y轴范围
    template<int curMaxVoltage>
    void setVoltageHelper()
    {
        // _waveDrawingInfo.maxVoltage = curMaxVoltage;
        // for(auto&& chart : _waveDrawingInfo.charts)
        // {
        //     chart->axisY()->setRange(-curMaxVoltage, curMaxVoltage);
        // }
    }
    // 设置X轴范围
    template<int ti>
    void setTime()
    {
        // _waveDrawingInfo.timeInterval = ti;
        // _waveDrawingInfo.threshold = 1000 * _waveDrawingInfo.timeInterval;
    }
    // 获取输入的marker名称
    void lineEditHelper(int N);
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
    FiltParam _filtParam;
    // Marker相关
    MarkerInfo _markerInfo;
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
public slots:
    // 创建Marker
    void createMark(std::string event);

private slots:
    // 跳转到当前窗口
    void receiveJump2Accquisition() { this->showParticipantInfoWindow(); }
    // 获取已输入的marker
    void onLineEditEditingFinished() { this->lineEditHelper(0); }
    void onLineEdit2EditingFinished() { this->lineEditHelper(1); }
    void onLineEdit3EditingFinished() { this->lineEditHelper(2); }
    void onLineEdit4EditingFinished() { this->lineEditHelper(3); }
    // 返回主界面
    void onPushButtonClicked();
    // Marker
    void onPushButton2Clicked() { createMark(_markerInfo.markerNames[0]); }
    void onPushButton3Clicked() { createMark(_markerInfo.markerNames[1]); }
    void onPushButton4Clicked() { createMark(_markerInfo.markerNames[2]); }
    void onPushButton5Clicked() { createMark(_markerInfo.markerNames[3]); }
    void onComboBoxCurrentIndexChanged(int index);
    void onComboBox2CurrentIndexChanged(int index);
    void onComboBox3CurrentIndexChanged(int index);
    // 触发滤波
    void onFilterClicked();
    // 接收P300-Oddball实验图片总数量
    void getImgNum(int n) { _p300OddballImgNum = n; }
    // 从子线程接收数据
    // void receiveData(std::vector<double>);
    // 进入滤波，置信号灯为绿色
    void setInFilt();
    // 设置各电导电压范围（单位：uV）
    void setVoltage10() { this->setVoltageHelper<10>(); }
    void setVoltage25() { this->setVoltageHelper<25>(); }
    void setVoltage50() { this->setVoltageHelper<50>(); }
    void setVoltage100() { this->setVoltageHelper<100>(); }
    void setVoltage200() { this->setVoltageHelper<200>(); }
    void setVoltage500() { this->setVoltageHelper<500>(); }
    void setVoltage1000() { this->setVoltageHelper<1000>(); }
    // 设置时间轴范围（单位：s）
    void setTime1() { this->setTime<1>(); }
    void setTime5() { this->setTime<5>(); }
    void setTime10() { this->setTime<10>(); }
    // 定时触发图像更新
    void graphFresh();
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
