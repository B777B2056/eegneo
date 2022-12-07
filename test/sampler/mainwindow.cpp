#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>
#include <QPainter>
#include <QTimer>
#include <QPointF>

#define AXIS_MAX_X 40
#define AXIS_MAX_Y 10

Chart::Chart()
    : mCntX_(0)
    , mAxisX_(new QValueAxis())
    , mAxisY_(new QValueAxis())
    , mLineSeries_(new QLineSeries())
    , mChart_(new QChart())
{
    mAxisX_->setTitleText("X");
    mAxisY_->setTitleText("Y");
    mAxisX_->setMin(0);
    mAxisY_->setMax(0);
    mAxisX_->setMax(AXIS_MAX_X);
    mAxisY_->setMax(AXIS_MAX_Y);

    mChart_->addAxis(mAxisY_, Qt::AlignLeft);                      // 将Y轴添加到图表上
    mChart_->addAxis(mAxisX_, Qt::AlignBottom);                    // 将X轴添加到图表上
    mChart_->addSeries(mLineSeries_);                              // 将曲线对象添加到图表上
    // mChart_->setAnimationOptions(QChart::SeriesAnimations);        // 动画：能使曲线绘制显示的更平滑，过渡效果更好看

    mLineSeries_->attachAxis(mAxisX_);                             // 曲线对象关联上X轴，此步骤必须在m_chart->addSeries之后
    mLineSeries_->attachAxis(mAxisY_);                             // 曲线对象关联上Y轴，此步骤必须在m_chart->addSeries之后

    mLineSeries_->setUseOpenGL(true);
}

Chart::~Chart()
{
    delete mAxisX_;
    delete mAxisY_;
    delete mLineSeries_;
    delete mChart_;
}

void Chart::update(double val)
{
    if(mCntX_ > AXIS_MAX_X)
    {
        mData_.pop_front();
        mChart_->axes(Qt::Horizontal).back()->setMin(mCntX_ - AXIS_MAX_X);
        mChart_->axes(Qt::Horizontal).back()->setMax(mCntX_); 
    }
    mData_.emplace_back(mCntX_, val);
    mLineSeries_->replace(mData_);
    mCntX_++;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mTimer_(new QTimer(this))
    , mDataSampler_(new TestDataSampler(5))
{
    ui->setupUi(this);

    ui->graphicsView->setChart(mChart_[0].chart());                          
    ui->graphicsView_2->setChart(mChart_[1].chart());                          
    ui->graphicsView_3->setChart(mChart_[2].chart());                          
    ui->graphicsView_4->setChart(mChart_[3].chart());                          
    ui->graphicsView_5->setChart(mChart_[4].chart());          
                    
    // ui->graphicsView_5->setRenderHint(QPainter::Antialiasing);    
    // ui->graphicsView->setRenderHint(QPainter::Antialiasing);   
    // ui->graphicsView_2->setRenderHint(QPainter::Antialiasing);
    // ui->graphicsView_3->setRenderHint(QPainter::Antialiasing);
    // ui->graphicsView_4->setRenderHint(QPainter::Antialiasing);

    QObject::connect(mTimer_, SIGNAL(timeout()), this, SLOT(update()));
    mTimer_->start(100);
}

MainWindow::~MainWindow()
{
    delete mTimer_;
    delete mDataSampler_;
    delete ui;
}

void MainWindow::update()
{
    for (int i = 0; i < 5; ++i)
    {
        mChart_[i].update(mDataSampler_->data(i));
    }
}
