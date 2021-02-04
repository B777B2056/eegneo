#ifndef PREPROCESSWINDOW_H
#define PREPROCESSWINDOW_H

#include <iostream>

#include <QMainWindow>
#include <QtCharts>
#include <QColor>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include "charthelp.h"
#include "setchannelname.h"

extern "C"
{
    #include "edflib.h"
}

#define SAMPLE_RATE 50  // 采样率

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

private slots:
    void on_pushButton_clicked();
    void readDataFromLocal();  // 从本程序缓存文件读取数据
    void readEDForBDF();  // 读取EDF/EDF+/BDF文件
//    void readCNT();  // 读取CNT格式文件
    void setChannelsName();

    void on_comboBox_2_currentIndexChanged(int index);

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_comboBox_currentIndexChanged(int index);

    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

private:
    int interval;  // 波形显示的区间的最大长度（单位为秒）
    int channelNum;  // 通道数
    int maxVotagle;  // 最大电压
    double start, end, jmp_start, jmp_end;  // 波形显示区间
    double allTime;
    bool hasOpen;  // 是否已经展示过波形
    bool isJmp;  // 是否进行区间跳转
    bool isOverlapping;  // 波形是否已经重叠
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

#endif // PREPROCESSWINDOW_H
