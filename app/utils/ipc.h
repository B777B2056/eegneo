#pragma once
#include <functional>
#include <unordered_map>
#include <QTcpServer>
#include <QTcpSocket>
#include "common/common.h"

namespace eegneo
{
    namespace utils
    {
        class IpcWrapper
        {
        public:
            IpcWrapper();
            ~IpcWrapper();

            bool start();

            void setMainProcess() { this->mIsMainProcess_ = true; }

            template<typename Cmd>
            void setCmdHandler(std::function<void(Cmd*)> handler)
            {
                mHandlers_[detail::CmdType2Id<Cmd>()] = [handler](detail::AbstractCmd* baseCmd)->void
                {
                    handler(static_cast<Cmd*>(baseCmd));
                };
            }

            void sendIdentifyInfo(SessionId sid);

            template<typename Cmd>
            void sendCmd(SessionId sid, const Cmd& cmd)
            {
                CmdHeader hdr; 
                hdr.sid = sid;
                hdr.cid = detail::CmdType2Id<Cmd>();
                constexpr std::int64_t len = sizeof(hdr) + sizeof(cmd);
                char buf[len] = {0};
                ::memcpy(buf, (char*)&hdr, sizeof(hdr));
                ::memcpy(buf + sizeof(hdr), (char*)&cmd, sizeof(cmd));
                std::int64_t bytesTransferred = 0;
                auto* channel = (mClt_ ? mClt_ : mSessions_[sid]);
                do
                {
                    std::int64_t t = channel->write(buf + bytesTransferred, len - bytesTransferred);
                    if (!t) break;
                    bytesTransferred += t;
                } while (bytesTransferred < len);
                channel->waitForBytesWritten();
                channel->flush();
            }

        private:
            using CmdHandlerHashMap = std::unordered_map<CmdId, std::function<void(detail::AbstractCmd*)>>;

        private:
            bool mIsMainProcess_;
            QTcpServer* mSvr_;
            QTcpSocket* mClt_;
            CmdHandlerHashMap mHandlers_;
            std::unordered_map<SessionId, QTcpSocket*> mSessions_;

            void handleMsg(QTcpSocket* channel);
            bool readBytes(QTcpSocket* channel, char* buf, std::uint16_t bytesLength);
        };
    }   // namespace utils
}   // namespace eegneo
