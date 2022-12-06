#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include "acquisition/sampler.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QChart;
class QValueAxis;
class QLineSeries;
class QTimer;

class TestDataSampler : public eegneo::DataSampler
{
public:
    using eegneo::DataSampler::DataSampler;

private:
    void run() override 
    {  
        for (;;)
        {
            for (int i = 0; i < mBuf_.size(); ++i)
            {
                mBuf_[i] = rand() % 10;
            }
        }
    }
};

class Chart
{
public:
    Chart();
    ~Chart();
    void update(double val);
    QChart* chart() { return mChart_; }

private:
    int mCntX_;
    QValueAxis* mAxisX_;
    QValueAxis* mAxisY_;
    QLineSeries* mLineSeries_;
    QChart* mChart_;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void update();

private:
    Ui::MainWindow *ui;
    Chart mChart_[5];
    QTimer* mTimer_;
    eegneo::DataSampler* mDataSampler_;
};
#endif // MAINWINDOW_H
