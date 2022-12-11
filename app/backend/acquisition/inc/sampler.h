#pragma once
#include <QSerialPort>
#include <QString>
#include <QUdpSocket>

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
        virtual ~EEGDataSampler() { delete[] mBuf_; }

        virtual void sampleOnce() = 0;
        const double* data() const { return mBuf_; }

    protected:
        double* mBuf_;
        std::size_t mChannelNum_;
    };

    class TestDataSampler : public EEGDataSampler
    {
    public:
        TestDataSampler(std::size_t channelNum);
        ~TestDataSampler() = default;

        void sampleOnce() override;

    private:
        // std::fstream mDataFile_;

        // const char* DATA_FILE_PATH = "";
    };

    class ShanghaiDataSampler : public EEGDataSampler
    {
    public:
        ShanghaiDataSampler(std::size_t channelNum, const QString& portName);
        ~ShanghaiDataSampler();

        void sampleOnce() override;

    private:
        QString mPortName_;
        QSerialPort mSerialPort_;

        void initSerialPort();
        void sendStartCmd();
        void handle();
        double turnBytes2uV(char byte1, char byte2, char byte3);

    private:
        static constexpr double MAGIC_COFF = 0.022351744455307063;
    };

    class ShanxiDataSampler : public EEGDataSampler
    {
    public:
        ShanxiDataSampler(std::size_t channelNum);
        ~ShanxiDataSampler() = default;

        void sampleOnce() override;

    private:
        QUdpSocket client;
        double turnBytes2uV(unsigned char *bytes);
    };
}   // namespace eegneo
