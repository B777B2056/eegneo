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
        IpcWrapper::IpcWrapper() 
            : mIsMainProcess_(false), mChannelPtr_(nullptr)
        {
            QObject::connect(&mSvr_, &QLocalServer::newConnection, [this]()->void
            { 
                mChannelPtr_ = mSvr_.nextPendingConnection();
                QObject::connect(mChannelPtr_, &QLocalSocket::readyRead, [this]()->void{ this->handleMsg(); });
            });
        }

        IpcWrapper::~IpcWrapper()
        {
            if (!mIsMainProcess_)
            {
                mChannelPtr_->disconnectFromServer();
                delete mChannelPtr_;
            }
            mSvr_.close();
        }

        bool IpcWrapper::start()
        {
            if (this->mIsMainProcess_)
            {
                return mSvr_.listen(IPC_NAME);
            }
            else
            {
                mChannelPtr_ = new QLocalSocket();
                mChannelPtr_->connectToServer(IPC_NAME);
                QObject::connect(mChannelPtr_, &QLocalSocket::readyRead, [this]()->void{ this->handleMsg(); });
                return mChannelPtr_->waitForConnected(-1);
            }
        }

        void IpcWrapper::handleMsg()
        {
            if (!mChannelPtr_->bytesAvailable())
            {
                return;
            }
            CmdHeader cmdHdr;
            if (!this->readBytes((char*)&cmdHdr, sizeof(cmdHdr)) || (CmdId::Invalid == cmdHdr.id))
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
            case CmdId::FileSave:
                ReadAndHandler(FileSave);
                break;
            case CmdId::FileSavedFinished:
                ReadAndHandler(FileSavedFinished);
                break;
            default:
                break;
            }
        }

        bool IpcWrapper::readBytes(char* buf, std::uint16_t bytesLength)
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
    }   // namespace utils
}   // namespace eegneo