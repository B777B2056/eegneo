#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <string>
#include <thread>
#include "backend.h"
#include "utils/ipc.h"


int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    
    QTcpServer svr;
    if (!svr.listen(QHostAddress::LocalHost, eegneo::IPC_PORT))
    {
        return 1;
    }
    while (!svr.waitForNewConnection());
    auto* ipcChannel = svr.nextPendingConnection();

    eegneo::utils::IpcReader ipcReader{ipcChannel};

    std::size_t channelNum = std::stoull(argv[1]);
    std::thread processThread{[channelNum, &ipcReader]()->void
    {
        eegneo::AcquisitionBackend backend{channelNum};

        ipcReader.setCmdHandler<eegneo::RecordCmd>([&backend](eegneo::RecordCmd* cmd)->void{ backend.handleRecordCmd(cmd); });
        ipcReader.setCmdHandler<eegneo::FiltCmd>([&backend](eegneo::FiltCmd* cmd)->void{ backend.handleFiltCmd(cmd); });
        ipcReader.setCmdHandler<eegneo::ShutdownCmd>([&backend](eegneo::ShutdownCmd* cmd)->void{ backend.handleShutdownCmd(cmd); });
        ipcReader.setCmdHandler<eegneo::MarkerCmd>([&backend](eegneo::MarkerCmd* cmd)->void{ backend.handleMarkerCmd(cmd); });

        backend.start();
    }};
    
    return app.exec();;
}
