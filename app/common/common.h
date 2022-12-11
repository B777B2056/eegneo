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
        Record,
        Filt,
        Shutdown,
        Marker
    };

    struct CmdHeader
    {
        CmdId id = CmdId::Invalid;
    };

    struct EmptyCmd {};

    struct RecordCmd : public EmptyCmd
    {
        bool isRecordOn = false;
    };

    struct FiltCmd : public EmptyCmd
    {
        bool isFiltOn = false;
        std::uint64_t sampleRate = 0;
        double lowCutoff = -1.0;  
        double highCutoff = -1.0; 
        double notchCutoff = -1.0; 

        bool isValid() const { return (sampleRate > 0) && ((lowCutoff > 0.0) || (highCutoff > 0.0) || (notchCutoff > 0.0)); }
    };

    struct ShutdownCmd : public EmptyCmd
    {

    };

    struct MarkerCmd : public EmptyCmd
    {
        char msg[1024];

        MarkerCmd() { ::memset(msg, 0, 1024); }
    };

#pragma pack(pop)

    namespace detail
    {
        template<typename Cmd>
        CmdId CmdType2Id()
        {
            if constexpr (std::is_same_v<Cmd, RecordCmd>)
            {
                return CmdId::Record;
            }
            else if constexpr (std::is_same_v<Cmd, FiltCmd>)
            {
                return CmdId::Filt;
            }
            else if constexpr (std::is_same_v<Cmd, ShutdownCmd>)
            {
                return CmdId::Shutdown;
            }
            else if constexpr (std::is_same_v<Cmd, MarkerCmd>)
            {
                return CmdId::Marker;
            }
            else
            {
                return CmdId::Invalid;
            }
        }
    }   // namespace detail
}   // namespace eegneo
