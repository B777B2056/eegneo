#include <QApplication>
#include "mainwindow.h"
#include "acquisition/acquisitionwindow.h"
#include "analysis/preprocesswindow.h"

int main(int argc, char *argv[])
{   
    QApplication a(argc, argv);
    MainWindow m;
    AcquisitionWindow acq(&m);
    QObject::connect(&m, SIGNAL(covert2Accquisition()), &acq, SLOT(receiveJump2Accquisition()));
    QObject::connect(&acq, SIGNAL(returnMain()), &m, SLOT(covert2InitWindowImpl()));
    PreprocessWindow pre(&m);
    QObject::connect(&pre, SIGNAL(returnMain()), &m, SLOT(covert2InitWindowImpl()));
    QObject::connect(&m, SIGNAL(covert2Analysis()), &pre, SLOT(receiveJump2Analysis()));
    m.show();
    return a.exec();
}

