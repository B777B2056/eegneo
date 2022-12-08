#pragma once
#include <fstream>
#include <QSerialPort>
#include <QString>
#include <QSharedMemory>
#include <QUdpSocket>

namespace eegneo
{
    class DataSampler
    {
    public:
        DataSampler(std::size_t channelNum);
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
        std::fstream mRecordFile_;
        QSharedMemory mSharedMemory_;
        void doRecord();
    };

    class TestDataSampler : public DataSampler
    {
    public:
        TestDataSampler(std::size_t channelNum);
        ~TestDataSampler() = default;

    private:
        // std::fstream mDataFile_;
        void doSample() override;

        const char* DATA_FILE_PATH = "";
    };

    class ShanghaiDataSampler : public DataSampler
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

    class ShanxiDataSampler : public DataSampler
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
