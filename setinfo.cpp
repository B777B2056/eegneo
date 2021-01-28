#include "setinfo.h"
#include "ui_setinfo.h"

SetInfo::SetInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::set_info)
{
    ui->setupUi(this);
}

SetInfo::~SetInfo()
{
    delete ui;
}

void SetInfo::getInfo(QString& num_info, QString& date_info, QString& other_info, QString& exp_name, int& motange_num)
{
    num_info = this->num_info;
    date_info = this->date_info;
    other_info = this->other_info;
    exp_name = this->exp_name;
    motange_num = this->motange_num;
}

void SetInfo::on_num_editingFinished()
{
    num_info = ui->num->text();
}

void SetInfo::on_date_editingFinished()
{
    date_info = ui->date->text();
}

void SetInfo::on_others_editingFinished()
{
    other_info = ui->others->text();
}

void SetInfo::on_exp_name_editingFinished()
{
    exp_name = ui->exp_name->text();
}

void SetInfo::on_comboBox_currentIndexChanged(int index)
{
    motange_num = index * 8;
}
