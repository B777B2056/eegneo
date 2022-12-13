#include "ipc.h"

#define ReadAndHandler(CMDType) \
    do  \
    {   \
        if (CMDType##Cmd cmd; this->readBytes((char*)&cmd, sizeof(cmd)))   \
        {   \
            this->mHandlers_[cmdHdr.id](&cmd);  \
        }   \
    }   \
    while (0)

namespace eegneo
{
    constexpr const char* IPC_NAME = "eegneo_ipc";

    namespace utils
    {
        IpcServer::IpcServer()  : mChannelPtr_(nullptr)
        {
            
        }

        IpcServer::~IpcServer()
        {
            mSvr_.close();
        }

        bool IpcServer::start()
        {
            if (mSvr_.listen(IPC_NAME))
            {
                if (mSvr_.waitForNewConnection(-1))
                {
                    mChannelPtr_ = mSvr_.nextPendingConnection();
                    QObject::connect(mChannelPtr_, &QLocalSocket::readyRead, [this]()->void{ this->handleMsg(); });
                    return true;
                }
            }
            return false;
        }

        void IpcServer::handleMsg()
        {
            if (!mChannelPtr_->bytesAvailable())
            {
                return;
            }
            CmdHeader cmdHdr;
            if (!this->readBytes((char*)&cmdHdr, sizeof(cmdHdr)))
            {
                return;
            }
            switch (cmdHdr.id)
            {
            case CmdId::Record:
                ReadAndHandler(Record);
                break;
            case CmdId::Filt:
                ReadAndHandler(Filt);
                break;
            case CmdId::Shutdown:
                ReadAndHandler(Shutdown);
                break;
            case CmdId::Marker:
                ReadAndHandler(Marker);
                break;
            default:
                break;
            }
        }

        bool IpcServer::readBytes(char* buf, std::uint16_t bytesLength)
        {   
            std::uint16_t bytesReceived = 0;    
            do  
            {   
                int t = mChannelPtr_->read(buf + bytesReceived, bytesLength - bytesReceived);  
                if (-1 == t) return false;
                bytesReceived += t; 
            } while (bytesReceived < bytesLength);   
            return true;
        }

        IpcClient::IpcClient()
        {

        }

        IpcClient::~IpcClient()
        {
            mChannel_.disconnectFromServer();
        }

        bool IpcClient::start()
        {
            mChannel_.connectToServer(IPC_NAME);
            if (mChannel_.waitForConnected(-1))
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }   // namespace utils
}   // namespace eegneo