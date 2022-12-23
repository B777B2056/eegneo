#pragma once
#include <cstdint>
#include <fstream>
#include <span>
#include <QSerialPort>
#include <QString>
#include <QUdpSocket>
#include <QTimer>
#include "third/usb/libusb.h"
#include "utils/file.h"

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

        void doRecordEvent(const char* msg);

        const double* data() const { return mBuf_; }

        void setRecordingFlag() { this->mIsRecord_ = true; }
        void clearRecordingFlag() { this->mIsRecord_ = false; }

    protected:
        double* mBuf_;
        std::size_t mChannelNum_;

        bool mIsRecord_;
        std::uint64_t mCurDataN_; 
        std::fstream mDataFile_, mEventFile_;

        void doRecordData();
        virtual void sampleOnce() = 0;
    };

    class TestDataSampler : public EEGDataSampler
    {
    public:
        TestDataSampler(std::size_t channelNum);
        ~TestDataSampler() = default;

    private:
        QTimer timer;
        int idx = 0;
        const char* TEST_DATA_FILE_PATH = "E:/jr/eegneo/test/data/S001R01.edf";
        utils::EDFReader mEDFReader_;

        void sampleOnce() override;
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

        void sampleOnce() override;

    private:
        static constexpr double MagicCoefficient = 0.022351744455307063;
    };

    class ShanxiDataSampler : public EEGDataSampler
    {
    public:
        ShanxiDataSampler(std::size_t channelNum);
        ~ShanxiDataSampler() = default;

    private:
        QUdpSocket client;
        double turnBytes2uV(unsigned char *bytes);

        void sampleOnce() override;
    };

    class UsbDataSampler : public EEGDataSampler
    {
    public:
        UsbDataSampler(std::size_t channelNum);
        ~UsbDataSampler();

    private:
        using BYTE = unsigned char;
        static constexpr std::uint16_t VendorId = 0x0483;
        static constexpr std::uint16_t ProductId = 0x6001;
        static constexpr double MagicCoefficient = 0.02235;

    private:
        BYTE* mRawBuf_;
        struct libusb_device_handle* mUsbHolderPtr_ = nullptr;
        std::uint8_t mEpIN_ = 0;
        std::uint8_t mEpOUT_ = 0;

        void findAndInitDevice();
        bool checkConfig() const;
        void configDevice();
        void startTransfer();
        void readFromDevice(std::span<BYTE> buf);
        void writeIntoDevice(std::span<BYTE> buf);
        void sampleOnce() override;
    };
}   // namespace eegneo
