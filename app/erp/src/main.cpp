#include <QApplication>
#include "p300.h"

int main(int argc, char *argv[])
{   
    QApplication app(argc, argv);
    ErpP300OddballWindow m;
    m.show();
    return app.exec();
}