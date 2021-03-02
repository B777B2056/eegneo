#include "seteventchannel.h"
#include "ui_seteventchannel.h"

SetEventChannel::SetEventChannel(QWidget *parent) :
    QDialog(parent),
    eventChannel(""), ui(new Ui::SetEventChannel)
{
    ui->setupUi(this);
}

SetEventChannel::~SetEventChannel()
{
    delete ui;
}

void SetEventChannel::on_lineEdit_editingFinished()
{
    eventChannel = ui->lineEdit->text();
}
