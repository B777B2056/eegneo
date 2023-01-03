#pragma once
#include <semaphore>
#include <QSharedMemory>
#include "threadpool.h"
#include "common/common.h"

namespace eegneo
{
    class EEGDataSampler;
    class TopoPlot;
    class Filter; 
    class FFTCalculator;
    namespace utils { class IpcClient; }

    class AcquisitionBackend
    {
    public:
        AcquisitionBackend(std::size_t channelNum, std::size_t sampleFreqHz);
        AcquisitionBackend(const AcquisitionBackend&) = delete;
        AcquisitionBackend& operator=(const AcquisitionBackend&) = delete;
        AcquisitionBackend(AcquisitionBackend&&) = default;
        AcquisitionBackend& operator=(AcquisitionBackend&&) = default;
        virtual ~AcquisitionBackend();

        void doTaskInMainThread();

    private:
        std::binary_semaphore mSmphSignalA_;
        std::binary_semaphore mSmphSignalB_;
        std::atomic<bool> mIsStop_;
        ThreadPool mThreadPool_;

        utils::IpcClient* mIpcWrapper_;

        EEGDataSampler* mDataSampler_;
        std::size_t mChannelNum_;

        QSharedMemory mSharedMemory_;

        FiltCmd mFiltCmd_;

        Filter* mFilter_; 
        double* mFiltBuf_;

        FFTCalculator* mFFT_;

        TopoPlot* mTopoPlot_;

        void initIpc();
        void initSharedMemory();
        void initTaskThreads();

        void doSample();
        void doFilt();
        void doFFT();
        void doTopoPlot();

        void handleRecordCmd(RecordCmd* cmd);
        void handleFiltCmd(FiltCmd* cmd);
        void handleShutdownCmd(ShutdownCmd* cmd);
        void handleMarkerCmd(MarkerCmd* cmd);
        void handleFileSaveCmd(FileSaveCmd* cmd);
    };
}   // namespace eegneo
