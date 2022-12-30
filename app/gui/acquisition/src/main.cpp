#include <QApplication>
#include "acquisitionwindow.h"

int main(int argc, char *argv[])
{   
    QApplication app(argc, argv);
    (new AcquisitionWindow())->show();  // 不销毁，让操作系统自己回收
    return app.exec();
}
