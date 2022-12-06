#pragma once
#include <QMainWindow>
#include <QtCharts>
#include <QLabel>
#include <fstream>
#include <string>
#include <array>
#include "utils/enum.h"
#include "utils/charthelp.h"
#include "ui_acquisitionwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AcquisitionWindow; }
QT_END_NAMESPACE

#define NO_BOARD  // 没板子宏
#define GRAPH_FRESH 250  // 触发波形显示定时器的时间，单位为ms
#define IMPEDANCE_FRESH 2000  // 2s刷新一次阻抗
#define MANUAL_MAKER 4 // 手动Mark数量

class ChartHelp;
class DataProcessThread;

/* 板子信息 */
struct BoardInfo
{
    BoardType type;
    int sampleRate;
};

/* 被试信息 */
struct ParticipantInfo
{
    QString participantNum; // 被试编号
    QString date;   // 实验日期
    QString others; // 备注信息
    QString expName;    // 实验名称
};

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
    std::map<QLineSeries*, std::pair<qint64, QGraphicsSimpleTextItem*> > marks;     // key=marker绘图直线，value=（直线高度，文本信息）
    std::string markerNames[MANUAL_MAKER];  // 手动marker标记名称
};

/* 波形绘图相关信息 */
struct WaveDrawingInfo
{
    int maxVoltage = 50;
    int timeInterval = 5;  // 波形显示的时间长度，单位为s
    int threshold = 5000;  // 图上最多显示多少个数据点
    ChartHelp help;  // 绘图帮助界面，存放chartview
    std::vector<QQueue<QPointF> > pointQueue;   // 图像点队列
    QTimer graphTimer; //图像渲染定时器
    std::vector<QSplineSeries *> series;    // 曲线集合
    std::vector<QDateTimeAxis *> axisX; // 横轴集合，为时间
    std::vector<QValueAxis *> axisY;    // 纵轴集合，为电压
    std::vector<QChart *> charts;   // 绘图板集合
    QList<QPointF> mData;   // 数据点集合
    std::vector<double> graphData;  // 从数据处理线程接收到的电压值

    WaveDrawingInfo()
    {
        //设置图像更新周期，单位：毫秒
        graphTimer.setInterval(GRAPH_FRESH);
    }
};

/* 电极 */
enum class ChannelNum {EIGHT = 8, SIXTEEN = 16};

struct ElectrodesInfo
{
    ChannelNum channelNum; // 电极数量
    QChartView* montages[16];   // 电极阻抗绘图板
    std::vector<QLabel*> impDisplay;   // 阻抗label
    std::vector<int> impedance; // 存放各电极阻抗大小
    QTimer impTimer;   // 阻抗刷新定时器

    ElectrodesInfo(WaveDrawingInfo& waveInfo);
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
    void start();

    /* 信号 */
signals:
    // 向主界面发送基本信息(被试编号、缓存文件地址)
    void sendBasicInfo(QString, QString);
    // 返回主界面
    void returnMain();
    // 开始记录
    void doRec(std::string);
    // 记录事件
    void writeEvent(std::string);
    // 停止记录
    void doneRec();
    // 开始滤波
    void doFilt(int, int, int);

    /* 槽 */
public slots:
    // 创建Marker
    void createMark(std::string event);

private slots:
    // 跳转到当前窗口
    void receiveJump2Accquisition() { this->show(); }
    // 获取已输入的marker
    void onLineEditEditingFinished() { this->lineEditHelper<0>(); }
    void onLineEdit2EditingFinished() { this->lineEditHelper<1>(); }
    void onLineEdit3EditingFinished() { this->lineEditHelper<2>(); }
    void onLineEdit4EditingFinished() { this->lineEditHelper<3>(); }
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
    void receiveData(std::vector<double>);
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
#ifdef NO_BOARD
    // 获取各导阻抗，截断原始的double类型数据(板子没有阻抗测量功能之前用槽函数产生随机数代替)
    void getImpedanceFromBoard();
#endif
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
#ifndef NO_BOARD
    void getImpedanceFromBoard();  // 获取各导阻抗，截断原始的double类型数据(有板子以后在这声明)
#endif

private:
    // 设置Y轴范围
    template<int curMaxVoltage>
    void setVoltageHelper()
    {
        _waveDrawingInfo.maxVoltage = curMaxVoltage;
        for(auto&& chart : _waveDrawingInfo.charts)
        {
            chart->axisY()->setRange(-curMaxVoltage, curMaxVoltage);
        }
    }
    // 设置X轴范围
    template<int ti>
    void setTime()
    {
        _waveDrawingInfo.timeInterval = ti;
        _waveDrawingInfo.threshold = 1000 * _waveDrawingInfo.timeInterval;
    }
    // 获取输入的marker名称
    template<int N>
    void lineEditHelper()
    {
        std::array<decltype (ui->lineEdit), 4> lineEditUi = {ui->lineEdit, ui->lineEdit_2, ui->lineEdit_3, ui->lineEdit_4};
        if(!lineEditUi[N]->text().isEmpty())
        {
            _markerInfo.markerNames[N] = lineEditUi[N]->text().toStdString();
        }
    }
    // 显示被试信息输入窗口
    void showParticipantInfoWindow();
    // 显示串口选择窗口
    void showComChooseWindow();
    // 设置信号与槽的连接关系
    void setSignalSlotConnect();
    // 画板初始化
    void initChart();
    // 更新波形显示
    void updateWave();
    // 数据文件保存
    void setFilePath(int s, std::string& path);
    // 保存行为学数据
    void saveBehavioralP300(const std::string path);

private:
    // 数据获取、存储与滤波子线程
    DataProcessThread* _dpt;
    // 绘图相关
    WaveDrawingInfo _waveDrawingInfo;
    // 电极相关
    ElectrodesInfo _electrodes;
    // 板子相关
    BoardInfo _board;
    // 被试信息
    ParticipantInfo _participant;
    // 滤波相关
    FiltParam _filtParam;
    // Marker相关
    MarkerInfo _markerInfo;
    // 文件保存有关
    FileInfo _fileInfo;
    // 视觉刺激实验中的图片总数量
    int _p300OddballImgNum;
    // UI
    QWidget* _parent;
    Ui::AcquisitionWindow *ui;
};
