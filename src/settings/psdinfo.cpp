#include "settings/psdinfo.h"
#include "ui_psdinfo.h"

PSDInfo::PSDInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PSDInfo)
{
    ui->setupUi(this);
}

PSDInfo::~PSDInfo()
{
    delete ui;
}

void PSDInfo::on_lineEdit_editingFinished()
{
    emit sendStartFreq(ui->lineEdit->text().toDouble());
}

void PSDInfo::on_lineEdit_2_editingFinished()
{
    emit sendStopFreq(ui->lineEdit_2->text().toDouble());
}

void PSDInfo::on_comboBox_currentIndexChanged(int index)
{
    if(index == 1)
        emit sendPSDType(Linear);
    if(index == 2)
        emit sendPSDType(Log);
}

void PSDInfo::on_lineEdit_3_editingFinished()
{
    emit sendStartTime(ui->lineEdit_3->text().toDouble());
}

void PSDInfo::on_lineEdit_4_editingFinished()
{
    emit sendStopTime(ui->lineEdit_4->text().toDouble());
}
