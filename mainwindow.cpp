#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    tempFiles(""),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    b = new Background(this);
    m = new AcquisitionWindow(this);
    p = new PreprocessWindow(this);
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

void MainWindow::getBasicInfo(QString a, QString b)
{
    this->participantNum = a;
    this->tempFiles = b;
    this->setWindowTitle("EEG信号采集平台@被试编号：" + participantNum);
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
    m->init();
    ui->stackedWidget->setCurrentWidget(m);
    ui->pushButton->setVisible(false);
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setVisible(false);
    ui->pushButton_2->setEnabled(false);
}

void MainWindow::on_pushButton_2_clicked()
{
    p->tempFile = tempFiles;
    ui->stackedWidget->setCurrentWidget(p);
    ui->pushButton->setVisible(false);
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setVisible(false);
    ui->pushButton_2->setEnabled(false);
}
