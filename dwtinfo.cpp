#include "dwtinfo.h"
#include "ui_dwtinfo.h"

DwtInfo::DwtInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DwtInfo)
{
    ui->setupUi(this);
}

DwtInfo::~DwtInfo()
{
    delete ui;
}

void DwtInfo::on_lineEdit_editingFinished()
{
    try
    {
        emit sendFreqMin(ui->lineEdit->text().toDouble());
    }
    catch (...)
    {
        QMessageBox::critical(this, this->tr("错误"), "所需信息输入错误！", QMessageBox::Ok);
    }
}

void DwtInfo::on_lineEdit_2_editingFinished()
{
    try
    {
        emit sendFreqMax(ui->lineEdit_2->text().toDouble());
    }
    catch (...)
    {
        QMessageBox::critical(this, this->tr("错误"), "所需信息输入错误！", QMessageBox::Ok);
    }
}

void DwtInfo::on_lineEdit_3_editingFinished()
{
    emit sendChannel(ui->lineEdit_3->text());
}
