#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>
#include <unordered_map>

constexpr const std::uint16_t IPC_SVR_PORT = 8888;  // IPC服务器端口
constexpr const char* IPC_SVR_ADDR = "127.0.0.1";   // IPC服务器IP地址
constexpr const char* DATA_FILE_PATH = "E:/jr/eegneo/temp_data.txt";    // 数据临时记录文件路径
constexpr const char* EVENT_FILE_PATH = "E:/jr/eegneo/temp_event.txt";  // 事件临时记录文件路径
constexpr const char* BACKEND_PATH = "E:/jr/eegneo/build/app/backend/acquisition/Debug/eegneo_sampler.exe"; // 采样窗口后端进程路径

class QTcpSocket;

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
        FileSavedFinished
    };

    enum EDFFileType
    {
        EDF = 1,
        BDF = 2
    };

    struct CmdHeader
    {
        SessionId sid = SessionId::Invalid;
        CmdId cid = CmdId::Invalid;
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

        bool isValid() const { return (sampleRate > 0) && ((lowCutoff > 0.0) || (highCutoff > 0.0) || (notchCutoff > 0.0)); }
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

#pragma pack(pop)
    namespace detail
    {
        using CmdCallBackMap = std::unordered_map<CmdId, std::function<void(QTcpSocket*)>>;

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
            else if constexpr (std::is_same_v<Cmd, FileSaveCmd>)
            {
                return CmdId::FileSave;
            }
            else if constexpr (std::is_same_v<Cmd, FileSavedFinishedCmd>)
            {
                return CmdId::FileSavedFinished;
            }
            else
            {
                return CmdId::Invalid;
            }
        }
    }   // namespace detail
}   // namespace eegneo
