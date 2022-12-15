#include "utils/mainbackground.h"
#include "ui_mainbackground.h"

BackgroundWindow::BackgroundWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BackgroundWindow)
{
    ui->setupUi(this);
    ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/Images/MainBackground.png"));
    ui->label->setScaledContents(true);
}

BackgroundWindow::~BackgroundWindow()
{
    delete ui;
}
