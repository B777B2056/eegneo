#pragma once
#include <functional>
#include <unordered_map>
#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
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

            template<typename Cmd>
            void sendCmd(const Cmd& cmd)
            {
                CmdHeader hdr; 
                hdr.id = detail::CmdType2Id<Cmd>();
                constexpr std::int64_t len = sizeof(hdr) + sizeof(cmd);
                char buf[len] = {0};
                ::memcpy(buf, (char*)&hdr, sizeof(hdr));
                ::memcpy(buf + sizeof(hdr), (char*)&cmd, sizeof(cmd));
                std::int64_t bytesTransferred = 0;
                do
                {
                    std::int64_t t = mChannelPtr_->write(buf + bytesTransferred, len - bytesTransferred);
                    if (!t) break;
                    bytesTransferred += t;
                } while (bytesTransferred < len);
                mChannelPtr_->waitForBytesWritten();
                mChannelPtr_->flush();
            }

        private:
            bool mIsMainProcess_;
            QLocalServer mSvr_;
            QLocalSocket* mChannelPtr_;
            std::unordered_map<CmdId, std::function<void(detail::AbstractCmd*)>> mHandlers_;

            void handleMsg();
            bool readBytes(char* buf, std::uint16_t bytesLength);
        };
    }   // namespace utils
}   // namespace eegneo
