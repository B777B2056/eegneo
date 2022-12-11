#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <string>
#include <thread>
#include "sampler.h"
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
        eegneo::TestDataSampler sampler{channelNum};

        ipcReader.setCmdHandler<eegneo::RecordCmd>([&sampler](eegneo::RecordCmd* cmd)->void{ sampler.handleRecordCmd(cmd); });
        ipcReader.setCmdHandler<eegneo::FiltCmd>([&sampler](eegneo::FiltCmd* cmd)->void{ sampler.handleFiltCmd(cmd); });
        ipcReader.setCmdHandler<eegneo::ShutdownCmd>([&sampler](eegneo::ShutdownCmd* cmd)->void{ sampler.handleShutdownCmd(cmd); });
        ipcReader.setCmdHandler<eegneo::MarkerCmd>([&sampler](eegneo::MarkerCmd* cmd)->void{ sampler.handleMarkerCmd(cmd); });

        sampler.start();
    }};
    
    return app.exec();;
}
