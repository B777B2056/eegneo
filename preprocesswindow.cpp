#include "preprocesswindow.h"
#include "ui_preprocesswindow.h"

PreprocessWindow::PreprocessWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreprocessWindow)
{
    ui->setupUi(this);
    connect(this, SIGNAL(returnMain()), parent, SLOT(goToMainWindow()));
}

PreprocessWindow::~PreprocessWindow()
{
    delete ui;
}

void PreprocessWindow::on_pushButton_clicked()
{
    emit returnMain();
}
