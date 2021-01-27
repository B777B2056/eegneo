#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QtCharts>
#include <QQueue>
#include <QCloseEvent>
#include <QFileDialog>
#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include "filter.h"
#include "setinfo.h"
#include "setchannelname.h"
#include "p300.h"

extern "C"
{
    #include "edflib.h"
}

using namespace QtCharts;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define NO_BOARD  // 没板子宏

#define CHANNELS 8  // 数据通道为8通道
#define SAMPLE_RATE 50  // 采样率
#define MAX_VOLTAGE 50.0  // EDF文件各通道最大电压值
#define MIN_VOLTAGE -10.0  // EDF文件各通道最小电压值
#define TIME_INTERVAL 5  // 波形显示的时间间隔，单位为s
#define GRAPH_FRESH 50  // 触发波形显示定时器的时间，单位为ms
#define DATA_FRESH 20 // 触发数据更新定时器的时间，单位为ms
#define IMPEDANCE_FRESH 2000  // 2s刷新一次阻抗
#define MANUAL_MAKER 8 // 手动Mark数量
#define FILTER_ORDER 31 // 滤波器阶数(只能为奇数)

/*颜色枚举*/
enum background
{Green, Yellow, Red};

/*滤波器枚举*/
enum filt
{BandPass, Notch};

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
     void closeEvent(QCloseEvent *event);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /*设置被试基本信息*/
    void setInfo();

public slots:
    void createMark(const std::string event);  // 创建Marker

private slots:
    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_lineEdit_3_editingFinished();

    void on_lineEdit_4_editingFinished();

    void on_lineEdit_5_editingFinished();

    void on_lineEdit_6_editingFinished();

    void on_lineEdit_7_editingFinished();

    void on_lineEdit_8_editingFinished();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();

    void on_comboBox_currentIndexChanged(int index);

    void on_comboBox_2_currentIndexChanged(int index);

    void on_comboBox_3_currentIndexChanged(int index);

    void setVoltage50();  // 设置各电导电压范围(-50~50uV)

    void setVoltage100();  // 设置各电导电压范围(-100~100uV)

    void setVoltage200();  // 设置各电导电压范围(-200~200uV)

    void on_filter_clicked();  // 触发滤波

    void graphFresh();  // 触发图像更新

#ifdef NO_BOARD
    void getDataFromBoard();  // 获取实时更新的EEG数据

    void getImpedanceFromBoard();  // 获取各导阻抗，截断原始的double类型数据(没有板子之前用槽函数产生随机数代替)
#endif

    void stopRec();  // 停止写入数据并保存缓存txt文件

    void saveEDF(); // 保存为EDF+文件

    void saveTXT();  // 保存为3个txt文档（样本数据点，事件信息，描述文档）

    void createTempTXT();  // 创建缓存的TXT文件

    void p300Oddball();  // oddball范式p300实验

private:
#ifdef NO_BOARD
    /*正弦波测试*/
    int msecCnt = 0;
    /*测试数据更新定时器*/
    QTimer *dataTimer;
#endif

    /*与被试信息相关的私有成员*/
    QString participantNum, date, others, expName;  // 被试信息

    /*与阻抗测量相关的私有成员*/
    int *impedance;
    QTimer *impTimer;

    /*与滤波相关的私有成员*/
    bool isFilt;  // 是否进行滤波
    double lowCut, highCut, notchCut;  // 被选好的高通、低通滤波、凹陷滤波频率
    double highPassFres[7] = {0.1, 0.3, 3.5, 8.0, 12.5, 16.5, 20.5};  // 高通滤波频率选择
    double lowPassFres[7] = {8.0, 12.5, 16.5, 20.5, 28.0, 45.0, 50.0};  // 低通滤波频率选择
    std::array<QQueue<double>, CHANNELS> bandPassBuffer, notchBuffer;  // 滤波时8通道数据缓存区，长度为滤波器阶数
    std::array<double *, CHANNELS> bandPassCoff, notchCoff;  // FIR I型带通滤波器与陷波器冲激响应
    double *filtData;  // 滤波后输入图像的数据
    double conv(filt type, int index);  // 卷积计算

    /*与Marker相关的私有成员*/
    std::map<QLineSeries *, std::pair<qint64, QGraphicsSimpleTextItem *>> marks;
    std::string *markerNames;  // 手动marker标记名称

    /*与绘图相关的私有成员*/
    int maxVoltage;
    const int threshold;  // 图上最多显示多少个数据点
    QQueue<QPointF> *pointQueue;
    QTimer *graphTimer; //图形渲染定时器
    QSplineSeries *series;
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    QChart *charts;
    void initChart();  // 绘图初始化
    void updateWave(double *channelData);  //更新波形

    /*与数据获取有关的私有成员*/
    double *originalData;  // 8个通道的原始数据
#ifndef NO_BOARD
    void getDataFromBoard();  // 从单片机获取数据
    void getImpedanceFromBoard();  // 获取各导阻抗，截断原始的double类型数据为int
#endif

    /*与文件保存有关的私有成员*/
    int eventCount;
    long long curLine;  // 数据缓存txt当前行数（用于计算marker标记的时间）
    bool isRec;  // 是否开始记录数据
    bool isSaveP300BH;  // 是否保存P300行为学数据
    int isFinish;  // EDF文件写入操作是否完成(-1:未开始保存edf文件，0:正在保存edf文件，1:保存完毕)
    QTime startTime;  // EDF文件开始记录数据的时间
    int flag;  // 创建EDF文件时的标志位
    int bufCnt[CHANNELS] = {0};  // 数据缓存区计数器
    std::ofstream samplesWrite;  // 8通道缓存txt文件输出流
    std::ofstream eventsWrite;  // 标记缓存txt文件输出流
    std::string tempFiles;
    std::string *channelNames;  // 通道名称
    void setFilePath(int s, std::string path);  // 设置文件保存的路径
    void saveBehavioralP300(std::string path);  // 保存行为学数据

    /*UI*/
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
