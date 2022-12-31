#include "ipc.h"

namespace eegneo
{
    namespace utils
    {
        IpcClient::IpcClient(SessionId sid, const char* svrAddr, std::uint16_t svrPort)
            : mSid_(sid), mChannel_(new QTcpSocket()), mEventLoopHook_(new QTimer())
        {
            QObject::connect(mChannel_, &QTcpSocket::readyRead, [this]()->void{ this->handleRead(); });
            QObject::connect(mEventLoopHook_, &QTimer::timeout, [this]()->void{ this->handleWrite(); });
            QObject::connect(mChannel_, &QTcpSocket::errorOccurred, [this](QAbstractSocket::SocketError err)->void
            {
                if (this->mErrorCallback_)  this->mErrorCallback_(err);
            });
            QObject::connect(mChannel_, &QTcpSocket::connected, [this]()->void
            {
                this->sendCmd(InitCmd{});
                mEventLoopHook_->start(0);
                if (this->mConnectedCallback_)  this->mConnectedCallback_();
            });
            mChannel_->connectToHost(svrAddr, svrPort);
        }

        IpcClient::IpcClient(QTcpSocket* channel)
            : mSid_(SessionId::Invalid), mChannel_(channel), mEventLoopHook_(new QTimer())
        {
            QObject::connect(mChannel_, &QTcpSocket::readyRead, [this]()->void{ this->handleRead(); });
            QObject::connect(mEventLoopHook_, &QTimer::timeout, [this]()->void{ this->handleWrite(); });
            mEventLoopHook_->start(0);
        }

        IpcClient::~IpcClient()
        {
            mEventLoopHook_->stop();
            delete mEventLoopHook_;
            if (mChannel_)  mChannel_->disconnectFromHost();
        }

        void IpcClient::handleRead()
        {
            CmdHeader hdr;
            if ((mChannel_->bytesAvailable() < sizeof(hdr)) 
            || !IpcClient::ReadBytes(mChannel_, (char*)&hdr, sizeof(hdr))
            || (CmdId::Invalid == hdr.cid))
            {
                return;
            }
            if (SessionId::Invalid == this->mSid_)
            {
                this->mSid_ = hdr.sid;
            }
            if (this->mHandlers_.contains(hdr.cid))
            {
                this->mHandlers_[hdr.cid](mChannel_);
            }
        }

        void IpcClient::handleWrite()
        {
            if (this->mWriteQueue_.empty()) return;
            const auto& msg = this->mWriteQueue_.front();
            std::int64_t bytesTransferred = 0;
            do
            {
                std::int64_t t = this->mChannel_->write(msg.data() + bytesTransferred, msg.size() - bytesTransferred);
                if (!t) break;
                bytesTransferred += t;
            } while (bytesTransferred < msg.size());
            this->mChannel_->flush();
            this->mWriteQueue_.pop_front();
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
                clt->setCmdHandler<InitCmd>([this, clt](InitCmd*)->void
                {
                    // 保证一个session只有一个进程连上来，从而保证其他进程掉线重连
                    if (this->mSessions_.contains(clt->sessionId()))
                    {
                        auto* lastClt = this->mSessions_[clt->sessionId()];
                        delete lastClt; // 断开原本的连接
                    }
                    this->mSessions_[clt->sessionId()] = clt;
                    if (this->mSessionHandlers_.contains(clt->sessionId()))
                    {
                        this->mSessionHandlers_[clt->sessionId()](clt);
                    }
                });
            });
            mSvr_.listen(QHostAddress::Any, svrPort);
        }

        IpcService::~IpcService()
        {
            for (auto& [_, clt] : this->mSessions_)
            {
                delete clt;
            }
            mSvr_.close();
        }
    }   // namespace utils
}   // namespace eegneo