#pragma once
#include <functional>
#include <unordered_map>
#include <QObject>
#include <QTcpSocket>
#include "common/common.h"

namespace eegneo
{
    namespace utils
    {
        class IpcReader : public QObject
        {
            Q_OBJECT

        public:
            IpcReader(QTcpSocket* channel);

            template<typename Cmd>
            void setCmdHandler(std::function<void(Cmd*)> handler)
            {
                mHandlers_[detail::CmdType2Id<Cmd>()] = [handler](EmptyCmd* baseCmd)->void
                {
                    handler(static_cast<Cmd*>(baseCmd));
                };
            }

        private:
            QTcpSocket* mChannel_;
            std::unordered_map<CmdId, std::function<void(EmptyCmd*)>> mHandlers_;

            void handleMsg();
            bool readBytes(char* buf, std::uint16_t bytesLength);
        };

        class IpcWriter
        {
        public:
            IpcWriter(QTcpSocket* channel);

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
                    std::int64_t t = mChannel_->write(buf + bytesTransferred, len - bytesTransferred);
                    if (!t) break;
                    bytesTransferred += t;
                } while (bytesTransferred < len);
                mChannel_->waitForBytesWritten();
                mChannel_->flush();
            }

        private:
            QTcpSocket* mChannel_;
        };
    }   // namespace utils
}   // namespace eegneo
