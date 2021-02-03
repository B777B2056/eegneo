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

#define SAMPLE_RATE 50  // 采样率

namespace Ui {
class PreprocessWindow;
}

class PreprocessWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PreprocessWindow(QString tempFile, QWidget *parent = nullptr);
    ~PreprocessWindow();

signals:
    void returnMain();

private slots:
    void on_pushButton_clicked();
    void readDataFromLocal();  // 从本程序缓存文件读取数据
//    void readEDFPlus();  // 读取EDF+文件
//    void readBDF();  // 读取BDF文件
//    void readCNT();  // 读取CNT格式文件

    void on_comboBox_2_currentIndexChanged(int index);

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

private:
    int channelNum;  // 通道数
    int maxVotagle;  // 最大电压
    bool isOverlapping;  // 波形是否已经重叠
    QString tempFile;
    ChartHelp *help;  // 绘图帮助类，存放chartview
    std::vector<QSplineSeries *> series;
    QValueAxis *axisX;
    QCategoryAxis *axisY, *axisMark;
    QChart *chart;
    std::map<int, std::vector<QPointF>> samplePoints;  // 数据点
    std::map<std::string, QColor> markColors;  // 不同的mark直线对应的颜色
    std::vector<std::pair<std::string, long long>> eventLines;  // 事件直线
    void initChart(int channelNum, int allTimes);  // 绘图初始化
    void paintChart();  // 绘图
    QColor getRadomColor(int baseR, int baseG, int baseB);  // 根据给定基数随机生成颜色

    Ui::PreprocessWindow *ui;
};

#endif // PREPROCESSWINDOW_H
