#include "analysis/charthelp.h"
#include "ui_charthelp.h"

ChartHelp::ChartHelp(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChartHelp)
{
    ui->setupUi(this);
}

ChartHelp::~ChartHelp()
{
    delete ui;
}
