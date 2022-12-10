#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <string>
#include "sampler.h"


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
    
    eegneo::TestDataSampler sampler{std::stoull(argv[1]), ipcChannel};
    QObject::connect(ipcChannel, SIGNAL(readyRead()), &sampler, SLOT(handleIpcMsg()));
    
    return app.exec();;
}
