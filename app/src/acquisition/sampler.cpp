#include "acquisition/sampler.h"
#include <QByteArray>

namespace eegneo
{
    constexpr const char* RECORD_FILE_PATH = "temp.txt";

    DataSampler::DataSampler(std::size_t channelNum)
        : mIsSampled_(false)
        , mSampleThread_{&DataSampler::doSample, this}
        , mRecordThread_{&DataSampler::doRecord, this}
        , mRecordFile_{RECORD_FILE_PATH}
        , mBuf_(channelNum)
    {
        if(!mRecordFile_.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            // TODO: 文件创建失败，处理
        }
    }

    DataSampler::~DataSampler()
    {
        if (mSampleThread_.joinable())
        {
            mSampleThread_.join();
        }
        mRecordFile_.close();
    }

    void DataSampler::doRecord()
    {
        std::unique_lock<std::mutex> l(mMutex_);
        for (;;)
        {
            while (!mIsSampled_)
            {
                mCv_.wait(l);
            }
            for (double& val : mBuf_)
            {
                mRecordFile_.write((char*)&val, sizeof(val));
                mRecordFile_.write(",");
            }
            mRecordFile_.write("\n");
        }
    }

    void DataSampler::doSample()
    {
        std::unique_lock<std::mutex> l(mMutex_);
        for (;;)
        {
            this->run();
            mIsSampled_ = true;
            mCv_.notify_all();
        }
    }

    static void SendData2SerialPort(QSerialPort& serialPort, const QByteArray& data)
    {
        serialPort.write(data);
        serialPort.flush();
    }

    ShanghaiDataSampler::ShanghaiDataSampler(std::size_t channelNum, const QString& portName)
        : DataSampler(channelNum)
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

    void ShanghaiDataSampler::run()
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
                    /* TODO: 滤波 */ 
                    // if(isRec)
                    // {
                    //     for(int si = 0; si < channels_num; si++)
                    //     {
                    //         /*写入缓存txt文件*/
                    //         if(si < this->channels_num - 1)
                    //             samplesWrite << data[si] << " ";
                    //         else
                    //             samplesWrite << data[si] << std::endl;
                    //     }
                    //     ++cnt;
                    // }
                    // processData();
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
}   // namespace eegneo
