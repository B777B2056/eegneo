#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QString participantNum, QString date, QString others, QString expName, int cn, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tempFiles = participantNum+'_'+date+'_'+others;
    this->setWindowTitle("EEG信号采集平台@被试编号：" + participantNum);
    b = new Background(this);
    m = new AcquisitionWindow(participantNum, date, others, expName, cn, this);
    p = new PreprocessWindow(tempFiles, this);
    ui->stackedWidget->addWidget(b);
    ui->stackedWidget->addWidget(m);
    ui->stackedWidget->addWidget(p);
    ui->stackedWidget->setCurrentWidget(b);
    /*设置界面背景色*/
    QPalette palette(this->palette());
    palette.setColor(QPalette::Background, Qt::white);
    this->setPalette(palette);
}

MainWindow::~MainWindow()
{
    /*删除缓存文件*/
//    std::remove((tempFiles + "_samples.txt").toStdString().c_str());
//    std::remove((tempFiles + "_events.txt").toStdString().c_str());
    delete ui;
}

void MainWindow::goToMainWindow()
{
    ui->pushButton->setVisible(true);
    ui->pushButton->setEnabled(true);
    ui->pushButton_2->setVisible(true);
    ui->pushButton_2->setEnabled(true);
    ui->stackedWidget->setCurrentWidget(b);
}

void MainWindow::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(m);
    ui->pushButton->setVisible(false);
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setVisible(false);
    ui->pushButton_2->setEnabled(false);
}

void MainWindow::on_pushButton_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(p);
    ui->pushButton->setVisible(false);
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setVisible(false);
    ui->pushButton_2->setEnabled(false);
}
