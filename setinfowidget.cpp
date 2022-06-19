#include "setinfowidget.h"
#include "ui_setinfowidget.h"
#include <QTextStream>

SetInfoWidget::SetInfoWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::SetInfoWidget)
{
    this->participant_num = "";
    this->date = "";
    ui->setupUi(this);
}

SetInfoWidget::~SetInfoWidget()
{
    delete ui;
}

void SetInfoWidget::on_confirm_clicked()
{
    if(!participant_num.isEmpty() && !date.isEmpty())
    {
        this->hide();
    }else{
        /*弹出警告：被试信息未填写*/
        return;
    }
}

void SetInfoWidget::on_num_textChanged()
{
    participant_num = ui->num->toPlainText();
}

void SetInfoWidget::on_date_textChanged()
{
    date = ui->date->toPlainText();
}

void SetInfoWidget::on_setOthers_textChanged()
{
    others = ui->setOthers->toPlainText();
}
