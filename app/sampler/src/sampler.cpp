#include "sampler.h"
#include <cstring>
#include <cstdlib>
#include <QByteArray>
#include <QTcpSocket>
#include "utils/filter.h"

namespace eegneo
{
    constexpr const char* RECORD_FILE_PATH = "E:/jr/eegneo/temp.txt";

    DataSampler::DataSampler(std::size_t channelNum, QTcpSocket* ipcChannel)
        : mBuf_(new double[channelNum]), mChannelNum_(channelNum), mIpcChannel_(ipcChannel)
        , mRecordFile_{RECORD_FILE_PATH, std::ios::out}, mSharedMemory_{"Sampler"}, mIsStop_(false)
        , mFilter_(new utils::Filter()), mFiltBuf_(new double[channelNum]), mProcessThread_{&DataSampler::start, this}
    {
        if (!mRecordFile_.is_open())
        {
            // TODO: 文件创建失败，处理
        }
        if (mSharedMemory_.attach())
        {
            mSharedMemory_.detach();
        }
        if (!mSharedMemory_.create(mChannelNum_ * sizeof(double)))
        {
            throw "Shared memory create failed!";
        }

        mOriginalSignals_.resize(mChannelNum_);
        mFiltResults_.resize(mChannelNum_);
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mFiltResults_[i].resize(utils::Filter::numTaps());
        }
    }

    DataSampler::~DataSampler()
    {
        if (mProcessThread_.joinable())
        {
            mProcessThread_.join();
        }

        delete[] mBuf_;
        delete[] mFiltBuf_;
        delete mFilter_;
    }

    void DataSampler::start()
    {
        while (!mIsStop_)
        {
            std::unique_lock<std::mutex> lock(mMutex_);
            this->doSample();
            if (mRecCmd_.isRecordOn) this->doRecord();
            if (mFiltCmd_.isFiltOn) this->doFilt();

            if (!mSharedMemory_.lock()) continue;
            if (mFiltCmd_.isFiltOn)
            {
                ::memcpy(mSharedMemory_.data(), mFiltBuf_, mChannelNum_ * sizeof(double));
            }
            else
            {
                ::memcpy(mSharedMemory_.data(), mBuf_, mChannelNum_ * sizeof(double));
            }
            if (!mSharedMemory_.unlock()) continue;
        }
    }

    void DataSampler::doRecord()
    {
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mRecordFile_ << mBuf_[i] << " ";
        }
        mRecordFile_ << "\n";
    }

    void DataSampler::doFilt()
    {
        if (mOriginalSignals_[0].size() < utils::Filter::numTaps())
        {
            for (std::size_t i = 0; i < mChannelNum_; ++i)
            {
                mOriginalSignals_[i].push_back(mBuf_[i]);
            }
            return ;
        }

        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mOriginalSignals_[i].erase(mOriginalSignals_[i].begin());
            mOriginalSignals_[i].push_back(mBuf_[i]);
        }

        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            auto& signal = mOriginalSignals_[i];
            auto& result = mFiltResults_[i];
            if ((mFiltCmd_.lowCutoff >= 0.0) && (mFiltCmd_.highCutoff < 0.0))   // 高通滤波
            {
                mFilter_->lowPass(mFiltCmd_.lowCutoff, signal, result);
            }
            else if ((mFiltCmd_.lowCutoff < 0.0) && (mFiltCmd_.highCutoff >= 0.0))  // 低通滤波
            {
                mFilter_->highPass(mFiltCmd_.highCutoff, signal, result);
            }
            else if ((mFiltCmd_.lowCutoff >= 0.0) && (mFiltCmd_.highCutoff >= 0.0)) // 带通滤波
            {
                mFilter_->bandPass(mFiltCmd_.lowCutoff, mFiltCmd_.highCutoff, signal, result);
            }
            else    // 无滤波
            {

            }
            if (mFiltCmd_.notchCutoff > 0.0)  // 陷波滤波
            {
                mFilter_->notch(mFiltCmd_.notchCutoff, signal, result);
            }
            mFiltBuf_[i] = result.back();
        }
    }

    void DataSampler::handleIpcMsg()
    {
        std::unique_lock<std::mutex> lock(mMutex_);
        if (mIpcChannel_->bytesAvailable() <= 0)
        {
            return;
        }
        CmdHeader cmdHdr;
        if (!mIpcChannel_->read((char*)&cmdHdr, sizeof(cmdHdr)))
        {
            return;
        }
        switch (cmdHdr.id)
        {
        case CmdId::Rec:
            if (RecordCmd cmd; mIpcChannel_->read((char*)&cmd, sizeof(cmd)))
            {
                this->mRecCmd_ = cmd;   // 设置记录参数
            }
            break;
        case CmdId::Filt:
            if (FiltCmd cmd; mIpcChannel_->read((char*)&cmd, sizeof(cmd)))
            {
                this->mFiltCmd_ = cmd;  // 设置滤波参数
                mFilter_->setSampleFreqHz(cmd.sampleRate);
            }
            break;
        case CmdId::Shutdown:
            this->mIsStop_ = true;  // 退出本进程
            break;
        default:
            break;
        }
    }

    TestDataSampler::TestDataSampler(std::size_t channelNum, QTcpSocket* ipcChannel)
        : DataSampler(channelNum, ipcChannel)
        // , mDataFile_{DATA_FILE_PATH, std::ios::in}
    {
        // if (!mDataFile_.is_open())
        // {
        //     throw "Data file not open!";
        // }
    }

    void TestDataSampler::doSample()
    {
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mBuf_[i] = rand() % 10;
        }
    }

    static void SendData2SerialPort(QSerialPort& serialPort, const QByteArray& data)
    {
        serialPort.write(data);
        serialPort.flush();
    }

    ShanghaiDataSampler::ShanghaiDataSampler(std::size_t channelNum, QTcpSocket* ipcChannel, const QString& portName)
        : DataSampler(channelNum, ipcChannel)
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

    void ShanghaiDataSampler::doSample()
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
        return (double)target * MAGIC_COFF;
    }

    ShanxiDataSampler::ShanxiDataSampler(std::size_t channelNum, QTcpSocket* ipcChannel)
        : DataSampler(channelNum, ipcChannel)
    {
        client.bind(QHostAddress::LocalHost, 4000);
    }

    void ShanxiDataSampler::doSample()
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
}   // namespace eegneo
