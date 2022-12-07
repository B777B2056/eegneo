#pragma once
#include <atomic>
#include <array>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <fstream>
#include <QSerialPort>
#include <QString>
#include <QVector>

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

        double data(std::size_t channelIdx) const;

    protected:
        std::array<double, 16> mBuf_;
        std::size_t mChannelNum_;
        virtual void run() = 0;

    private:
        std::atomic_bool mIsStop_;
        mutable std::shared_mutex mReadWriteMutex_;
        std::thread mSampleThread_; // 板子采样线程
        std::thread mRecordThread_; // 记录数据线程（记录到文件中）
        std::deque<std::array<double, 16>> mQueue_;

        void doRecord();
        void doSample();
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
        void run() override;

    private:
        static constexpr double MAGIC_COFF = 0.022351744455307063;
    };
}   // namespace eegneo
