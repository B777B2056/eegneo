#include "mainbackground.h"
#include "ui_mainbackground.h"

Background::Background(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Background)
{
    ui->setupUi(this);
    ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/Images/MainBackground.png"));
    ui->label->setScaledContents(true);
}

Background::~Background()
{
    delete ui;
}
