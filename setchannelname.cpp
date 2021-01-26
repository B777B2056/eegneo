#include "setchannelname.h"
#include "ui_setchannelname.h"

SetChannelName::SetChannelName(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::setchannelname)
{
    ui->setupUi(this);
    names = new QString[8];
}

SetChannelName::~SetChannelName()
{
    delete ui;
}

void SetChannelName::on_lineEdit_editingFinished()
{
    names[0] = ui->lineEdit->text();
}

void SetChannelName::on_lineEdit_2_editingFinished()
{
    names[1] = ui->lineEdit_2->text();
}

void SetChannelName::on_lineEdit_3_editingFinished()
{
    names[2] = ui->lineEdit_3->text();
}

void SetChannelName::on_lineEdit_4_editingFinished()
{
    names[3] = ui->lineEdit_4->text();
}

void SetChannelName::on_lineEdit_5_editingFinished()
{
    names[4] = ui->lineEdit_5->text();
}

void SetChannelName::on_lineEdit_6_editingFinished()
{
    names[5] = ui->lineEdit_6->text();
}

void SetChannelName::on_lineEdit_7_editingFinished()
{
    names[6] = ui->lineEdit_7->text();
}

void SetChannelName::on_lineEdit_8_editingFinished()
{
    names[7] = ui->lineEdit_8->text();
}
