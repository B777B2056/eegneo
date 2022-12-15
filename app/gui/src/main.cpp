#include <QApplication>
#include "mainwindow.h"
#include "acquisition/acquisitionwindow.h"
#include "analysis/preprocesswindow.h"
#include "erp/p300.h"

int main(int argc, char *argv[])
{   
    QApplication app(argc, argv);
    MainWindow m;
    AcquisitionWindow acq(&m);
    QObject::connect(&m, &MainWindow::covert2Accquisition, [&acq]()->void{ acq.start(); });
    QObject::connect(&acq, &AcquisitionWindow::closeAll, [&app]()->void{ app.exit(); });
    PreprocessWindow pre(&m);
    QObject::connect(&pre, &PreprocessWindow::closeAll, [&app]()->void{ app.exit(); });
    QObject::connect(&m, &MainWindow::covert2Analysis, [&pre]()->void{ pre.show(); });
    m.show();
    return app.exec();
}

