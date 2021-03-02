#include "filtersetting.h"
#include "ui_filtersetting.h"

filterSetting::filterSetting(QWidget *parent) :
    QDialog(parent),
    lowPass(-1.0), highPass(-1.0), order(31),
    ui(new Ui::filterSetting)
{
    ui->setupUi(this);
}

filterSetting::~filterSetting()
{
    delete ui;
}

void filterSetting::on_lineEdit_editingFinished()
{
    lowPass = ui->lineEdit->text().toDouble();
}

void filterSetting::on_lineEdit_2_editingFinished()
{
    highPass = ui->lineEdit_2->text().toDouble();
}

void filterSetting::on_lineEdit_3_editingFinished()
{
    order = ui->lineEdit_3->text().toInt();
}

void filterSetting::on_buttonBox_accepted()
{
    emit sendFilterSetting(lowPass, highPass, order);
}

void filterSetting::on_buttonBox_rejected()
{
    return;
}
