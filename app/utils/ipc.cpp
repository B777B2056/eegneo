#include "ipc.h"
#include <iostream>

#define ReadAndHandler(CMDType) \
    do  \
    {   \
        if (CMDType##Cmd cmd; this->readBytes(channel, (char*)&cmd, sizeof(cmd)))   \
        {   \
            this->mHandlers_[cmdHdr.cid](&cmd);  \
        }   \
    }   \
    while (0)

namespace eegneo
{
    namespace utils
    {
        IpcWrapper::IpcWrapper() 
            : mIsMainProcess_(false), mSvr_(nullptr), mClt_(nullptr)
        {
            
        }

        IpcWrapper::~IpcWrapper()
        {
            if (mSvr_)
            {
                mSvr_->close();
                delete mSvr_;
            }
            if (mClt_)
            {
                mClt_->disconnectFromHost();
                delete mClt_;
            }
        }

        void IpcWrapper::sendIdentifyInfo(SessionId sid)
        {
            CmdHeader cmdHdr;
            cmdHdr.sid = sid;
            cmdHdr.cid = CmdId::Init;
            char* buf = reinterpret_cast<char*>(&cmdHdr);
            std::int64_t bytesTransferred = 0;
            do
            {
                std::int64_t t = mClt_->write(buf + bytesTransferred, sizeof(cmdHdr) - bytesTransferred);
                if (!t) break;
                bytesTransferred += t;
            } while (bytesTransferred < sizeof(cmdHdr));
            mClt_->waitForBytesWritten();
            mClt_->flush();
        }

        bool IpcWrapper::start()
        {
            if (this->mIsMainProcess_)
            {
                mSvr_ = new QTcpServer();
                QObject::connect(mSvr_, &QTcpServer::newConnection, [this]()->void
                { 
                    auto* channel = mSvr_->nextPendingConnection();
                    QObject::connect(channel, &QTcpSocket::readyRead, [this, channel]()->void{ this->handleMsg(channel); });
                });
                return mSvr_->listen(QHostAddress::Any, IPC_SVR_PORT);
            }
            else
            {
                mClt_ = new QTcpSocket();
                QObject::connect(mClt_, &QTcpSocket::connected, [this]()->void
                {
                    QObject::connect(mClt_, &QTcpSocket::readyRead, [this]()->void{ this->handleMsg(mClt_); });
                });
                mClt_->connectToHost(IPC_SVR_ADDR, IPC_SVR_PORT);
                return true;
            }
        }

        void IpcWrapper::handleMsg(QTcpSocket* channel)
        {
            if (!channel->bytesAvailable())
            {
                return;
            }
            CmdHeader cmdHdr;
            if (!this->readBytes(channel, (char*)&cmdHdr, sizeof(cmdHdr)) || (CmdId::Invalid == cmdHdr.cid))
            {
                return;
            }
            switch (cmdHdr.cid)
            {
            case CmdId::Init:
                this->mSessions_[cmdHdr.sid] = channel;
                break;
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

        bool IpcWrapper::readBytes(QTcpSocket* channel, char* buf, std::uint16_t bytesLength)
        {   
            std::uint16_t bytesReceived = 0;    
            do  
            {   
                int t = channel->read(buf + bytesReceived, bytesLength - bytesReceived);  
                if (-1 == t) return false;
                bytesReceived += t; 
            } while (bytesReceived < bytesLength);   
            return true;
        }
    }   // namespace utils
}   // namespace eegneo