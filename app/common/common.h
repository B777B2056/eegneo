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
    };

    struct ShutdownCmd : public EmptyCmd
    {

    };

#pragma pack(pop)
}   // namespace eegneo
