#pragma once
#include <mutex>
#include <thread>
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

        double data(std::size_t channelIdx) const { return mBuf_.at(channelIdx); }

    protected:
        std::mutex mMutex_;
        std::thread mThread_;
        QVector<double> mBuf_;

        virtual void doSample() = 0;
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
