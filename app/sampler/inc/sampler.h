#pragma once
#include <atomic>
#include <fstream>
#include <mutex>
#include <thread>
#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QSharedMemory>
#include <QUdpSocket>
#include "common/common.h"

class QTcpSocket;

namespace eegneo
{
    namespace utils { class Filter; }

    class DataSampler : public QObject
    {
        Q_OBJECT

    public:
        DataSampler(QObject* parent = nullptr) : mBuf_(nullptr), mIpcChannel_(nullptr) {}
        DataSampler(std::size_t channelNum, QTcpSocket* ipcChannel);
        DataSampler(const DataSampler&) = delete;
        DataSampler& operator=(const DataSampler&) = delete;
        DataSampler(DataSampler&&) = default;
        DataSampler& operator=(DataSampler&&) = default;
        virtual ~DataSampler();

        void start();

    protected:
        double* mBuf_;
        std::size_t mChannelNum_;
        virtual void doSample() = 0;

    private:
        std::mutex mMutex_;
        QTcpSocket* mIpcChannel_;
        std::fstream mRecordFile_;
        QSharedMemory mSharedMemory_;
        RecordCmd mRecCmd_;
        FiltCmd mFiltCmd_;
        std::atomic_bool mIsStop_;
        std::thread mProcessThread_;

        utils::Filter* mFilter_;
        double* mFiltBuf_;
        std::vector<std::vector<double>> mOriginalSignals_;
        std::vector<std::vector<double>> mFiltResults_;

        void doRecord();
        void doFilt();

    public slots:
        void handleIpcMsg();
    };

    class TestDataSampler : public DataSampler
    {
        Q_OBJECT

    public:
        TestDataSampler(std::size_t channelNum, QTcpSocket* ipcChannel);
        ~TestDataSampler() = default;

    private:
        // std::fstream mDataFile_;
        void doSample() override;

        // const char* DATA_FILE_PATH = "";
    };

    class ShanghaiDataSampler : public DataSampler
    {
        Q_OBJECT

    public:
        ShanghaiDataSampler(std::size_t channelNum, QTcpSocket* ipcChannel, const QString& portName);
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

    class ShanxiDataSampler : public DataSampler
    {
        Q_OBJECT

    public:
        ShanxiDataSampler(std::size_t channelNum, QTcpSocket* ipcChannel);
        ~ShanxiDataSampler() = default;

    private:
        QUdpSocket client;
        double turnBytes2uV(unsigned char *bytes);
        void doSample() override;
    };
}   // namespace eegneo
