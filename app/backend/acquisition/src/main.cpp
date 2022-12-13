#include <QCoreApplication>
#include <QTimer>
#include <string>
#include <thread>
#include "backend.h"
#include "utils/ipc.h"


int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    // 初始化backend
    eegneo::AcquisitionBackend backend{std::stoull(argv[1])};
    // 初始化IPC读者，并设置接收到主进程发来的不同命令的回调
    eegneo::utils::IpcServer ipcReader;
    ipcReader.setCmdHandler<eegneo::RecordCmd>([&backend](eegneo::RecordCmd* cmd)->void{ backend.handleRecordCmd(cmd); });
    ipcReader.setCmdHandler<eegneo::FiltCmd>([&backend](eegneo::FiltCmd* cmd)->void{ backend.handleFiltCmd(cmd); });
    ipcReader.setCmdHandler<eegneo::ShutdownCmd>([&backend](eegneo::ShutdownCmd* cmd)->void{ backend.handleShutdownCmd(cmd); });
    ipcReader.setCmdHandler<eegneo::MarkerCmd>([&backend](eegneo::MarkerCmd* cmd)->void{ backend.handleMarkerCmd(cmd); });
    // 启动监听，等待主进程连接
    if (!ipcReader.start())
    {
        return 1;
    }
    // 启动backend处理过程，进入事件循环
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&backend]{ backend.run(); });
    timer.start();
    
    return app.exec();
}
