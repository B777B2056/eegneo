#include "eprime/eprimeconfigdialog.h"
#include "ui_eprimeconfigdialog.h"
#include <QSerialPortInfo>
#include <QMessageBox>

EPrimeConfigDialog::EPrimeConfigDialog(QWidget *parent)
    : QDialog(parent)
    , mEProxy_(std::make_shared<eegneo::erp::EPrimeProxy>())
    , ui(new Ui::EPrimeConfigDialog)
{
    ui->setupUi(this);

    for (auto& port : QSerialPortInfo::availablePorts())
    {
        ui->comboBox_5->addItem(port.portName());
    }

    QObject::connect(ui->comboBox, &QComboBox::textActivated, [this](const QString& text)->void
    {
        if ("-" == text)
        {
            QMessageBox::critical(this, tr("错误"), "波特率未设置", QMessageBox::Ok);
        }
        else
        {
            this->mEProxy_->serialPort().setBaudRate(text.toInt());
        }
    });

    QObject::connect(ui->comboBox_2, &QComboBox::textActivated, [this](const QString& text)->void
    {
        if ("-" == text)
        {
            QMessageBox::critical(this, tr("错误"), "数据位未设置", QMessageBox::Ok);
        }
        else
        {
            this->mEProxy_->serialPort().setDataBits(static_cast<QSerialPort::DataBits>(text.toInt()));
        }
    });

    QObject::connect(ui->comboBox_3, &QComboBox::activated, [this](int index)->void
    {
        switch (index)
        {
        case 0:
            QMessageBox::critical(this, tr("错误"), "奇偶校验未设置", QMessageBox::Ok);
            break;
        case 1:
            this->mEProxy_->serialPort().setParity(QSerialPort::NoParity);
            break;
        case 2:
            this->mEProxy_->serialPort().setParity(QSerialPort::EvenParity);
            break;
        case 3:
            this->mEProxy_->serialPort().setParity(QSerialPort::OddParity);
            break;
        case 4:
            this->mEProxy_->serialPort().setParity(QSerialPort::SpaceParity);
            break;
        case 5:
            this->mEProxy_->serialPort().setParity(QSerialPort::MarkParity);
            break;
        }
    });

    QObject::connect(ui->comboBox_4, &QComboBox::activated, [this](int index)->void
    {
        switch (index)
        {
        case 0:
            QMessageBox::critical(this, tr("错误"), "停止位未设置", QMessageBox::Ok);
            break;
        case 1:
            this->mEProxy_->serialPort().setStopBits(QSerialPort::StopBits::OneStop);
            break;
        case 2:
            this->mEProxy_->serialPort().setStopBits(QSerialPort::StopBits::OneAndHalfStop);
            break;
        case 3:
            this->mEProxy_->serialPort().setStopBits(QSerialPort::StopBits::TwoStop);
            break;
        }
    });

    QObject::connect(ui->comboBox_5, &QComboBox::textActivated, [this](const QString& text)->void
    {
        if ("-" == text)
        {
            QMessageBox::critical(this, tr("错误"), "COM端口未设置", QMessageBox::Ok);
        }
        else
        {
            this->mEProxy_->serialPort().setPortName(text);
        }
    });
}

EPrimeConfigDialog::~EPrimeConfigDialog()
{
    delete ui;
}
