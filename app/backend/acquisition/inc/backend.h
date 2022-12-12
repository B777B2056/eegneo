#pragma once
#include <fstream>
#include <QSharedMemory>
#include "common/common.h"

class QTimer;

namespace eegneo
{
    class EEGDataSampler;
    namespace utils { class Filter; class FFTCalculator; }

    class AcquisitionBackend
    {
    public:
        AcquisitionBackend(std::size_t channelNum);
        AcquisitionBackend(const AcquisitionBackend&) = delete;
        AcquisitionBackend& operator=(const AcquisitionBackend&) = delete;
        AcquisitionBackend(AcquisitionBackend&&) = default;
        AcquisitionBackend& operator=(AcquisitionBackend&&) = default;
        virtual ~AcquisitionBackend();

        void run();

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

        utils::Filter* mFilter_; 
        double* mFiltBuf_;

        utils::FFTCalculator* mFFT_;

        void doSample();
        void doRecord();
        void doFilt();
        void doFFT();
    };
}   // namespace eegneo
