#include "settings/choosecom.h"
#include "ui_choosecom.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

ChooseCom::ChooseCom(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseCom)
{
    ui->setupUi(this);
}

ChooseCom::~ChooseCom()
{
    delete ui;
}

void ChooseCom::getCom(QString& com)
{
    com = _com;
}

void ChooseCom::on_lineEdit_editingFinished()
{
    _com = "COM" + ui->lineEdit->text();
}
