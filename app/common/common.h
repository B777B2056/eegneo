#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <QTcpSocket>

namespace eegneo
{
    constexpr qint16 IPC_PORT = 8888;

#pragma  pack (push,1)

    enum class CmdId : std::uint8_t
    {
        Invalid = 0,
        Rec,
        Filt,
        Shutdown
    };

    struct CmdHeader
    {
        CmdId id = CmdId::Invalid;
    };

    struct RecordCmd
    {
        bool isRecordOn = false;
    };

    struct FiltCmd
    {
        bool isFiltOn = false;
        std::uint64_t sampleRate = 0;
        double lowCutoff = -1.0;  
        double highCutoff = -1.0; 
        double notchCutoff = -1.0; 
    };

    struct ShutdownCmd
    {

    };

    template<typename Cmd>
    void SendCmd(QTcpSocket* socket, const Cmd& cmd)
    {
        CmdHeader hdr; 
        if constexpr (std::is_same_v<Cmd, RecordCmd>)
        {
            hdr.id = CmdId::Rec;
        }
        else if constexpr (std::is_same_v<Cmd, FiltCmd>)
        {
            hdr.id = CmdId::Filt;
        }
        else if constexpr (std::is_same_v<Cmd, ShutdownCmd>)
        {
            hdr.id = CmdId::Shutdown;
        }
        else
        {
            hdr.id = CmdId::Invalid;
        }
        constexpr std::int64_t len = sizeof(hdr) + sizeof(cmd);
        char buf[len] = {0};
        ::memcpy(buf, (char*)&hdr, sizeof(hdr));
        ::memcpy(buf + sizeof(hdr), (char*)&cmd, sizeof(cmd));
        std::int64_t bytesTransferred = 0;
        do
        {
            std::int64_t t = socket->write(buf + bytesTransferred, len - bytesTransferred);
            if (!t) break;
            bytesTransferred += t;
        } while (bytesTransferred < len);
        socket->waitForBytesWritten();
        socket->flush();
    }

#pragma pack(pop)
}   // namespace eegneo
