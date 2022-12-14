#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>


constexpr const char* DATA_FILE_PATH = "E:/jr/eegneo/temp_data.txt";
constexpr const char* EVENT_FILE_PATH = "E:/jr/eegneo/temp_event.txt";
namespace eegneo
{

#pragma  pack (push,1)

    enum class CmdId : std::uint8_t
    {
        Invalid = 0,
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
        CmdId id = CmdId::Invalid;
    };

    namespace detail { struct AbstractCmd {}; }

    struct RecordCmd : public detail::AbstractCmd
    {
        bool isRecordOn = false;
    };

    struct FiltCmd : public detail::AbstractCmd
    {
        bool isFiltOn = false;
        std::uint64_t sampleRate = 0;
        double lowCutoff = -1.0;  
        double highCutoff = -1.0; 
        double notchCutoff = -1.0; 

        bool isValid() const { return (sampleRate > 0) && ((lowCutoff > 0.0) || (highCutoff > 0.0) || (notchCutoff > 0.0)); }
    };

    struct ShutdownCmd : public detail::AbstractCmd
    {

    };

    struct MarkerCmd : public detail::AbstractCmd
    {
        char msg[1024] = {'\0'};
    };

    struct FileSaveCmd : public detail::AbstractCmd
    {
        std::uint64_t sampleRate;
        std::size_t channelNum;
        EDFFileType fileType;
        char filePath[1024] = {'\0'};
    };

    struct FileSavedFinishedCmd : public detail::AbstractCmd
    {

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
