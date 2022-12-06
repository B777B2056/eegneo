#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "acquisition/acquisitionwindow.h"
#include "analysis/preprocesswindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow m;
    m.show();
    AcquisitionWindow acq;
    QObject::connect(&m, SIGNAL(MainWindow::covert2Accquisition()), &acq, SLOT(AcquisitionWindow::receiveJump2Accquisition()));
    PreprocessWindow pre;
    QObject::connect(&m, SIGNAL(MainWindow::covert2Analysis()), &pre, SLOT(PreprocessWindow::receiveJump2Analysis()));
    return a.exec();
}

