#include "mainwindow.h"
#include <QProcess>
#include <QStringList>
#include <QApplication>

int main(int argc, char *argv[])
{
    QProcess dataSampler;
    QStringList args;
    args << "8";
    dataSampler.start("E:/jr/eegneo/build/sampler/Debug/eegneo_sampler.exe", args);
    if (!dataSampler.waitForStarted(-1))
    {
        return 1;
    }
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
