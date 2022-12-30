#pragma once
#include <deque>
#include <vector>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "common/common.h"

namespace eegneo
{
    namespace utils
    {
        class IpcClient
        {
        public:
            using ConnectedCallback = std::function<void()>;
            using ErrorCallback = std::function<void(QAbstractSocket::SocketError)>;

            template<typename Cmd> 
            using CmdHandler = std::function<void(Cmd*)>;

        public:
            IpcClient(SessionId sid, const char* svrAddr, std::uint16_t svrPort);
            IpcClient(QTcpSocket* channel);
            ~IpcClient();

            SessionId sessionId() const { return this->mSid_; }; 
            void setConnectedCallback(ConnectedCallback cb) { this->mConnectedCallback_ = cb; }
            void setErrorCallback(ErrorCallback cb) { this->mErrorCallback_ = cb; }

            template<typename Cmd>
            void setCmdHandler(CmdHandler<Cmd> handler)
            {
                this->mHandlers_[detail::CmdTypeMapToCmdId<Cmd>()] = [handler](QTcpSocket* channel)->void
                {
                    if (Cmd cmd; IpcClient::ReadBytes(channel, (char*)&cmd, sizeof(cmd)))   
                    {   
                        handler(&cmd);
                    }   
                };
            }

            template<typename Cmd>
            void sendCmd(const Cmd& cmd)
            {
                CmdHeader hdr{this->mSid_, detail::CmdTypeMapToCmdId<Cmd>()}; 

                std::vector<char> buf;
                buf.insert(buf.begin(), (char*)&hdr, (char*)&hdr + sizeof(hdr));
                buf.insert(buf.end(), (char*)&cmd, (char*)&cmd + sizeof(cmd));

                if (CmdId::Init == hdr.cid) this->mWriteQueue_.push_front(buf);
                else    this->mWriteQueue_.push_back(buf);
            }

        private:
            using CmdHandlerWrapper = std::function<void(QTcpSocket*)>;

        private:
            SessionId mSid_;
            QTcpSocket* mChannel_;
            std::unordered_map<CmdId, CmdHandlerWrapper> mHandlers_;
            QTimer* mEventLoopHook_;
            std::deque<std::vector<char>> mWriteQueue_;
            ConnectedCallback mConnectedCallback_;
            ErrorCallback mErrorCallback_;

            void handleRead();
            void handleWrite();
            static bool ReadBytes(QTcpSocket* channel, char* buf, std::uint16_t bytesLength);
        };

        class IpcService
        {
        public:
            using SessionHandler = std::function<void(IpcClient*)>;

        public:
            IpcService(std::uint16_t svrPort);
            ~IpcService();

            void setSessionHandler(SessionId sid, SessionHandler handler) { this->mSessionHandlers_[sid] = handler; }
            IpcClient* session(SessionId sid) { return this->mSessions_[sid]; }

        private:
            QTcpServer mSvr_;
            std::unordered_map<SessionId, IpcClient*> mSessions_;
            std::unordered_map<SessionId, SessionHandler> mSessionHandlers_;
        };
    }   // namespace utils
}   // namespace eegneo
