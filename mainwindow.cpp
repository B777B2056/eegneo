#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    participantNum(""),
    tempFiles(""),
    backgroundWindow(this),
    acquisitionWindow(this),
    preprocessWindow(this),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(onPushButtonClicked()));
    QObject::connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(onPushButton2Clicked()));
    /*显示可选界面*/
    ui->stackedWidget->addWidget(&backgroundWindow);
    ui->stackedWidget->addWidget(&acquisitionWindow);
    ui->stackedWidget->addWidget(&preprocessWindow);
    ui->stackedWidget->setCurrentWidget(&backgroundWindow);
    /*设置界面背景色*/
    QPalette palette(this->palette());
    palette.setColor(QPalette::Background, Qt::white);
    this->setPalette(palette);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getBasicInfo(QString participantNum, QString tempFiles)
{
    this->participantNum = participantNum;
    this->tempFiles = tempFiles;
    this->setWindowTitle("EEG信号采集平台@被试编号：" + participantNum);
}

void MainWindow::goToMainWindow()
{
    this->goToOtherWindow(&backgroundWindow, true);
}

void MainWindow::onPushButtonClicked()
{
    acquisitionWindow.start();
    this->goToOtherWindow(&acquisitionWindow, false);
}

void MainWindow::onPushButton2Clicked()
{
    preprocessWindow.tempFile = tempFiles;
    this->goToOtherWindow(&preprocessWindow, false);
}
