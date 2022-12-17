#pragma once
#include <unordered_map>
#include <QTcpServer>
#include <QTcpSocket>
#include "common/common.h"

namespace eegneo
{
    namespace utils
    {
        class IpcService;

        class IpcClient
        {
            friend class IpcService;
        public:
            IpcClient(SessionId sid);
            IpcClient(IpcClient&& rhs);
            IpcClient& operator=(IpcClient&& rhs);
            ~IpcClient();

            template<typename Cmd>
            void setCmdHandler(std::function<void(Cmd*)> handler)
            {
                this->mHandlers_[detail::CmdType2Id<Cmd>()] = [handler](QTcpSocket* channel)->void
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
                CmdHeader hdr; 
                hdr.sid = this->mSid_;
                hdr.cid = detail::CmdType2Id<Cmd>();
                constexpr std::int64_t len = sizeof(hdr) + sizeof(cmd);
                char buf[len] = {0};
                ::memcpy(buf, (char*)&hdr, sizeof(hdr));
                ::memcpy(buf + sizeof(hdr), (char*)&cmd, sizeof(cmd));
                std::int64_t bytesTransferred = 0;
                do
                {
                    std::int64_t t = mChannel_->write(buf + bytesTransferred, len - bytesTransferred);
                    if (!t) break;
                    bytesTransferred += t;
                } while (bytesTransferred < len);
                // mChannel_->waitForBytesWritten();
                mChannel_->flush();
            }

        private:
            SessionId mSid_;
            QTcpSocket* mChannel_;
            detail::CmdCallBackMap mHandlers_;

            IpcClient();
            IpcClient(QTcpSocket* channel);
            void sendIdentifyInfo();
            void handleMsg();
            static bool ReadBytes(QTcpSocket* channel, char* buf, std::uint16_t bytesLength);
        };

        class IpcService
        {
        public:
            IpcService();
            ~IpcService();

            IpcClient* session(SessionId sid) { return this->mSessions_[sid]; }

            template<typename Cmd>
            void setCmdHandler(SessionId sid, std::function<void(Cmd*)> handler)
            {
                if (!this->mSessions_.contains(sid))
                {
                    this->mSessions_[sid] = new IpcClient();
                }
                this->mSessions_[sid]->setCmdHandler<Cmd>(handler);
            }

        private:
            QTcpServer mSvr_;
            std::vector<IpcClient*> mClients_;
            std::unordered_map<SessionId, IpcClient*> mSessions_;
        };
    }   // namespace utils
}   // namespace eegneo
