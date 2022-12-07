#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>
#include <QPainter>
#include <QTimer>
#include <QPointF>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mChart_(8)
    , mTimer_(new QTimer(this))
    , mSharedMemory_{"Sampler"}
{
    ui->setupUi(this);

    ui->graphicsView->setChart(mChart_.chart());                                   
    // ui->graphicsView->setRenderHint(QPainter::Antialiasing);   

    if (!mSharedMemory_.attach())
    {
        return;
    }
    mChart_.setAxisXScale(eegneo::Second::FIVE);
    mChart_.setAxisYScale(0, 80);
    QObject::connect(mTimer_, SIGNAL(timeout()), this, SLOT(update()));
    mTimer_->start(100);
}

MainWindow::~MainWindow()
{
    delete mTimer_;
    delete ui;
}

void MainWindow::update()
{
    if (!mSharedMemory_.lock()) return;
    ::memcpy(mBuf_.data(), mSharedMemory_.data(), mBuf_.size() * sizeof(double));
    if (!mSharedMemory_.unlock()) return;
    auto tmp = mBuf_;
    mChart_.update(tmp.data());
}
