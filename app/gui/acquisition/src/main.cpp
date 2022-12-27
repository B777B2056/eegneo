#include <QApplication>
#include "acquisitionwindow.h"

int main(int argc, char *argv[])
{   
    QApplication app(argc, argv);
    auto* acq = new AcquisitionWindow();
    QObject::connect(acq, &AcquisitionWindow::closeAll, [&app]()->void{ app.exit(); });
    acq->show();
    return app.exec();
}

