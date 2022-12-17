﻿#pragma once
#include <QMainWindow>
#include <QtCharts>
#include <QProcess>
#include <fstream>
#include <string>
#include <array>
#include <vector>
#include "acquisition/plotter.h"
#include "utils/ipc.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AcquisitionWindow; }
QT_END_NAMESPACE

#define GRAPH_FRESH 50  // 触发波形显示定时器的时间，单位为ms
#define MANUAL_MAKER 4 // 手动Mark数量

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
    void updateFFT();
    // 设置Y轴范围
    void setVoltageAxisScale(int curMaxVoltage);
    // 设置X轴范围
    void setTimeAxisScale(std::int8_t t);
    // 绘图初始化
    void initSignalChart();
    void initFFTChart();
    // 设置信号与槽的连接关系
    void connectSignalAndSlot();
    // 设置通道名称
    void setChannelName();
    // 数据文件保存
    void saveToEDFFormatFile();
    void saveToBDFFormatFile();

private:
    QString mFileName_;
    std::size_t mSampleRate_; 
    std::size_t mChannelNum_;
    QProcess mBackend_;
    // 绘图相关
    QTimer* mPlotTimer_ = nullptr;
    double* mSignalBuf_ = nullptr;
    QSharedMemory* mSharedMemory_ = nullptr;
    eegneo::EEGWavePlotter* mSignalChart_ = nullptr;
    eegneo::FFTWavePlotter* mFFTChart_ = nullptr;
    // 滤波相关
    eegneo::utils::IpcService* mIpcWrapper_ = nullptr;
    eegneo::RecordCmd mRecCmd_;
    eegneo::FiltCmd mFiltCmd_;
    // 文件保存有关
    int mFileSaveFinishedFlag_;
    QString mChannelNames_[64];
    // 视觉刺激实验中的图片总数量
    int _p300OddballImgNum;
    // UI
    Ui::AcquisitionWindow *ui;

signals:
    void closeAll();// 返回主界面
};
