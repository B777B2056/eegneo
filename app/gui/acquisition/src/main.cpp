#include <QApplication>
#include "acquisitionwindow.h"

int main(int argc, char *argv[])
{   
    QApplication app(argc, argv);
    AcquisitionWindow acq;
    QObject::connect(&acq, &AcquisitionWindow::closeAll, &acq, &AcquisitionWindow::close);
    acq.show();
    return app.exec();
}
