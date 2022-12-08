#pragma once
#include <QMainWindow>
#include <QtCharts>
#include <QColor>
#include <QQueue>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include "utils/charthelp.h"
#include "settings/setchannelname.h"
#include "settings/seteventchannel.h"
#include "acquisition/filter.h"
#include "settings/filtersetting.h"
#include "analysis/psd.h"
#include "analysis/wigner.h"
#include "settings/psdinfo.h"
#include "settings/dwtinfo.h"
#include "settings/wignerinfo.h"

extern "C"
{
    #include "third/edflib/edflib.h"
}
using namespace std;

namespace Ui {
class PreprocessWindow;
}

class PreprocessWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PreprocessWindow(QWidget *parent = nullptr);
    ~PreprocessWindow();

    QString tempFile;

signals:
    void returnMain();

public slots:
    void receiveFilterInfo(double, double, int);

private slots:
    // 跳转到当前窗口
    void receiveJump2Analysis() { this->show(); }

    void on_pushButton_clicked();
    void readDataFromLocal();  // 从本程序缓存文件读取数据
    void readEDF();  // 读取EDF/EDF+/BDF文件
    void readEEG();  // 读取CNT格式文件
    void setChannelsName();
    void filt();  // 滤波
    void getStartTimePSD(double);  // 获取功率谱起始时间
    void getStopTimePSD(double);  // 获取功率谱结束时间
    void plotPSD();  // 绘制功率谱估计曲线
    void getBeginTime(double);
    void getEndTime(double);
    void getChannel(QString);
    void plotWigner();  // 绘制单通道维格纳分布图
    void getFreqMin(double);
    void getFreqMax(double);
    void plotDWT();  // 绘制基于Morlet小波变换的时频图
    void on_comboBox_2_currentIndexChanged(int index);
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_comboBox_currentIndexChanged(int index);
    void on_lineEdit_editingFinished();
    void on_lineEdit_2_editingFinished();

private:
    int order;  // 滤波器阶数
    int interval;  // 波形显示的区间的最大长度（单位为秒）
    int channelNum;  // 通道数
    int maxVotagle;  // 最大电压
    int startTimePSD, stopTimePSD;  // 功率谱起始、结束时间
    int startTimeWigner, endTimeWigner;  // Wigner起始、终止时间
    int freqMin, freqMax;  // DWT所需的起始与终止频率
    double sampleFreq;  // 采样率
    double lowPass, highPass;  // 滤波器低、高通频率
    double start, end, jmp_start, jmp_end;  // 波形显示区间
    double allTime;
    bool hasOpen;  // 是否已经展示过波形
    bool isJmp;  // 是否进行区间跳转
    bool isOverlapping;  // 波形是否已经重叠
    QString filePath;  // 导入的文件路径
    QString channel;  // 待进行时频分析的通道名称
    PSDInfo *psi;
    PSD *p;
    WignerInfo *wi;
    Wigner *w;
    DwtInfo *di;
    std::string *channelsName;
    ChartHelp *help;  // 绘图帮助类，存放chartview
    std::vector<QSplineSeries *> series;
    QValueAxis *axisX;
    QCategoryAxis *axisY;
    QChart *chart;
    std::map<int, std::vector<QPointF>> samplePoints;  // 数据点
    std::map<std::string, QColor> markColors;  // 不同的mark直线对应的颜色
    std::vector<std::pair<std::string, double>> eventLines;  // 事件直线
    std::vector<QLineSeries *> lines;  // mark直线集合
    std::vector<QGraphicsSimpleTextItem *> pItems;  // 保存在QChartview上的Mrak名称
    void initChart(int markNum);  // 绘图初始化
    void paintChart();  // 绘图
    QColor getRadomColor(int baseR, int baseG, int baseB);  // 根据给定基数随机生成颜色

    Ui::PreprocessWindow *ui;
};
