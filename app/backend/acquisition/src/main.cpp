#include <QCoreApplication>
#include <QTimer>
#include <string>
#include <thread>
#include "backend.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    // 初始化backend
    eegneo::AcquisitionBackend backend{std::stoull(argv[1])};
    // 启动backend处理过程，进入事件循环
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&backend]{ backend.run(); });
    timer.start();
    return app.exec();
}
