#pragma once
#include <atomic>
#include <fstream>
#include <mutex>
#include <QSharedMemory>
#include "common/common.h"

namespace eegneo
{
    class EEGDataSampler;
    namespace utils { class Filter; }

    class AcquisitionBackend
    {
    public:
        AcquisitionBackend(std::size_t channelNum);
        AcquisitionBackend(const AcquisitionBackend&) = delete;
        AcquisitionBackend& operator=(const AcquisitionBackend&) = delete;
        AcquisitionBackend(AcquisitionBackend&&) = default;
        AcquisitionBackend& operator=(AcquisitionBackend&&) = default;
        virtual ~AcquisitionBackend();

        void start();

        void handleRecordCmd(RecordCmd* cmd);
        void handleFiltCmd(FiltCmd* cmd);
        void handleShutdownCmd(ShutdownCmd* cmd);
        void handleMarkerCmd(MarkerCmd* cmd);

    private:
        EEGDataSampler* mDataSampler_;
        std::size_t mChannelNum_;

        std::uint64_t mCurDataN_;
        std::fstream mDataFile_, mEventFile_;

        QSharedMemory mSharedMemory_;

        RecordCmd mRecCmd_;
        FiltCmd mFiltCmd_;
        
        std::mutex mMutex_;
        std::atomic_bool mIsStop_;

        utils::Filter* mFilter_;
        double* mFiltBuf_;

        void doSample();
        void doRecord();
        void doFilt();
    };
}   // namespace eegneo
