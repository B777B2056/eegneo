#pragma once
#include <string>
#include <vector>
#include <QMainWindow>
#include <QtCharts>
#include <QProcess>
#include "plotter.h"
#include "utils/ipc.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AcquisitionWindow; }
QT_END_NAMESPACE

class AcquisitionWindow : public QMainWindow
{
    Q_OBJECT

public:
    AcquisitionWindow(QWidget *parent = nullptr);
    ~AcquisitionWindow();
    // 显示信息输入窗口并启动数据采集进程
    void show();

protected:
    void showEvent(QShowEvent *event) override;

private:
    // 初始化ui
    void initUI();
    // 初始化IPC管理器
    void initIPCSvr();
    // 启动数据采集
    void startDataSampler();
    // 停止数据采集
    void stopDataSampler();
    // 创建Marker
    void createMark(const QString& event);
    // 更新波形显示
    void updateEEG();
    void updateFFT();
    void updateTopography();
    // 设置Y轴范围
    void setVoltageAxisScale(int curMaxVoltage);
    // 设置X轴范围
    void setTimeAxisScale(std::int8_t t);
    // 绘图初始化
    void initSignalChart();
    void initFFTChart();
    // 数据文件保存
    void saveToEDFFormatFile();
    void saveToBDFFormatFile();
    // 设置信号与槽的连接关系
    void connectSignalAndSlot();

private:
    QString mFileName_;
    std::size_t mSampleRate_; 
    std::size_t mChannelNum_;
    QProcess mBackend_;
    QSharedMemory* mSharedMemory_ = nullptr;
    // 绘图相关
    QTimer* mPlotTimer_ = nullptr;
    double* mSignalBuf_ = nullptr;
    eegneo::EEGWavePlotter* mSignalPlotter_ = nullptr;
    eegneo::FFTWavePlotter* mFFTPlotter_ = nullptr;
    eegneo::TopographyPlotter* mTopoPlotter_ = nullptr;
    // 滤波相关
    eegneo::utils::IpcService* mIpcWrapper_ = nullptr;
    eegneo::RecordCmd mRecCmd_;
    eegneo::FiltCmd mFiltCmd_;
    // 文件保存有关
    int mFileSaveFinishedFlag_;
    // UI
    Ui::AcquisitionWindow *ui;
};
