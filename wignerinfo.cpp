#include "wignerinfo.h"
#include "ui_wignerinfo.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

WignerInfo::WignerInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WignerInfo)
{
    ui->setupUi(this);
}

WignerInfo::~WignerInfo()
{
    delete ui;
}

void WignerInfo::on_lineEdit_editingFinished()
{
    try
    {
        emit sendBeginTime(ui->lineEdit->text().toDouble());
    }
    catch (...)
    {
        QMessageBox::critical(this, this->tr("错误"), "所需信息输入错误", QMessageBox::Ok);
    }
}

void WignerInfo::on_lineEdit_2_editingFinished()
{
    try
    {
        emit sendEndTime(ui->lineEdit_2->text().toDouble());
    }
    catch (...)
    {
        QMessageBox::critical(this, this->tr("错误"), "所需信息输入错误", QMessageBox::Ok);
    }
}

void WignerInfo::on_lineEdit_3_editingFinished()
{
    emit sendChannelName(ui->lineEdit_3->text());
}
