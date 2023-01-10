#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>
#include <unordered_map>

constexpr const char* DATA_CACHE_FILE_PATH = "data.temp";    // 数据临时记录文件路径
constexpr const char* EVENT_CACHE_FILE_PATH = "event.temp";  // 事件临时记录文件路径

class QTcpSocket;

#define FiltType_NoFilt 0x00
#define FiltType_LowPass 0x01
#define FiltType_HighPass 0x02
#define FiltType_BandPass 0x04
#define FiltType_Notch 0x08

namespace eegneo
{

#pragma  pack (push,1)

    enum class SessionId : std::uint8_t
    {
        Invalid = 0,
        AccquisitionInnerSession = 1,
        ERPSession = 2
    };

    enum class CmdId : std::uint8_t
    {
        Invalid = 0,
        Init,
        Record,
        Filt,
        Shutdown,
        Marker,
        FileSave,
        FileSavedFinished,
        Error,
        TopoReady
    };

    enum EDFFileType : std::uint8_t
    {
        EDF = 1,
        BDF = 2
    };

    struct CmdHeader
    {
        SessionId sid = SessionId::Invalid;
        CmdId cid = CmdId::Invalid;
    };

    struct InitCmd
    {

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

        bool isValid() const 
        { 
            return (sampleRate > 0) && ((lowCutoff > 0.0) || (highCutoff > 0.0) || (notchCutoff > 0.0)); 
        }

        int type() const
        {
            int ret = FiltType_NoFilt;
            if ((lowCutoff >= 0.0) && (highCutoff < 0.0))   // 高通滤波
            {
                ret |= FiltType_HighPass;
            }
            else if ((lowCutoff < 0.0) && (highCutoff >= 0.0))  // 低通滤波
            {
                ret |= FiltType_LowPass;
            }
            else if ((lowCutoff >= 0.0) && (highCutoff >= 0.0)) // 带通滤波
            {
                if (lowCutoff < highCutoff)
                {
                    ret |= FiltType_BandPass;
                }
            }
            else    // 无滤波
            {

            }
            if (notchCutoff > 0.0)  // 陷波滤波（在上述滤波的基础上滤波）
            {
                ret |= FiltType_Notch;
            }
            return ret;
        }
    };

    struct ShutdownCmd
    {

    };

    struct MarkerCmd
    {
        char msg[1024] = {'\0'};
    };

    struct FileSaveCmd
    {
        std::uint64_t sampleRate;
        std::size_t channelNum;
        EDFFileType fileType;
        char filePath[1024] = {'\0'};
    };

    struct FileSavedFinishedCmd
    {

    };

    struct ErrorCmd
    {
        char errmsg[1024] = {'\0'};
    };

    struct TopoReadyCmd
    {

    };

#pragma pack(pop)
    namespace detail
    {
        template<typename Cmd>
        constexpr CmdId CmdTypeMapToCmdId()
        {
            if constexpr (std::is_same_v<Cmd, InitCmd>)
            {
                return CmdId::Init;
            }
            else if constexpr (std::is_same_v<Cmd, RecordCmd>)
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
            else if constexpr (std::is_same_v<Cmd, FileSaveCmd>)
            {
                return CmdId::FileSave;
            }
            else if constexpr (std::is_same_v<Cmd, FileSavedFinishedCmd>)
            {
                return CmdId::FileSavedFinished;
            }
            else if constexpr (std::is_same_v<Cmd, ErrorCmd>)
            {
                return CmdId::Error;
            }
            else if constexpr (std::is_same_v<Cmd, TopoReadyCmd>)
            {
                return CmdId::TopoReady;
            }
            else
            {
                return CmdId::Invalid;
            }
        }
    }   // namespace detail
}   // namespace eegneo
