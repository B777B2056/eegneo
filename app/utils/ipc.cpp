#include "ipc.h"
#include <iostream>

namespace eegneo
{
    namespace utils
    {
        IpcClient::IpcClient()
            : mSid_(SessionId::Invalid), mChannel_(nullptr)
        {

        }

        IpcClient::IpcClient(IpcClient&& rhs)
        {
            
            *this = std::move(rhs);
        }

        IpcClient& IpcClient::operator=(IpcClient&& rhs)
        {
            if (this != &rhs)
            {
                mSid_ = rhs.mSid_;
                mChannel_ = rhs.mChannel_;
                mHandlers_ = std::move(rhs.mHandlers_);
                rhs.mSid_ = SessionId::Invalid;
                rhs.mChannel_ = nullptr;
            }
            return *this;
        }

        IpcClient::IpcClient(SessionId sid, const char* svrAddr, std::uint16_t svrPort)
            : mSid_(sid), mChannel_(new QTcpSocket())
        {
            QObject::connect(mChannel_, &QTcpSocket::connected, [this]()->void
            {
                QObject::connect(mChannel_, &QTcpSocket::readyRead, [this]()->void{ this->handleMsg(); });
                this->sendIdentifyInfo();
            });


            mChannel_->connectToHost(svrAddr, svrPort);
        }

        IpcClient::IpcClient(QTcpSocket* channel)
            : mSid_(SessionId::Invalid), mChannel_(channel)
        {
            QObject::connect(mChannel_, &QTcpSocket::readyRead, [this]()->void{ this->handleMsg(); });
        }

        IpcClient::~IpcClient()
        {
            if (mChannel_)  mChannel_->disconnectFromHost();
        }

        void IpcClient::sendIdentifyInfo()
        {
            CmdHeader cmdHdr;
            cmdHdr.sid = this->mSid_;
            cmdHdr.cid = CmdId::Init;
            char* buf = reinterpret_cast<char*>(&cmdHdr);
            std::int64_t bytesTransferred = 0;
            do
            {
                std::int64_t t = mChannel_->write(buf + bytesTransferred, sizeof(cmdHdr) - bytesTransferred);
                if (!t) break;
                bytesTransferred += t;
            } while (bytesTransferred < sizeof(cmdHdr));
            // mChannel_->waitForBytesWritten();
            mChannel_->flush();
        }

        void IpcClient::handleMsg()
        {
            if (!mChannel_->bytesAvailable())
            {
                return;
            }
            CmdHeader cmdHdr;
            if (!IpcClient::ReadBytes(mChannel_, (char*)&cmdHdr, sizeof(cmdHdr)) || (CmdId::Invalid == cmdHdr.cid))
            {
                return;
            }
            this->mSid_ = cmdHdr.sid;
            this->mHandlers_[cmdHdr.cid](mChannel_);
        }

        bool IpcClient::ReadBytes(QTcpSocket* channel, char* buf, std::uint16_t bytesLength)
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

        IpcService::IpcService(std::uint16_t svrPort)
        {
            QObject::connect(&mSvr_, &QTcpServer::newConnection, [this]()->void
            { 
                auto* clt = new IpcClient(mSvr_.nextPendingConnection());
                clt->mHandlers_[CmdId::Init] = [this, clt](QTcpSocket* channel)->void
                {
                    auto* cltProxy = this->mSessions_[clt->mSid_];
                    
                    for (auto&& [cmdid, cb] : cltProxy->mHandlers_)
                    {
                        clt->mHandlers_[cmdid] = cb;
                    }

                    this->mSessions_[clt->mSid_] = clt;
                    delete cltProxy;
                };
                mClients_.push_back(clt);
            });
            mSvr_.listen(QHostAddress::Any, svrPort);
        }

        IpcService::~IpcService()
        {
            mSvr_.close();
            for (auto* p : mClients_)
            {
                delete p;
            }
        }
    }   // namespace utils
}   // namespace eegneo