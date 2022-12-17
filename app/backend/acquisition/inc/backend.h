#pragma once
#include <fstream>
#include <QSharedMemory>
#include "common/common.h"
#include "utils/ipc.h"

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

    private:
        utils::IpcClient mIpcWrapper_;

        EEGDataSampler* mDataSampler_;
        std::size_t mChannelNum_;

        QSharedMemory mSharedMemory_;

        FiltCmd mFiltCmd_;

        utils::Filter* mFilter_; 
        double* mFiltBuf_;

        utils::FFTCalculator* mFFT_;

        void doSample();
        void doFilt();
        void doFFT();

        void handleRecordCmd(RecordCmd* cmd);
        void handleFiltCmd(FiltCmd* cmd);
        void handleShutdownCmd(ShutdownCmd* cmd);
        void handleMarkerCmd(MarkerCmd* cmd);
        void handleFileSaveCmd(FileSaveCmd* cmd);
    };
}   // namespace eegneo
