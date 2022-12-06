#include "settings/setchannelname.h"
#include "ui_setchannelname.h"

SetChannelName::SetChannelName(int channelNum, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::setchannelname)
{
    ui->setupUi(this);
    names = new QString[channelNum];
    if(channelNum == 8)
    {
        ui->label_9->setVisible(false);
        ui->label_10->setVisible(false);
        ui->label_11->setVisible(false);
        ui->label_12->setVisible(false);
        ui->label_13->setVisible(false);
        ui->label_14->setVisible(false);
        ui->label_15->setVisible(false);
        ui->label_16->setVisible(false);
        ui->lineEdit_9->setVisible(false);
        ui->lineEdit_9->setEnabled(false);
        ui->lineEdit_10->setVisible(false);
        ui->lineEdit_10->setEnabled(false);
        ui->lineEdit_11->setVisible(false);
        ui->lineEdit_11->setEnabled(false);
        ui->lineEdit_12->setVisible(false);
        ui->lineEdit_12->setEnabled(false);
        ui->lineEdit_13->setVisible(false);
        ui->lineEdit_13->setEnabled(false);
        ui->lineEdit_14->setVisible(false);
        ui->lineEdit_14->setEnabled(false);
        ui->lineEdit_15->setVisible(false);
        ui->lineEdit_15->setEnabled(false);
        ui->lineEdit_16->setVisible(false);
        ui->lineEdit_16->setEnabled(false);
    }
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

void SetChannelName::on_lineEdit_11_editingFinished()
{
    names[8] = ui->lineEdit_11->text();
}

void SetChannelName::on_lineEdit_15_editingFinished()
{
    names[9] = ui->lineEdit_15->text();
}

void SetChannelName::on_lineEdit_9_editingFinished()
{
    names[10] = ui->lineEdit_9->text();
}

void SetChannelName::on_lineEdit_13_editingFinished()
{
    names[11] = ui->lineEdit_13->text();
}

void SetChannelName::on_lineEdit_16_editingFinished()
{
    names[12] = ui->lineEdit_16->text();
}

void SetChannelName::on_lineEdit_14_editingFinished()
{
    names[13] = ui->lineEdit_14->text();
}

void SetChannelName::on_lineEdit_10_editingFinished()
{
    names[14] = ui->lineEdit_10->text();
}

void SetChannelName::on_lineEdit_12_editingFinished()
{
    names[15] = ui->lineEdit_12->text();
}
