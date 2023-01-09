#include "sampler.h"
#include <cstring>
#include <cstdlib>
#include <QCoreApplication>
#include <QByteArray>

namespace eegneo
{
    EEGDataSampler::EEGDataSampler(std::size_t channelNum)
        : mBuf_(new double[channelNum]), mChannelNum_(channelNum)
        , mIsRecord_(false), mCurDataN_(0)
        , mDataFile_{DATA_CACHE_FILE_PATH, std::ios::out | std::ios::binary}
        , mEventFile_{EVENT_CACHE_FILE_PATH, std::ios::out}
    {
        if (!mDataFile_.is_open())
        {
            // TODO: 文件创建失败，处理
        }
        mDataFile_.seekg(0, std::ios::beg);
        mEventFile_.seekg(0, std::ios::beg);
    }

    void EEGDataSampler::doRecordData()
    {
        if (!mIsRecord_)    return;
        mDataFile_.write(reinterpret_cast<const char*>(mBuf_), this->mChannelNum_ * sizeof(double));
        ++mCurDataN_;
    }

    void EEGDataSampler::doRecordEvent(const char* msg)
    {
        this->mEventFile_ << this->mCurDataN_ << ":" << msg << std::endl;
    }

    TestDataSampler::TestDataSampler(std::size_t channelNum)
        : EEGDataSampler(channelNum)
        , mLastTimePoint_(std::chrono::steady_clock::now())
        , mEDFReader_(TEST_DATA_FILE_PATH)
    {

    }

    void TestDataSampler::sampleOnce()
    {
        auto targetTimePoint = this->mLastTimePoint_ + std::chrono::milliseconds((int)(1000.0 / mEDFReader_.sampleFreqencyHz()));
        while (targetTimePoint > std::chrono::steady_clock::now());
        this->mLastTimePoint_ = std::chrono::steady_clock::now();

        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mBuf_[i] = mEDFReader_.channel(i)[idx % mEDFReader_.channel(i).size()];
        }
        ++idx;
    }

    static void SendData2SerialPort(QSerialPort& serialPort, const QByteArray& data)
    {
        serialPort.write(data);
        serialPort.flush();
    }

    ShanghaiDataSampler::ShanghaiDataSampler(std::size_t channelNum, const QString& portName)
        : EEGDataSampler(channelNum)
        , mPortName_(portName)
    {
        this->initSerialPort();
        if (!mSerialPort_.open(QSerialPort::ReadWrite))
        {
            // Error 
            return;
        }
        this->sendStartCmd();
    }

    ShanghaiDataSampler::~ShanghaiDataSampler()
    {
        mSerialPort_.clear();
        mSerialPort_.close();
    }

    void ShanghaiDataSampler::initSerialPort()
    {
        mSerialPort_.setPortName(mPortName_);
        mSerialPort_.setBaudRate(QSerialPort::Baud256000);
        mSerialPort_.setParity(QSerialPort::NoParity);
        mSerialPort_.setStopBits(QSerialPort::OneStop);
        mSerialPort_.setFlowControl(QSerialPort::NoFlowControl);
    }

    void ShanghaiDataSampler::sendStartCmd()
    {
        QByteArray cmd;
        cmd.resize(2); 
        cmd[0] = 0x73; cmd[1] = 0x76;
        SendData2SerialPort(mSerialPort_, cmd);
        cmd.resize(5); 
        cmd[0] = 0x2f; cmd[1] = 0x30;
        cmd[2] = 0x7e; cmd[3] = 0x36; cmd[4] = 0x78;
        SendData2SerialPort(mSerialPort_, cmd);
        cmd.resize(71);
        cmd[0]  = 0x31; cmd[1]  = 0x30; cmd[2]  = 0x36; cmd[3]  = 0x30; cmd[4]  = 0x31; cmd[5]  = 0x31;
        cmd[6]  = 0x30; cmd[7]  = 0x58; cmd[8]  = 0x78; cmd[9]  = 0x32; cmd[10] = 0x30; cmd[11] = 0x36;
        cmd[12] = 0x30; cmd[13] = 0x31; cmd[14] = 0x31; cmd[15] = 0x30; cmd[16] = 0x58; cmd[17] = 0x78;
        cmd[18] = 0x33; cmd[19] = 0x30; cmd[20] = 0x36; cmd[21] = 0x30; cmd[22] = 0x31; cmd[23] = 0x31;
        cmd[24] = 0x30; cmd[25] = 0x58; cmd[26] = 0x78; cmd[27] = 0x34; cmd[28] = 0x30; cmd[29] = 0x36;
        cmd[30] = 0x30; cmd[31] = 0x31; cmd[32] = 0x31; cmd[33] = 0x30; cmd[34] = 0x58; cmd[35] = 0x78;
        cmd[36] = 0x35; cmd[37] = 0x30; cmd[38] = 0x36; cmd[39] = 0x30; cmd[40] = 0x31; cmd[41] = 0x31;
        cmd[42] = 0x30; cmd[43] = 0x58; cmd[44] = 0x78; cmd[45] = 0x36; cmd[46] = 0x30; cmd[47] = 0x36;
        cmd[48] = 0x30; cmd[49] = 0x31; cmd[50] = 0x31; cmd[51] = 0x30; cmd[52] = 0x58; cmd[53] = 0x78;
        cmd[54] = 0x37; cmd[55] = 0x30; cmd[56] = 0x36; cmd[57] = 0x30; cmd[58] = 0x31; cmd[59] = 0x31;
        cmd[60] = 0x30; cmd[61] = 0x58; cmd[62] = 0x78; cmd[63] = 0x38; cmd[64] = 0x30; cmd[65] = 0x36;
        cmd[66] = 0x30; cmd[67] = 0x31; cmd[68] = 0x31; cmd[69] = 0x30; cmd[70] = 0x58;
        SendData2SerialPort(mSerialPort_, cmd);
        cmd.resize(1);
        cmd[0] = 'b';
        SendData2SerialPort(mSerialPort_, cmd);
    }

    // 帧头：C0 A0
    // 帧尾：6个0
    void ShanghaiDataSampler::sampleOnce()
    {
        int cur_channel = 0, state_machine = 0;
        char ch = 0;
        char single_num[3];
        // 有限状态机
        while(mSerialPort_.read(&ch, 1))
        {
            if(state_machine == 0)
            {
                if(ch == (char)0xc0)
                    state_machine = 1;
                else
                    state_machine = 0;
            }
            else if(state_machine == 1)
            {
                if(ch == (char)0xa0)
                    state_machine = 2;
                else
                    state_machine = 0;
            }
            else if(state_machine == 2)
            {
                state_machine = 3;
            }
            else if(state_machine < 28)
            {
                if(!((state_machine - 3) % 3) && (state_machine > 3))
                {
                    mBuf_[cur_channel++] = turnBytes2uV(single_num[0], single_num[1], single_num[2]);
                }
                single_num[(state_machine - 3) % 3] = ch;
                state_machine++;
            }
            else if(state_machine < 34)
            {
                if(ch == 0)
                    state_machine++;
                else
                    state_machine = 0;
            }
            else
                return;
        }
    }

    double ShanghaiDataSampler::turnBytes2uV(char byte1, char byte2, char byte3)
    {
        int target = ((int)byte1 << 16) + (((int)byte2 << 8) & 0x0000ffff) + ((int)byte3 & 0x000000ff);
        return (double)target * MagicCoefficient;
    }

    UsbDataSampler::UsbDataSampler(std::size_t channelNum)
        : EEGDataSampler(channelNum)
        , mRawBuf_(new BYTE[3 * channelNum + 1])
    {
        // 初始化libusb
        if (LIBUSB_SUCCESS != ::libusb_init(nullptr))
        {
            // Error
            return;
        }
        // 打开指定usb设备
        this->findAndInitDevice();
        // 发送启动命令
        this->startTransfer();
    }

    UsbDataSampler::~UsbDataSampler()
    {
        ::libusb_release_interface(mUsbHolderPtr_, 0);
        ::libusb_close(mUsbHolderPtr_);
        ::libusb_exit(nullptr);
        delete[] mRawBuf_;
    }

    void UsbDataSampler::findAndInitDevice()
    {
        // 打开指定设备
        this->mUsbHolderPtr_ = ::libusb_open_device_with_vid_pid(nullptr, VendorId, ProductId);
        if (!this->mUsbHolderPtr_)
        {
            // Error
            return;
        }
        // 检查usb设备是否已经被操作系统配置
        if (this->checkConfig())
        {   
            // 若未配置，则配置usb设备
            this->configDevice();
        }
        // 设置端点
        struct libusb_config_descriptor* config;
        if (LIBUSB_SUCCESS != ::libusb_get_active_config_descriptor(::libusb_get_device(this->mUsbHolderPtr_), &config)) 
        {
            // Error
            return;
        }
        // 匹配第一个输入输出端点
        auto* interface = &config->interface[0];
        auto* interfaceDescriptor = &interface->altsetting[0];

        for (std::uint8_t k = 0; k < interfaceDescriptor->bNumEndpoints; ++k)
        {
            auto* endpointDescriptor = &interfaceDescriptor->endpoint[k];
            if ((endpointDescriptor->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) & (LIBUSB_TRANSFER_TYPE_BULK)) 
            {
                if (LIBUSB_ENDPOINT_IN == (LIBUSB_ENDPOINT_IN & endpointDescriptor->bEndpointAddress))
                {
                    this->mEpIN_ = endpointDescriptor->bEndpointAddress;
                    ::libusb_clear_halt(this->mUsbHolderPtr_, this->mEpIN_); //清除暂停标志
                }
                else if (LIBUSB_ENDPOINT_OUT == (LIBUSB_ENDPOINT_OUT & endpointDescriptor->bEndpointAddress))
                {
                    this->mEpOUT_ = endpointDescriptor->bEndpointAddress;
                    ::libusb_clear_halt(this->mUsbHolderPtr_, this->mEpOUT_); //清除暂停标志
                }
            }
            if ((this->mEpIN_ > 0) && (this->mEpOUT_ > 0))  break;
        }

        // 启用内核驱动程序自动分离（在Windows中，libusb需要附加驱动程序，因此分离驱动程序的功能毫无意义，故不检查返回值）
        ::libusb_set_auto_detach_kernel_driver(this->mUsbHolderPtr_, 1);
        // 初始化USB设备接口
        if (LIBUSB_SUCCESS != ::libusb_claim_interface(this->mUsbHolderPtr_, 0))
        {
            // Error
            return;
        }
    }

    bool UsbDataSampler::checkConfig() const
    {
        int configuration = 0;
        if (LIBUSB_SUCCESS != ::libusb_get_configuration(mUsbHolderPtr_, &configuration)) 
        {
            // Error
            return false;
        }
        return configuration > 0;
    }

    void UsbDataSampler::configDevice()
    {
        if (LIBUSB_SUCCESS != ::libusb_set_configuration(mUsbHolderPtr_, 1))
        {
            // Error
            return;
        }
    }

    void UsbDataSampler::startTransfer()
    {
        BYTE readBuf[512] = {0};
        BYTE writeBuf[2] = {0xAD,0x02};
        this->writeIntoDevice({writeBuf, 2});
        this->readFromDevice({readBuf, 512});
        this->readFromDevice({readBuf, 512});
        
        writeBuf[0] = 0xAD; writeBuf[1] = 0x00;
        this->writeIntoDevice({writeBuf, 2});

        this->readFromDevice({readBuf, 36});
        if (0xAE != readBuf[0])
        {
            // Error, should be array('B', [174, 0, 0, 0, 0, 0, 0, 96, 4, 1, 128, 1, 0, 0, 0, 0])
            return;
        }

        writeBuf[0] = 0xAD; writeBuf[1] = 0x01;
        this->writeIntoDevice({writeBuf, 2});
    }

    void UsbDataSampler::readFromDevice(std::span<BYTE> buf)
    {
        int length = static_cast<int>(buf.size_bytes());
        int bytesTransferred;
        for (bytesTransferred = 0; bytesTransferred < length; )
        {
            int ret = ::libusb_bulk_transfer(this->mUsbHolderPtr_, this->mEpIN_, 
                                             buf.data() + bytesTransferred, length - bytesTransferred, 
                                             &bytesTransferred, 100u);
            if (LIBUSB_SUCCESS != ret)
            {
                // Error
                return;
            }
        }
    }

    void UsbDataSampler::writeIntoDevice(std::span<BYTE> buf)
    {
        int length = static_cast<int>(buf.size_bytes());
        int bytesTransferred;
        for (bytesTransferred = 0; bytesTransferred < length; )
        {
            int ret = ::libusb_bulk_transfer(this->mUsbHolderPtr_, this->mEpOUT_, 
                                             buf.data() + bytesTransferred, length - bytesTransferred, 
                                             &bytesTransferred, 0u);
            if (LIBUSB_SUCCESS != ret)
            {
                // Error
                return;
            }
        }
    }

    void UsbDataSampler::sampleOnce()
    {
        this->readFromDevice({this->mRawBuf_, 3 * this->mChannelNum_ + 1});
        for (std::size_t i = 0; i < this->mChannelNum_; ++i)
        {
            // 以大端字节序解析原始数据
            std::int64_t rawNum = 0;
            rawNum |= (this->mRawBuf_[3 * i + 0] * 0x100 * 0x100);
            rawNum |= (this->mRawBuf_[3 * i + 1] * 0x100);
            rawNum |= (this->mRawBuf_[3 * i + 2]);
            // 乘以系数转换为可读数据
            this->mBuf_[i] = MagicCoefficient * rawNum;
        }
    }
}   // namespace eegneo
