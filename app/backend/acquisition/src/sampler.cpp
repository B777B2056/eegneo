#include "sampler.h"
#include <cstring>
#include <cstdlib>
#include <QByteArray>
#include <QTcpSocket>

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

    static double maxInVector(const std::vector<double>& vec)
    {
        double ret = vec[0];
        for (std::size_t i = 1; i < vec.size(); ++i)
        {
            if (vec[i] > ret)   ret = vec[i];
        }
        return ret;
    }

    static double minInVector(const std::vector<double>& vec)
    {
        double ret = vec[0];
        for (std::size_t i = 1; i < vec.size(); ++i)
        {
            if (vec[i] < ret)   ret = vec[i];
        }
        return ret;
    }

    TestDataSampler::TestDataSampler(std::size_t channelNum)
        : EEGDataSampler(channelNum)
        , mEDFReader_(TEST_DATA_FILE_PATH)
    {
        QObject::connect(&timer, &QTimer::timeout, [this]()->void
        {
            for (std::size_t i = 0; i < mChannelNum_; ++i)
            {
                mBuf_[i] = mEDFReader_.channel(i)[idx % mEDFReader_.channel(i).size()];
            }
            ++idx;
            this->doRecordData();
        });
        timer.start(1000.0/mEDFReader_.sampleFreqencyHz());
    }

    void TestDataSampler::sampleOnce()
    {

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
            // Handle open series port error 
        }
    }

    ShanghaiDataSampler::~ShanghaiDataSampler()
    {
        mSerialPort_.clear();
        mSerialPort_.close();
    }

    void ShanghaiDataSampler::sampleOnce()
    {
        if (!mSerialPort_.isOpen())
        {
            return;
        }
        this->sendStartCmd();
        this->handle();
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
    void ShanghaiDataSampler::handle()
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
                if(cur_channel == 8)
                {
                    cur_channel = 0;
                }
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
                state_machine = 0;
        }
    }

    double ShanghaiDataSampler::turnBytes2uV(char byte1, char byte2, char byte3)
    {
        int target = ((int)byte1 << 16) + (((int)byte2 << 8) & 0x0000ffff) + ((int)byte3 & 0x000000ff);
        return (double)target * MagicCoefficient;
    }

    ShanxiDataSampler::ShanxiDataSampler(std::size_t channelNum)
        : EEGDataSampler(channelNum)
    {
        client.bind(QHostAddress::LocalHost, 4000);
    }

    void ShanxiDataSampler::sampleOnce()
    {
        while(!client.hasPendingDatagrams());
        char bytes[72];
        int size = client.readDatagram(bytes, 72);
        for(int i = 0; i < size; i++)
        {
            if((i < size - 1)
                    && (bytes[i] == (char)0xc0) && (bytes[i + 1] == (char)0xa0))
            {
                if((i < size - 71)
                        && (bytes[i + 66] == (char)0x00) && (bytes[i + 67] == (char)0x00)
                        && (bytes[i + 68] == (char)0x00) && (bytes[i + 69] == (char)0x00)
                        && (bytes[i + 70] == (char)0x00) && (bytes[i + 71] == (char)0x00))
                {
                    int curChannel = 0;
                    for(int offset = 2; offset < 66; offset += 8)
                    {
                        unsigned char chs[8] = {
                                                (unsigned char)bytes[i+offset], (unsigned char)bytes[i+offset+1],
                                                (unsigned char)bytes[i+offset+2], (unsigned char)bytes[i+offset+3],
                                                (unsigned char)bytes[i+offset+4], (unsigned char)bytes[i+offset+5],
                                                (unsigned char)bytes[i+offset+6], (unsigned char)bytes[i+offset+7]
                                                };
                        mBuf_[curChannel++] = turnBytes2uV(chs);
                    }
                    i += 39;
                }
            }
        }
    }

    double ShanxiDataSampler::turnBytes2uV(unsigned char *bytes)
    {
        int i, exponent;
        unsigned int sign;
        unsigned long long mantissa;
        double curNum = 0.0;
        std::string str = "";
        sign = (bytes[7] & 0x80) >> 7;
        exponent = (static_cast<unsigned int>(bytes[7] & 0x7f) << 4) + (static_cast<unsigned int>(bytes[6] & 0xf0) >> 4) - 1023;
        mantissa = (static_cast<unsigned long long>(bytes[6] & 0x0f) << 48)
                + (static_cast<unsigned long long>(bytes[5]) << 40)
                + (static_cast<unsigned long long>(bytes[4]) << 32)
                + (static_cast<unsigned long long>(bytes[3]) << 24)
                + (static_cast<unsigned long long>(bytes[2]) << 16)
                + (static_cast<unsigned long long>(bytes[1]) << 8)
                + (static_cast<unsigned long long>(bytes[0]));
        while(mantissa)
        {
            str += ('0' + mantissa % 2);
            mantissa /= 2;
        }
        while(str.length() < 52)
            str += '0';
        str += '1';
        if(exponent >= 0)
        {
            for(i = 0; i < (int)str.length(); i++)
            {
                if((int)i <= exponent)
                    curNum += ((str[str.length() - i - 1] - '0') * pow(2, exponent - i));
                else
                    curNum += ((str[str.length() - i - 1] - '0') * (1.0 / pow(2, i - exponent)));
            }
        }
        else
        {
            for(i = 0; i < -exponent - 1; i++)
                str += '0';
            for(i = 0; i < (int)str.length(); i++)
                curNum += ((str[str.length() - i - 1] - '0') * pow(2, -i - 1));
        }
        curNum *= pow(-1, sign);
        return curNum;
    }

    UsbDataSampler::UsbDataSampler(std::size_t channelNum)
        : EEGDataSampler(channelNum)
        , mRawBuf_(new BYTE[3 * channelNum + 1])
        , mUsbHolderPtr_(nullptr)
    {
        // 初始化libusb
        if (LIBUSB_SUCCESS != ::libusb_init(nullptr))
        {
            // Error
            return;
        }
        // 打开指定usb设备
        if (this->findDevice(); !mUsbHolderPtr_)
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
        // 初始化USB设备接口
        if (LIBUSB_SUCCESS != ::libusb_claim_interface(mUsbHolderPtr_, 0))
        {
            // Error
            return;
        }
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

    void UsbDataSampler::findDevice()
    {
        struct libusb_device** devs; 
        int cnt = ::libusb_get_device_list(nullptr, &devs);
        if (cnt < 0)
        {
            // Error
            return;
        }
        // 遍历设备列表，查找匹配要求的设备
        int ret;
        for (int i = 0; i < cnt; ++i)
        {
            struct libusb_device_descriptor desc;
            ret = ::libusb_get_device_descriptor(devs[i], &desc);
            if (LIBUSB_SUCCESS != ret) 
            {
                // Error
                return;
            }

            struct libusb_config_descriptor* config;
            ret = ::libusb_get_active_config_descriptor(devs[i], &config);
            if (LIBUSB_SUCCESS != ret) 
            {
                // Error
                return;
            }

            if((VendorId == desc.idVendor) && (ProductId == desc.idProduct) && (0 == config->bNumInterfaces)) 
            {
                // 打开目标设备
                ret = ::libusb_open(devs[i], &this->mUsbHolderPtr_); 
                if(LIBUSB_SUCCESS != ret) 
                {
                    // Error
                }
                ::libusb_free_config_descriptor(config);
                ::libusb_free_device_list(devs, 1);
                return;
            }
            ::libusb_free_config_descriptor(config);
        }
        ::libusb_free_device_list(devs, 1);
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
        BYTE buf[1024] = {0xAD,0x02};
        this->writeIntoDevice({buf, 2});
        this->readFromDevice({buf, 512 * 2});
        
        buf[0] = 0xAD; buf[1] = 0x00;
        this->writeIntoDevice({buf, 2});

        this->readFromDevice({buf, 36});
        if (0xAE != buf[0])
        {
            // Error
            return;
        }

        buf[0] = 0xAD; buf[1] = 0x01;
        this->writeIntoDevice({buf, 2});
    }

    void UsbDataSampler::readFromDevice(std::span<BYTE> buf)
    {
        // TODO
    }

    void UsbDataSampler::writeIntoDevice(std::span<BYTE> buf)
    {
        // TODO
    }

    void UsbDataSampler::sampleOnce()
    {
        this->readFromDevice({this->mRawBuf_, 3 * this->mChannelNum_ + 1});
        for (std::size_t i = 0; i < this->mChannelNum_; ++i)
        {
            // 以大端字节序解析原始数据
            std::uint32_t rawNum = 0;
            rawNum |= this->mRawBuf_[3 * i];     rawNum <<= 16;
            rawNum |= this->mRawBuf_[3 * i + 1]; rawNum <<= 8;
            rawNum |= this->mRawBuf_[3 * i + 2];
            // 乘以系数转换为可读数据
            this->mBuf_[i] = MagicCoefficient * static_cast<std::int32_t>(rawNum);
        }
    }
}   // namespace eegneo
