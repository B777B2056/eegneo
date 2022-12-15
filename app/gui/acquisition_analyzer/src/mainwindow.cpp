#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->AccquisitionButton, &QPushButton::clicked, [this]()->void{ this->covert2AccquisitionImpl(); });
    QObject::connect(ui->AnalysisButton, &QPushButton::clicked, [this]()->void{ this->covert2AnalysisImpl(); });
    /*设置界面背景色*/
    QPalette palette(this->palette());
    palette.setColor(QPalette::Window, Qt::white);
    this->setPalette(palette);
    this->setWindowTitle("精神疾病患者EEG信号采集平台@被试编号");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::covert2AccquisitionImpl()
{
    this->hide();
    emit covert2Accquisition();
}

void MainWindow::covert2AnalysisImpl()
{
    this->hide();
    emit covert2Analysis();
}

void MainWindow::covert2ERPImpl()
{
    this->hide();
    emit covert2ERP();
}
