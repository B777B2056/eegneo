#include "settings/p300oddballsetting.h"
#include "ui_p300oddballsetting.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

p300oddballsetting::p300oddballsetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::p300oddballsetting)
{
    ui->setupUi(this);
}

p300oddballsetting::~p300oddballsetting()
{
    delete ui;
}

void p300oddballsetting::on_lineEdit_editingFinished()
{
    try {
        int n = ui->lineEdit->text().toInt();
        this->num_img = n;
    } catch (...) {
        QMessageBox::critical(this, tr("错误"), "只能输入整数", QMessageBox::Ok);
    }
}

void p300oddballsetting::on_lineEdit_2_editingFinished()
{
    try {
        double n = ui->lineEdit_2->text().toDouble();
        this->freq_2 = n;
    } catch (...) {
        QMessageBox::critical(this, tr("错误"), "只能输入小数", QMessageBox::Ok);
    }
}

void p300oddballsetting::on_lineEdit_4_editingFinished()
{
    try {
        int n = ui->lineEdit_4->text().toInt();
        this->crossDurationTime = n;
    } catch (...) {
        QMessageBox::critical(this, tr("错误"), "只能输入整数", QMessageBox::Ok);
    }
}

void p300oddballsetting::on_lineEdit_5_editingFinished()
{
    try {
        int n = ui->lineEdit_5->text().toInt();
        this->numDurationTime = n;
    } catch (...) {
        QMessageBox::critical(this, tr("错误"), "只能输入整数", QMessageBox::Ok);
    }
}

void p300oddballsetting::on_lineEdit_6_editingFinished()
{
    try {
        int n = ui->lineEdit_6->text().toInt();
        this->blankDurationTimeDown = n;
    } catch (...) {
        QMessageBox::critical(this, tr("错误"), "只能输入整数", QMessageBox::Ok);
    }
}

void p300oddballsetting::on_lineEdit_3_editingFinished()
{
    try {
        int n = ui->lineEdit_3->text().toInt();
        this->blankDurationTimeUp = n;
    } catch (...) {
        QMessageBox::critical(this, tr("错误"), "只能输入整数", QMessageBox::Ok);
    }
}
