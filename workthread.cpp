#include "workthread.h"
#include <cstdlib>
#include <QDebug>

std::vector<std::deque<double> > _dataBuf;

DataProcessThread::DataProcessThread(int channels_num, int sampleRate, BoardType b, QString c)
    : sp(sampleRate), cnt(1), cofe(0.022351744455307063), isFilt(false), isRec(false), board(b), com(c)
{
    this->channels_num = channels_num;
    sum.assign(channels_num, 0.0);
    data.assign(channels_num, 0.0);
    averData.assign(channels_num, 0.0);
    filtData.assign(channels_num, 0.0);
    bandPassBuffer.assign(channels_num, QQueue<double>());
    notchBuffer.assign(channels_num, QQueue<double>());
    averBuffer.assign(channels_num, QQueue<QPointF>());
    qRegisterMetaType<std::vector<double> >("std::vector<double>");
    boardInit();
}

DataProcessThread::~DataProcessThread()
{
    delete []bandPassCoff;
    delete []notchCoff;
}

void DataProcessThread::run()
{
    //启动子线程消息循环
    exec();
}

void DataProcessThread::boardInit()
{
    if(board == Shanxi)
    {
        // 用UDP进行IPC
        client = new QUdpSocket(this);
        client->abort();
        client->bind(QHostAddress("127.0.0.1"), 4000);
        connect(client, SIGNAL(readyRead()), this, SLOT(getDataFromShanxi()));
    }
    else
    {
        // 直接读串口
        struct PortSettings my_setting
        {
            BAUD256000,   // 波特率
            DATA_8,     // 数据位
            PAR_NONE,   // 奇偶校验位
            STOP_1,     // 停止位
            FLOW_OFF,   // 控制流
            100         // 超时时间（ms）
        };
        port = new QextSerialPort(com, my_setting, QextSerialPort::QueryMode::EventDriven);
        port->open(QIODevice::ReadWrite);
        connect(port, SIGNAL(readyRead()), this, SLOT(getDataFromShanghai()));
        QByteArray commands[3];
        commands[0].resize(2); commands[0][0] = 0x73; commands[0][1] = 0x76;
        port->write(commands[0]);
        commands[1].resize(5); commands[1][0] = 0x2f; commands[1][1] = 0x30;
        commands[1][2] = 0x7e; commands[1][3] = 0x36; commands[1][4] = 0x78;
        port->write(commands[1]);
        commands[2].resize(71);
        commands[2][0] = 0x31; commands[2][1] = 0x30; commands[2][2] = 0x36;
        commands[2][3] = 0x30; commands[2][4] = 0x31; commands[2][5] = 0x31;
        commands[2][6] = 0x30; commands[2][7] = 0x58; commands[2][8] = 0x78;
        commands[2][9] = 0x32; commands[2][10] = 0x30; commands[2][11] = 0x36;
        commands[2][12] = 0x30; commands[2][13] = 0x31; commands[2][14] = 0x31;
        commands[2][15] = 0x30; commands[2][16] = 0x58; commands[2][17] = 0x78;
        commands[2][18] = 0x33; commands[2][19] = 0x30; commands[2][20] = 0x36;
        commands[2][21] = 0x30; commands[2][22] = 0x31; commands[2][23] = 0x31;
        commands[2][24] = 0x30;
        commands[2][25] = 0x58;
        commands[2][26] = 0x78;
        commands[2][27] = 0x34;
        commands[2][28] = 0x30;
        commands[2][29] = 0x36;
        commands[2][30] = 0x30;
        commands[2][31] = 0x31;
        commands[2][32] = 0x31;
        commands[2][33] = 0x30;
        commands[2][34] = 0x58;
        commands[2][35] = 0x78;
        commands[2][36] = 0x35;
        commands[2][37] = 0x30;
        commands[2][38] = 0x36;
        commands[2][39] = 0x30;
        commands[2][40] = 0x31;
        commands[2][41] = 0x31;
        commands[2][42] = 0x30;
        commands[2][43] = 0x58;
        commands[2][44] = 0x78;
        commands[2][45] = 0x36;
        commands[2][46] = 0x30;
        commands[2][47] = 0x36;
        commands[2][48] = 0x30;
        commands[2][49] = 0x31;
        commands[2][50] = 0x31;
        commands[2][51] = 0x30;
        commands[2][52] = 0x58;
        commands[2][53] = 0x78;
        commands[2][54] = 0x37;
        commands[2][55] = 0x30;
        commands[2][56] = 0x36;
        commands[2][57] = 0x30;
        commands[2][58] = 0x31;
        commands[2][59] = 0x31;
        commands[2][60] = 0x30;
        commands[2][61] = 0x58;
        commands[2][62] = 0x78;
        commands[2][63] = 0x38;
        commands[2][64] = 0x30;
        commands[2][65] = 0x36;
        commands[2][66] = 0x30;
        commands[2][67] = 0x31;
        commands[2][68] = 0x31;
        commands[2][69] = 0x30;
        commands[2][70] = 0x58;
        port->write(commands[2]);
        port->write("b");
        port->waitForReadyRead(1000);
    }
}

// 帧头：C0 A0
// 帧尾：6个0
void DataProcessThread::getDataFromShanxi()
{
    while(client->hasPendingDatagrams())
    {
        char bytes[72];
        int size = client->readDatagram(bytes, 72);
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
                        _mutex.lock();
                        data[curChannel] = _turnBytes2uV(chs);
                        if(isRec)
                        {
                            /*写入缓存txt文件*/
                            if(curChannel < this->channels_num - 1)
                                samplesWrite << data[curChannel] << " ";
                            else
                            {
                                samplesWrite << data[curChannel] << std::endl;
                                ++cnt;
                            }
                        }
                         _mutex.unlock();
                        ++curChannel;
                    }
                    processData();
                    i += 39;
                }
            }
        }
    }
}

// 帧头：C0 A0
// 帧尾：6个0
void DataProcessThread::getDataFromShanghai()
{
    int cur_channel = 0, state_machine = 0;
    char ch = 0, single_num[3];
    // 有限状态机
    while(port->read(&ch,1))
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
                if(isRec)
                {
                    _mutex.lock();
                    for(int si = 0; si < channels_num; si++)
                    {
                        /*写入缓存txt文件*/
                        if(si < this->channels_num - 1)
                            samplesWrite << data[si] << " ";
                        else
                            samplesWrite << data[si] << std::endl;
                    }
                    _mutex.unlock();
                    ++cnt;
                }
                processData();
            }
            if(!((state_machine - 3) % 3) && (state_machine > 3))
            {
                _mutex.lock();
                data[cur_channel++] = _turnBytes2uV(single_num[0], single_num[1], single_num[2]);
                _mutex.unlock();
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

double DataProcessThread::_turnBytes2uV(char byte1, char byte2, char byte3)
{
    int target = ((int)byte1 << 16) + (((int)byte2 << 8) & 0x0000ffff) + ((int)byte3 & 0x000000ff);
    return (double)target * cofe;
}

double DataProcessThread::_turnBytes2uV(unsigned char *bytes)
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

void DataProcessThread::processData()
{
    // 原始数据塞进缓存区
    for(int i = 0; i < channels_num; ++i)
    {
        if(averBuffer[i].size() >= 100)
        {
            double aver = sum[i] / 100.0, tmp = averBuffer[i].front().y();
            averBuffer[i].dequeue();
            sum[i] -= tmp;
            sum[i] += data[i];
            averBuffer[i].enqueue(QPointF(cnt, data[i]));
            averData[i] = data[i] - aver;
        }else{
            sum[i] += data[i];
            averBuffer[i].enqueue(QPointF(cnt, data[i]));
        }
    }
    if(isFilt)
    {
        // 滤波
        for(std::size_t i = 0; i < averData.size(); i++)
        {
            // 原始数据进入带通滤波缓冲区
            if(bandPassBuffer[i].size() < FILTER_ORDER + 1)
            {
                bandPassBuffer[i].enqueue(averData[i]);
            }
            // 带通滤波
            else
            {
                // 计算滤波后的值
                filtData[i] = conv(BandPass, i);
                // 队列左移一位
                bandPassBuffer[i].dequeue();
                // 原始值入队
                bandPassBuffer[i].enqueue(averData[i]);
                // 带通滤波后的值入队
//                if(notchBuffer[i].size() < FILTER_ORDER + 1)
//                {
//                    notchBuffer[i].enqueue(bp_y_n);
//                    filtData[i] = bp_y_n;
//                }
//                else
//                {
//                    double y_n = conv(Notch, i);
//                    notchBuffer[i].dequeue();
//                    notchBuffer[i].enqueue(bp_y_n);
//                    filtData[i] = y_n;
//                }
                if(!filt_flag)
                    emit inFilt();
                filt_flag = 1;
            }
        }
        // 发送滤波后的数据
        emit sendData(filtData);
    }
    else
    {
        // 不滤波，直接发送原始数据
        emit sendData(averData);
    }
}

void DataProcessThread::startRec(std::string tempFile)
{
    samplesWrite.open(tempFile + "_samples.txt");
    samplesWrite.close();
    samplesWrite.open(tempFile + "_samples.txt", std::ios::app);
    for(int i = 0; i < channels_num; i++)
    {
        if(i < channels_num - 1)
            samplesWrite << i + 1 << " ";
        else
            samplesWrite << i + 1 << std::endl;
    }
    eventsWrite.open(tempFile + "_events.txt");
    eventsWrite.close();
    eventsWrite.open(tempFile + "_events.txt", std::ios::app);
    eventsWrite << "latency type" << std::endl;
    isRec = true;
}

void DataProcessThread::doEvent(std::string event)
{
    double secs = static_cast<double>(cnt) / sp;
    unsigned long long run_time = 10000 * secs;
    // 写入缓存txt文件
    eventsWrite << run_time << " " + event << std::endl;
}

void DataProcessThread::stopRec()
{
    isRec = false;
    samplesWrite.close();
    eventsWrite.close();
}

void DataProcessThread::startFilt(int lowCut, int highCut, int notchCut)
{
    filt_flag = 0;
    isFilt = true;
    for(std::size_t i = 0; i < data.size(); i++)
    {
        // 计算带通滤波器冲激响应
        bandPassCoff = new double[FILTER_ORDER + 1];
        Filter::countBandPassCoef(FILTER_ORDER, sp, bandPassCoff, lowCut, highCut);
        // 计算陷波器冲激响应
        notchCoff = new double[FILTER_ORDER + 1];
        Filter::countNotchCoef(FILTER_ORDER, sp, notchCoff, notchCut);
    }
}

// 时域序列卷积
double DataProcessThread::conv(FilterType type, std::size_t index)
{
    auto conv_impl = [](const double* coff, const QQueue<double>& data)->double
    {
        double y_n = 0.0;
        for(int k = 0; k <= FILTER_ORDER; k++)
        {
            y_n += coff[k] * data[FILTER_ORDER - k];
        }
        return y_n;
    };
    return (type == BandPass) ? conv_impl(bandPassCoff, bandPassBuffer[index]) : conv_impl(notchCoff, notchBuffer[index]);
}
