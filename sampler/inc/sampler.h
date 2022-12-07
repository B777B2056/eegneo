#pragma once
#include <fstream>
#include <QSerialPort>
#include <QString>
#include <QSharedMemory>

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
        std::fstream mRecordFile_;
        QSharedMemory mSharedMemory_;

        virtual void doSample() = 0;
        void doRecord();
    };

    class TestDataSampler : public DataSampler
    {
    public:
        using DataSampler::DataSampler;

    private:
        void doSample() override 
        {  
            for (int i = 0; i < mChannelNum_; ++i)
            {
                mBuf_[i] = rand() % 10;
            }
        }
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
}   // namespace eegneo
