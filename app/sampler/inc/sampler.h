#pragma once
#include <atomic>
#include <fstream>
#include <mutex>
#include <QSerialPort>
#include <QString>
#include <QSharedMemory>
#include <QUdpSocket>
#include "common/common.h"

class QTcpSocket;

namespace eegneo
{
    namespace utils { class Filter; }

    class EEGDataSampler
    {
    public:
        EEGDataSampler(std::size_t channelNum);
        EEGDataSampler(const EEGDataSampler&) = delete;
        EEGDataSampler& operator=(const EEGDataSampler&) = delete;
        EEGDataSampler(EEGDataSampler&&) = default;
        EEGDataSampler& operator=(EEGDataSampler&&) = default;
        virtual ~EEGDataSampler();

        void start();

        void handleRecordCmd(RecordCmd* cmd);
        void handleFiltCmd(FiltCmd* cmd);
        void handleShutdownCmd(ShutdownCmd* cmd);
        void handleMarkerCmd(MarkerCmd* cmd);

    protected:
        double* mBuf_;
        std::size_t mChannelNum_;
        virtual void doSample() = 0;

    private:
        std::uint64_t mCurDataN_;
        std::fstream mDataFile_, mEventFile_;

        QSharedMemory mSharedMemory_;

        RecordCmd mRecCmd_;
        FiltCmd mFiltCmd_;
        
        std::mutex mMutex_;
        std::atomic_bool mIsStop_;

        utils::Filter* mFilter_;
        double* mFiltBuf_;

        void doRecord();
        void doFilt();
    };

    class TestDataSampler : public EEGDataSampler
    {
    public:
        TestDataSampler(std::size_t channelNum);
        ~TestDataSampler() = default;

    private:
        // std::fstream mDataFile_;
        void doSample() override;

        // const char* DATA_FILE_PATH = "";
    };

    class ShanghaiDataSampler : public EEGDataSampler
    {
    public:
        ShanghaiDataSampler(std::size_t channelNum, const QString& portName);
        ~ShanghaiDataSampler();

    private:
        QString mPortName_;
        QSerialPort mSerialPort_;

        void initSerialPort();
        void sendStartCmd();
        void handle();
        double turnBytes2uV(char byte1, char byte2, char byte3);
        void doSample() override;

    private:
        static constexpr double MAGIC_COFF = 0.022351744455307063;
    };

    class ShanxiDataSampler : public EEGDataSampler
    {
    public:
        ShanxiDataSampler(std::size_t channelNum);
        ~ShanxiDataSampler() = default;

    private:
        QUdpSocket client;
        double turnBytes2uV(unsigned char *bytes);
        void doSample() override;
    };
}   // namespace eegneo
