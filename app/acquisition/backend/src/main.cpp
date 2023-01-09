#include <QCoreApplication>
#include <QTimer>
#include <string>
#include "backend.h"

int main(int argc, char* argv[])
{   
    QCoreApplication app(argc, argv);
    // 初始化backend
    eegneo::AcquisitionBackend backend{std::stoull(argv[1]), std::stoull(argv[2])};
    // 启动backend在主线程中的任务，进入事件循环
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&backend]{ backend.doTaskInMainThread(); });
    timer.start(0);
    return app.exec();
}
