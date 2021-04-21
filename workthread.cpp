#include "workthread.h"

DataProcessThread::DataProcessThread(int channels_num, int sampleRate, BoardType b, QString c)
    : sp(sampleRate), isFilt(false), isRec(false), board(b), com(c)
{
    this->channels_num = channels_num;
    for(int i = 0; i < this->channels_num; i++)
    {
        data.push_back(0.0);
        filtData.push_back(0.0);
        bandPassBuffer.push_back(QQueue<double>());
        notchBuffer.push_back(QQueue<double>());
        bandPassCoff.push_back(new double[channels_num]);
        notchCoff.push_back(new double[channels_num]);
    }
    qRegisterMetaType<std::vector<double> >("std::vector<double>");
    boardInit();
}

DataProcessThread::~DataProcessThread()
{
//    process->kill();
//    delete process;
    for(std::size_t i = 0; i < bandPassCoff.size(); i++)
    {
        delete []bandPassCoff[i];
        delete []notchCoff[i];
    }
}

void DataProcessThread::run()
{
    d = new QTimer();
    d->setInterval(50);
    connect(d, SIGNAL(timeout()), this, SLOT(processData()));
    d->start();
    //启动子线程消息循环
    this->exec();
}

void DataProcessThread::boardInit()
{
    if(board == Shanxi)
    {
//        process->start("/dist/shanxi.exe");
//        process->waitForStarted();  // 等待程序确实启动
//        this->sleep(5);  // 等待服务器开启
        client = new QUdpSocket(this);
        client->abort();
        client->bind(QHostAddress(getLocalIP()), 4000);
        connect(client, SIGNAL(readyRead()), this, SLOT(getDataFromShanxi()));
        std::cout << "UDP client open" << std::endl;
        qtime.start();
        std::cout << "Ready for reading data" << std::endl;
    }
    else
    {
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
        commands[0].resize(2);
        commands[0][0] = 0x73;
        commands[0][1] = 0x76;
        port->write(commands[0]);
        commands[1].resize(5);
        commands[1][0] = 0x2f;
        commands[1][1] = 0x30;
        commands[1][2] = 0x7e;
        commands[1][3] = 0x36;
        commands[1][4] = 0x78;
        port->write(commands[1]);
        commands[2].resize(71);
        commands[2][0] = 0x31;
        commands[2][1] = 0x30;
        commands[2][2] = 0x36;
        commands[2][3] = 0x30;
        commands[2][4] = 0x31;
        commands[2][5] = 0x31;
        commands[2][6] = 0x30;
        commands[2][7] = 0x58;
        commands[2][8] = 0x78;
        commands[2][9] = 0x32;
        commands[2][10] = 0x30;
        commands[2][11] = 0x36;
        commands[2][12] = 0x30;
        commands[2][13] = 0x31;
        commands[2][14] = 0x31;
        commands[2][15] = 0x30;
        commands[2][16] = 0x58;
        commands[2][17] = 0x78;
        commands[2][18] = 0x33;
        commands[2][19] = 0x30;
        commands[2][20] = 0x36;
        commands[2][21] = 0x30;
        commands[2][22] = 0x31;
        commands[2][23] = 0x31;
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

QString DataProcessThread::getLocalIP()
{
    QString hostName = QHostInfo::localHostName();//本地主机名
    QHostInfo hostInfo = QHostInfo::fromName(hostName);
    QString  localIP = "";
    QList<QHostAddress> addList = hostInfo.addresses();
    for (int i = 0;i < addList.count(); i++)
    {
        QHostAddress aHost = addList.at(i);
        if (QAbstractSocket::IPv4Protocol == aHost.protocol())
        {
            localIP = aHost.toString();
            break;
        }
    }
    return localIP;
}

// 帧头：C0 A0
// 帧尾：6个0
void DataProcessThread::getDataFromShanxi()
{
    while(client->hasPendingDatagrams())
    {
        char bytes[40];
        int size = client->readDatagram(bytes, 40);
        for(int i = 0; i < size; i++)
        {
            if((i < size - 1)
                    && (bytes[i] == (char)0xc0) && (bytes[i + 1] == (char)0xa0))
            {
                if((i < size - 39)
                        && (bytes[i + 34] == (char)0x00) && (bytes[i + 35] == (char)0x00)
                        && (bytes[i + 36] == (char)0x00) && (bytes[i + 37] == (char)0x00)
                        && (bytes[i + 38] == (char)0x00) && (bytes[i + 39] == (char)0x00))
                {
                    int curChannel = 0;
                    for(int offset = 2; offset < 34; offset += 4)
                    {
                        data[curChannel] = _turnBytes2uV(bytes[i + offset], bytes[i + offset + 1],
                                bytes[i + offset + 2], bytes[i + offset + 3]);
                        if(isRec)
                        {
                            /*写入缓存txt文件*/
                            if(curChannel < this->channels_num - 1)
                                samplesWrite << data[curChannel] << " ";
                            else
                                samplesWrite << data[curChannel] << std::endl;
                        }
                        ++curChannel;
                    }
                    ++cnt;
                    std::cout << "MATCH " << qtime.elapsed() / 1000.0 << " " << cnt << std::endl;
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
    while(port->read(&ch,1))
    {
        if(isRec)   std::cout << std::hex << (unsigned int)(unsigned char)ch << " ";
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
//                ++cnt;
//                std::cout << "MATCH, " << "Real receive = " << cnt << std::endl;
                cur_channel = 0;
                if(isRec)
                {
                    for(int si = 0; si < channels_num; si++)
                    {
                        /*写入缓存txt文件*/
                        if(si < this->channels_num - 1)
                            samplesWrite << data[si] << " ";
                        else
                            samplesWrite << data[si] << std::endl;
                    }
                }
            }
            if(!((state_machine - 3) % 3) && (state_machine > 3))
                data[cur_channel++] = _turnBytes2uV(single_num[0], single_num[1], single_num[2]);
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

double DataProcessThread::_turnBytes2uV(unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4)
{
    int i, exponent;
    unsigned int sign, mantissa;
    float curNum = 0.0;
    std::string str = "";
    sign = (byte4 & 0x80) >> 7;
    exponent = (((byte4 & 0x7f) << 1) + ((byte3 & 0x80) >> 7)) - 127;
    mantissa = ((byte3 & 0x7f) << 16) + (byte2 << 8) + byte1;
    while(mantissa)
    {
        str += ('0' + mantissa % 2);
        mantissa /= 2;
    }
    while(str.length() < 23)
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
    if(isFilt)
    {
        for(std::size_t i = 0; i < data.size(); i++)
        {
            Filter f;
            /*原始数据进入带通滤波缓冲区*/
            if(bandPassBuffer[i].size() < FILTER_ORDER + 1)
            {
                bandPassBuffer[i].enqueue(data[i]);
            }
            /*带通滤波*/
            else
            {
                /*计算滤波后的值*/
                double bp_y_n = conv(BandPass, i);
                /*队列左移一位*/
                bandPassBuffer[i].dequeue();
                /*原始值入队*/
                bandPassBuffer[i].enqueue(data[i]);
                /*带通滤波后的值入队*/
                if(notchBuffer[i].size() < FILTER_ORDER + 1)
                {
                    notchBuffer[i].enqueue(bp_y_n);
                    data[i] = bp_y_n;
                }
                else
                {
                    double y_n = conv(Notch, i);
                    notchBuffer[i].dequeue();
                    notchBuffer[i].enqueue(bp_y_n);
                    data[i] = y_n;
                }
                if(!filt_flag)
                    emit inFilt();
                filt_flag = 1;
            }
        }
    }
    emit sendData(data);
}

void DataProcessThread::startRec(std::string tempFile)
{
    samplesWrite.open(tempFile);
    samplesWrite.close();
    samplesWrite.open(tempFile, std::ios::app);
    for(int i = 0; i < channels_num; i++)
    {
        if(i < channels_num - 1)
            samplesWrite << i + 1 << " ";
        else
            samplesWrite << i + 1 << std::endl;
    }
    isRec = true;
}

void DataProcessThread::stopRec()
{
    isRec = false;
    samplesWrite.close();
}

void DataProcessThread::startFilt(int lowCut, int highCut, int notchCut)
{
    filt_flag = 0;
    isFilt = true;
    Filter f;
    for(std::size_t i = 0; i < data.size(); i++)
    {
        /*计算带通滤波器冲激响应*/
        double *cur_bp_h = new double[FILTER_ORDER + 1];
        f.countBandPassCoef(FILTER_ORDER, sp, cur_bp_h, lowCut, highCut);
        bandPassCoff[i] = cur_bp_h;
        /*计算陷波器冲激响应*/
        double *cur_n_h = new double[FILTER_ORDER + 1];
        f.countNotchCoef(FILTER_ORDER, sp, cur_n_h, notchCut);
        notchCoff[i] = cur_n_h;
    }
}

/*时域序列卷积*/
double DataProcessThread::conv(FilterType type, int index)
{
    double y_n = 0.0;
    if(type == BandPass)
    {
        for(int k = 0; k <= FILTER_ORDER; k++)
        {
            y_n += bandPassCoff[index][k] * bandPassBuffer[index][FILTER_ORDER - k];
        }
    }
    else
    {
        for(int k = 0; k <= FILTER_ORDER; k++)
        {
            y_n += notchCoff[index][k] * notchBuffer[index][FILTER_ORDER - k];
        }
    }
    return y_n;
}
