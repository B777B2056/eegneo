#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QString participantNum, QString date, QString others, QString expName, int cn, QWidget *parent)
    : QMainWindow(parent)
    , lowCut(-1.0), highCut(-1.0), notchCut(-1.0)
    , maxVoltage(50), threshold(1000 * TIME_INTERVAL / GRAPH_FRESH)
    , eventCount(0), curLine(0), isRec(false), isSaveP300BH(false), isFinish(-1), tempFiles("")
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /*子线程初始化并开始*/
    dataThread = new DataThread(cn);
    dataThread->start();
    /*被试信息初始化*/
    this->participantNum = participantNum;
    this->date = date;
    this->others = others;
    this->expName = expName;
    this->channel_num = cn;
    this->setWindowTitle("EEG信号采集平台@被试编号：" + this->participantNum);
    tempFiles += (participantNum.toStdString()+'_'+date.toStdString()+'_'+others.toStdString());
    /*滤波信号灯初始化为红色：未滤波*/
    ui->label_5->setStyleSheet("QLabel{background:#FF0000;}");
    /*电极数组初始化*/
    montages[0] = ui->wave;
    montages[1] = ui->wave_2;
    montages[2] = ui->wave_3;
    montages[3] = ui->wave_4;
    montages[4] = ui->wave_5;
    montages[5] = ui->wave_6;
    montages[6] = ui->wave_7;
    montages[7] = ui->wave_8;
    montages[8] = ui->wave_9;
    montages[9] = ui->wave_10;
    montages[10] = ui->wave_11;
    montages[11] = ui->wave_12;
    montages[12] = ui->wave_13;
    montages[13] = ui->wave_14;
    montages[14] = ui->wave_15;
    montages[15] = ui->wave_16;
    if(channel_num == 8)
    {
        for(int i = 8; i < 16; i++)
            montages[i]->hide();
    }
    /*动态分配内存*/
    markerNames = new std::string[MANUAL_MAKER];
    /*初始化vector*/
    for(int i = 0; i < channel_num; i++)
    {
        impedance.push_back(0);
        graphData.push_back(0.0);
        channelNames.push_back("");
        pointQueue.push_back(QQueue<QPointF>());
        impDisplay.push_back(new QLabel(this));
        series.push_back(new QSplineSeries);
        axisX.push_back(new QDateTimeAxis);
        axisY.push_back(new QValueAxis);
        charts.push_back(new QChart);
    }
    /*定时器初始化*/
    graphTimer = new QTimer(this);
    graphTimer->setInterval(GRAPH_FRESH);//设置定时周期，单位：毫秒
    graphTimer->start();
    impTimer = new QTimer(this);
    impTimer->setInterval(IMPEDANCE_FRESH);
    impTimer->start();
    /*绘图板初始化*/
    initChart();
    /*信号与槽的链接*/
    connect(graphTimer, SIGNAL(timeout()), this, SLOT(graphFresh()));
    connect(impTimer, SIGNAL(timeout()), this, SLOT(getImpedanceFromBoard()));
    connect(ui->actionStart_Recording, SIGNAL(triggered()), this, SLOT(createTempTXT()));
    connect(ui->actionEDF, SIGNAL(triggered()), this, SLOT(saveEDF()));
    connect(ui->actionTXT, SIGNAL(triggered()), this, SLOT(saveTXT()));
    connect(ui->actionStop_Recording, SIGNAL(triggered()), this, SLOT(stopRec()));
    connect(ui->actionp300oddball, SIGNAL(triggered()), this, SLOT(p300Oddball()));
    connect(ui->action50uV, SIGNAL(triggered()), this, SLOT(setVoltage50()));
    connect(ui->action100uV, SIGNAL(triggered()), this, SLOT(setVoltage100()));
    connect(ui->action200uV, SIGNAL(triggered()), this, SLOT(setVoltage200()));
    connect(dataThread, SIGNAL(sendData(std::vector<double>)), this, SLOT(receiveData(std::vector<double>)));
    connect(dataThread, SIGNAL(inFilt()), this, SLOT(isInFilt()));
    connect(this, SIGNAL(doFilt(int, int, int)), dataThread, SLOT(startFilt(int, int, int)));
    connect(this, SIGNAL(doRec(std::string)), dataThread, SLOT(startRec(std::string)));
    connect(this, SIGNAL(doneRec()), dataThread, SLOT(stopRec()));    
}

MainWindow::~MainWindow()
{
    /*删除缓存文件*/
    remove((tempFiles + "_samples.txt").c_str());
    remove((tempFiles + "_events.txt").c_str());
    /*释放内存*/
    std::vector<QLineSeries *> qls;
    std::map<QLineSeries *, std::pair<qint64, QGraphicsSimpleTextItem *>>::iterator iter;
    for(iter = marks.begin(); iter != marks.end(); iter++)
    {
        qls.push_back(iter->first);
        QGraphicsSimpleTextItem *t = (iter->second).second;
        (iter->second).second = nullptr;
        delete t;
    }
    marks.clear();
    for(std::size_t i = 0; i < qls.size(); i++)
        delete qls[i];
    for(int i = 0; i < channel_num; i++)
    {
        delete impDisplay[i];
        delete series[i];
        delete axisX[i];
        delete axisY[i];
        delete charts[i];
    }
    delete []markerNames;
    delete graphTimer;
    delete impTimer;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(this->isFinish == 0)
    {
        QMessageBox::critical(this, tr("错误"), "EDF文件正在写入！", QMessageBox::Ok);
        event->ignore();
    }
    else
        event->accept();
}

/*创建缓存TXT文件*/
void MainWindow::createTempTXT()
{
    eventsWrite.open(tempFiles + "_events.txt");
    eventsWrite.close();
    eventsWrite.open(tempFiles + "_events.txt", std::ios::app);
    eventsWrite << "latency type" << std::endl;
    isRec = true;
    emit doRec(tempFiles + "_samples.txt");
}

/*设置文件保存的路径*/
void MainWindow::setFilePath(int s, std::string& path)
{
    //新建对话框获取EDF文件放置的绝对地址
    path = QFileDialog::getExistingDirectory(this, tr("文件保存路径选择"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                               | QFileDialog::DontResolveSymlinks).toStdString();
    path += ("/"+participantNum.toStdString()+'_'+date.toStdString()+'_'+others.toStdString());
    QString q = QString::fromStdString(path);
    q.replace("/", "\\");
    path = q.toStdString();
    //询问用户是否采用默认的通道名称(仅EDF可用)
    if(!s)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("通道名称"),
                                        "是否自定义通道名称?\n默认名称为数字编号，例如1~8。",
                                        QMessageBox::Ok | QMessageBox::No);
        if (reply == QMessageBox::Ok){
            /*弹窗口让用户输入各通道名称*/
            SetChannelName scl;
      Again:
            int rec = scl.exec();
            if(rec == QDialog::Accepted)
            {
                for(int i = 0; i < channel_num; i++)
                    channelNames[i] = scl.names[i].toStdString();
            }
            else
            {
                goto Again;
            }
        }
        else{
            /*各通道默认标签*/
            for(int i = 0; i < channel_num; i++)
                channelNames[i] = std::to_string(i + 1);
        }
    }
    else
    {
        /*各通道默认标签*/
        for(int i = 0; i < channel_num; i++)
            channelNames[i] = std::to_string(i + 1);
    }
}

/*创建EDF文件*/
void MainWindow::saveEDF()
{
    QFileInfo samples_file(QString::fromStdString(tempFiles) + "_samples.txt");
    QFileInfo events_file(QString::fromStdString(tempFiles) + "_events.txt");
    if(!samples_file.isFile() || !events_file.isFile())
    {
        QMessageBox::critical(this, tr("错误"), "数据未记录，无法导出！", QMessageBox::Ok);
        return;
    }
    isFinish = 0;
    int i, col = 0;
    std::string edf_path;
    std::ifstream samples_read;  // 8通道缓存txt文件输入流
    std::ifstream events_read;  // 标记缓存txt文件输入流
    double *buf_persec = new double[channel_num * SAMPLE_RATE];
    std::string file_name = participantNum.toStdString()+'_'+date.toStdString()+'_'+others.toStdString();
    setFilePath(0, edf_path);
    //新建文件夹
    QDir dir;
    if (!dir.exists(QString::fromStdString(edf_path)))
    {
        dir.mkpath(QString::fromStdString(edf_path));

    }
    //设置EDF文件参数
    flag = edfopen_file_writeonly((edf_path + "\\" + file_name + ".edf").c_str(), EDFLIB_FILETYPE_EDFPLUS, channel_num);
    for(i = 0; i < channel_num; i++)
    {
        //设置各通道采样率
        edf_set_samplefrequency(flag, i, SAMPLE_RATE);
        //设置信号最大与最小数字值(EDF为16位文件，一般设置为-32768~32767)
        edf_set_digital_maximum(flag, i, 32767);
        edf_set_digital_minimum(flag, i, -32768);
        //设置信号最大与最小物理值(即信号在物理度量上的最大、最小值)
        edf_set_physical_maximum(flag, i, maxVoltage);
        edf_set_physical_minimum(flag, i, -maxVoltage);
        //设置各通道数据的单位
        edf_set_physical_dimension(flag, i, "uV");
        //设置各通道名称
        edf_set_label(flag, i, channelNames[i].data());
    }
    //从缓存txt文件中读取数据，并写入edf文件
    samples_read.open(tempFiles + "_samples.txt");
    events_read.open(tempFiles + "_events.txt");
    //写入数据
    while(samples_read.peek() != EOF)
    {
        if(col)
        {
            std::string str;
            std::getline(samples_read, str);
            std::stringstream ss(str);
            for(int row = 0; row < SAMPLE_RATE; row++)
            {
              if(SAMPLE_RATE * row + col < channel_num * SAMPLE_RATE)
                  ss >> buf_persec[SAMPLE_RATE * row + col];
            }
            /*1s结束*/
            if(!((col) % SAMPLE_RATE))
            {
                edf_blockwrite_physical_samples(flag, buf_persec);
                col = 0;
            }
        }
        ++col;
    }
    /*写入多余的空数据以保证标记存在*/
    for(i = 0; i < channel_num * SAMPLE_RATE; i++)
       buf_persec[i] = 0.0;
    for(i = 0; i < eventCount - curLine / SAMPLE_RATE + 1; i++)
        edf_blockwrite_physical_samples(flag, buf_persec);
    delete []buf_persec;
    //写入事件
    while(events_read.peek() != EOF)
    {
        long long run_time;
        std::string str, event;
        std::getline(events_read, str);
        std::stringstream ss(str);
        ss >> run_time >> event;
        std::cout << "Time: " << run_time << ", Event: " << event << std::endl;
        edfwrite_annotation_utf8(flag, run_time, 10, event.c_str());
    }
    //关闭文件流
    samples_read.close();
    events_read.close();
    //关闭edf文件
    edfclose_file(flag);
    /*保存行为学数据*/
    if(isSaveP300BH)
        saveBehavioralP300(edf_path);
    //EDF文件写入完成
    isFinish = 1;
}

/*保存为3个txt文档（样本数据点，事件信息，描述文档）*/
void MainWindow::saveTXT()
{
    QFileInfo samples_file(QString::fromStdString(tempFiles) + "_samples.txt");
    QFileInfo events_file(QString::fromStdString(tempFiles) + "_events.txt");
    if(!samples_file.isFile() || !events_file.isFile())
    {
        QMessageBox::critical(this, tr("错误"), "数据未记录，无法导出！", QMessageBox::Ok);
        return;
    }
    isFinish = 0;
    std::string txt_path;
    std::string file_name = participantNum.toStdString()+'_'+date.toStdString()+'_'+others.toStdString();
    std::ifstream samples_read;  // 8通道缓存txt文件输入流
    std::ofstream samples_txt;  // 数据点txt文件输出流
    std::ifstream events_read;
    std::ofstream events_txt_eeglab;  // eeglab风格
    std::ofstream events_txt_brainstorm;  // brainstorm风格
    setFilePath(1, txt_path);
    /*新建文件夹，把三个txt放入一个文件夹*/
    QDir dir;
    if (!dir.exists(QString::fromStdString(txt_path)))
    {
        dir.mkpath(QString::fromStdString(txt_path));

    }
    /*写入数据点*/
    int col = 0;
    samples_txt.open(txt_path + "\\" + file_name + "_samples.txt");
    samples_txt.close();
    samples_read.open(tempFiles + "_samples.txt");
    samples_txt.open(txt_path + "\\" + file_name + "_samples.txt", std::ios::app);
    for(int i = 0; i < channel_num; i++)
    {
        if(i < channel_num - 1)
            samples_txt << channelNames[i] << " ";
        else
            samples_txt << channelNames[i] << std::endl;
    }
    while(samples_read.peek() != EOF)
    {
        std::string str;
        std::getline(samples_read, str);
        if(col)
        {
            samples_txt << str << std::endl;
        }
        ++col;
    }
    samples_txt.close();
    samples_read.close();
    /*写入事件(EEGLAB风格)*/
    col = 0;
    events_txt_eeglab.open(txt_path + "\\" + file_name + "_events(eeglab).txt");
    events_txt_eeglab.close();
    events_read.open(tempFiles + "_events.txt");
    events_txt_eeglab.open(txt_path + "\\" + file_name + "_events(eeglab).txt", std::ios::app);
    while(events_read.peek() != EOF)
    {
        std::string str;
        std::getline(events_read, str);
        if(col)
        {
            long long run_time;
            std::string event;
            std::stringstream ss(str);
            ss >> run_time >> event;
            events_txt_eeglab << (double)run_time / 10000.0 << " " << event << std::endl;
        }
        else
        {
            events_txt_eeglab << str << std::endl;
        }
        ++col;
    }
    events_txt_eeglab.close();
    events_read.close();
    /*写入事件(Brainstorm风格)*/
    col = 0;
    events_txt_brainstorm.open(txt_path + "\\" + file_name + "_events(brainstorm).txt");
    events_txt_brainstorm.close();
    events_read.open(tempFiles + "_events.txt");
    events_txt_brainstorm.open(txt_path + "\\" + file_name + "_events(brainstorm).txt", std::ios::app);
    while(events_read.peek() != EOF)
    {
        std::string str;
        std::getline(events_read, str);
        if(col)
        {
            long long run_time;
            std::string event;
            std::stringstream ss(str);
            ss >> run_time >> event;
            events_txt_brainstorm << event << "," << (double)run_time / 10000.0 << "," << 0.1 << std::endl;
        }
        ++col;
    }
    events_txt_brainstorm.close();
    events_read.close();
    /*写描述性文件文件*/
    std::ofstream readme;
    readme.open(txt_path + "\\" + file_name + "_readme.txt");
    readme.close();
    readme.open(txt_path + "\\" + file_name + "_readme.txt", std::ios::app);
    readme << "Data sampling rate: " << SAMPLE_RATE << std::endl;
    readme << "Number of channels: " << channel_num << std::endl;
    readme << "Input field(column) names: latency type" << std::endl;
    readme << "Number of file header lines: 1" << std::endl;
    readme << "Time unit(sec): 1" << std::endl;
    readme.close();
    /*保存行为学数据*/
    if(isSaveP300BH)
        saveBehavioralP300(txt_path);
    isFinish = 1;
}

/*保存行为学数据*/
void MainWindow::saveBehavioralP300(std::string path)
{
    int col = 0;
    std::ifstream events_read;
    std::vector<int> blank_acc, blank_rt, blank_resp, on_set_time, blank_rttime;
    std::vector<std::string> code;
    std::vector<std::pair<double, std::string>> vec;
    events_read.open(tempFiles + "_events.txt");
    while(events_read.peek() != EOF)
    {
        std::string str;
        std::getline(events_read, str);
        if(col)
        {
            long long run_time;
            std::string event;
            std::stringstream ss(str);
            ss >> run_time >> event;
            vec.push_back(std::make_pair(run_time, event));
        }
        ++col;
    }
    for(std::size_t i = 0; i < vec.size() - 1; ++i)
    {
        if(vec[i].second == "2")
        {
            if(vec[i + 1].second == "Response")
            {
                //对2有反应，说明反应正确
                blank_acc.push_back(1);
                //计算反应时间，单位为毫秒，在表格中对应2的位置
                blank_rt.push_back((int)((vec[i + 1].first - vec[i].first) / 10));
                //对2有反应，按键正确
                blank_resp.push_back(1);
                //反应动作的时间，毫秒
                blank_rttime.push_back((int)(vec[i + 1].first / 10));
            }
            else
            {
                blank_acc.push_back(0);
                blank_resp.push_back(0);
                blank_rt.push_back(0);
                blank_rttime.push_back(0);
            }
            code.push_back("2");
            on_set_time.push_back((int)(vec[i].first / 10));
        }
        else if(vec[i].second == "8")
        {
            //对8有反应，说明反应错误
            if(vec[i + 1].second == "Response")
            {
                blank_acc.push_back(0);
                blank_resp.push_back(1);
                blank_rt.push_back((int)((vec[i + 1].first - vec[i].first) / 10));
                blank_rttime.push_back((int)(vec[i + 1].first / 10));
            }
            //对8无反应，说明反应正确
            else
            {
                blank_acc.push_back(1);
                blank_resp.push_back(-1);
                blank_rt.push_back(-1);
                blank_rttime.push_back(-1);
            }
            code.push_back("8");
            on_set_time.push_back((int)(vec[i].first / 10));
        }
    }
    events_read.close();
    std::string file_name = participantNum.toStdString()+'_'+date.toStdString()+'_'+others.toStdString();
    std::ofstream beh_file;
    beh_file.open(path + "\\" + file_name + "_behavioral.csv");
    beh_file.close();
    beh_file.open(path + "\\" + file_name + "_behavioral.csv", std::ios::app);
    beh_file << "Experiment Name: " << expName.toStdString()
             << ", Date: " << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss").toStdString()
             << std::endl;
    beh_file << "Subject,Trial,blank.ACC,blank.CRESP,blank.OnsetTime,blank.RESP,blank.RT,blank.RTTime,code,correct,pic" << std::endl;
    //预实验
    for(int i = 0; i < 40; i++)
    {
        std::string pre_exp = participantNum.toStdString() + "," + std::to_string(i + 1);
        for(int j = 0; j < 9; j++)
        {
            pre_exp += ",";
        }
        beh_file << pre_exp << std::endl;
    }
    //正式实验
    for(int i = 0; i < p300OddballImgNum; i++)
    {
        //Subject
        beh_file << participantNum.toStdString();
        beh_file << ",";
        //Trial
        beh_file << std::to_string(i + 1);
        beh_file << ",";
        //blank.ACC
        beh_file << blank_acc[i];
        beh_file << ",";
        //blank.CRESP
        if(code[i] == "2")
            beh_file << "1";
        beh_file << ",";
        //blank.OnsetTime
        if(code[i] == "2")
            beh_file << on_set_time[i];
        beh_file << ",";
        //blank.RESP
        if(blank_resp[i] == 1)
            beh_file << blank_resp[i];
        beh_file << ",";
        //blank.RT
        if(code[i] == "2" || blank_resp[i] == 1)
            beh_file << blank_rt[i];
        else
            beh_file << 0;
        beh_file << ",";
        //blank.RTTime
        if(code[i] == "2" || blank_resp[i] == 1)
            beh_file << blank_rttime[i];
        else
            beh_file << 0;
        beh_file << ",";
        //code
        beh_file << code[i];
        beh_file << ",";
        //correct
        if(code[i] == "2")
            beh_file << "1";
        beh_file << ",";
        //pic
        if(code[i] == "2")
            beh_file << "2.bmp" << std::endl;
        else
            beh_file << "8.bmp" << std::endl;
    }
    beh_file.close();
}

/*发送marker*/
void MainWindow::createMark(const std::string event)
{
    if(event.empty()) return;
    /*添加新的marker直线*/
    QLineSeries *line = new QLineSeries;
    QPen splinePen;//设置直线的颜色
    splinePen.setBrush(Qt::red);
    splinePen.setColor(Qt::red);
    line->setPen(splinePen);
    charts[0]->addSeries(line);
    charts[0]->setAxisX(axisX[0], line);
    charts[0]->setAxisY(axisY[0], line);
    /*添加marker文字注释*/
    QGraphicsSimpleTextItem *pItem = new QGraphicsSimpleTextItem(charts[0]);
    pItem->setText(QString::fromStdString(event) + "\n" + QDateTime::currentDateTime().toString("hh:mm:ss"));
    marks[line] = std::make_pair(QDateTime::currentDateTime().toMSecsSinceEpoch(), pItem);
    /*将marker写入文件*/
    if(isRec)
    {
        eventCount++;
        double secs = 10000 * curLine / SAMPLE_RATE;
        long long run_time = secs;
        /*写入缓存txt文件*/
        eventsWrite << run_time << " " + event << std::endl;
    }
}

void MainWindow::initChart()
{
    for(int index = 0; index < channel_num; index++)
    {
        //设置x轴
        axisX[index]->setRange(QDateTime::currentDateTime().addSecs(-TIME_INTERVAL), QDateTime::currentDateTime());
        axisX[index]->setTickCount(TIME_INTERVAL);
        axisX[index]->setFormat("hh:mm:ss");
        charts[index]->addAxis(axisX[index], Qt::AlignBottom);
        //设置y轴
        axisY[index]->setRange(-50, 50);
        axisY[index]->setTickCount(2);
        axisY[index]->setTitleText("Voltage/uV");
        axisY[index]->setLabelFormat("%d");
        charts[index]->addAxis(axisY[index], Qt::AlignLeft);
        //链接数据
        series[index]->setUseOpenGL(true);
        series[index]->append(QPointF(0, 0));
        charts[index]->addSeries(series[index]);
        QPen splinePen;
        splinePen.setBrush(Qt::blue);
        splinePen.setColor(Qt::blue);
        series[index]->setPen(splinePen);
        //设置界面显示
        charts[index]->setAxisX(axisX[index], series[index]);
        charts[index]->setAxisY(axisY[index], series[index]);
        charts[index]->legend()->hide();
        charts[index]->setTheme(QChart::ChartThemeLight);
        charts[index]->setMargins({-10, 0, 0, -10});
        charts[index]->axisX()->setGridLineVisible(false);
        charts[index]->axisY()->setGridLineVisible(false);
        montages[index]->setChart(charts[index]);
        montages[index]->setMaximumSize(QSize(761, 75));
        QLabel *num = new QLabel(this);
        num->setText(QString::number(index + 1));
        num->setMinimumSize(QSize(15, 15));
        num->setMaximumSize(QSize(20, 20));
        QWidget *widget = new QWidget(ui->listWidget);
        QHBoxLayout *horLayout = new QHBoxLayout;
        horLayout->setContentsMargins(0, 0, 0, 0);
        horLayout->setMargin(0);
        horLayout->setSpacing(0);
        horLayout->addWidget(num);
        horLayout->addWidget(montages[index]);
        horLayout->addWidget(impDisplay[index]);
        widget->setLayout(horLayout);
        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        item->setSizeHint(QSize(860, 75));
        ui->listWidget->setItemWidget(item, widget);
    }
}

/*设置Y轴范围*/
void MainWindow::setVoltage50()
{
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-50, 50);
    }
    maxVoltage = 50;
}

void MainWindow::setVoltage100()
{
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-100, 100);
    }
    maxVoltage = 100;
}

void MainWindow::setVoltage200()
{
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-200, 200);
    }
    maxVoltage = 200;
}

/*波形更新*/
void MainWindow::updateWave(const std::vector<double>& channelData)
{
    /*绘制波形*/
    for(int index = 0; index < channel_num; index++)
    {
        /*坐标轴刷新*/
        charts[index]->axisX()->setRange(QDateTime::currentDateTime().addSecs(-TIME_INTERVAL), QDateTime::currentDateTime());
        if(pointQueue[index].size() >= threshold)
        {
            pointQueue[index].dequeue();
        }
        pointQueue[index].enqueue(QPointF(QDateTime::currentDateTime().toMSecsSinceEpoch(), channelData[index]));
        series[index]->replace(pointQueue[index]);
    }

}

void MainWindow::getImpedanceFromBoard()
{
    for(int i = 0; i < channel_num; i++)
    {
    #ifdef NO_BOARD
        impedance[i] = 500.0 * rand() / (RAND_MAX);
    #endif
        if(impedance[i] >= 490)
            impedance[i] = -1;  // 阻抗无穷大
    }
}

/*数据获取，暂用随机数方式代替实际方法*/
void MainWindow::receiveData(std::vector<double> vec)
{
    for(int i = 0; i < channel_num; i++)
        graphData[i] = vec[i];
    if(isRec) curLine++;
}

/*图像刷新*/
void MainWindow::graphFresh()
{
    /*波形刷新*/
    updateWave(graphData);
    /*绘制marker*/
    std::map<QLineSeries *, std::pair<qint64, QGraphicsSimpleTextItem *>>::iterator iter;
    for(iter = marks.begin(); iter != marks.end(); iter++)
    {
        QList<QPointF> marker_point;
        marker_point.append(QPointF((iter->second).first, -maxVoltage));
        marker_point.append(QPointF((iter->second).first, maxVoltage));
        iter->first->replace(marker_point);
        /*文字标记*/
        (iter->second).second->setPos(charts[0]->mapToPosition(QPointF((iter->second).first, 50.0), iter->first));
    }
    /*阻抗刷新*/
    for(int i = 0; i < channel_num; i++)
    {
        background color;  // 背景色（0-20绿色，20-100黄色，100以上红色）
        QString text;
        if(impedance[i] < 0)
        {
            text = "Inf";
            color = Red;
        }
        else
        {
            text = QString::number(impedance[i]) + "KOhm";
            if(impedance[i] <= 20)
                color = Green;
            else if(impedance[i] > 20 && impedance[i] <= 100)
                color = Yellow;
            else
                color = Red;
        }
        impDisplay[i]->setText(text);
        if(color == Green)
            impDisplay[i]->setStyleSheet("QLabel{background:#00FF00;}");
        else if(color == Yellow)
            impDisplay[i]->setStyleSheet("QLabel{background:#DAA520;}");
        else
            impDisplay[i]->setStyleSheet("QLabel{background:#FF0000;}");
    }
}

/*停止写入数据并保存缓存txt文件*/
void MainWindow::stopRec()
{
  emit doneRec();
  //关闭txt文件输入流
  eventsWrite.close();
}

/*p300 Oddball范式*/
void MainWindow::p300Oddball()
{
    //弹窗提示用户本实验相关信息
    QMessageBox::StandardButton reply;
    reply = QMessageBox::information(this, tr("p300-oddball"),
                                    "实验名称：P300诱发电位刺激\n"
                                    "实验范式：Oddball\n"
                                    "实验内容：数字2与数字8交替闪烁。",
                                    QMessageBox::Ok);
    if(reply == QMessageBox::Ok)
    {
        isSaveP300BH = true;
        //进入实验
        P300Oddball *p = new P300Oddball();
        connect(p, SIGNAL(sendImgNum(int)), this, SLOT(getImgNum(int)));
        connect(p, SIGNAL(sendMark(const std::string)), this, SLOT(createMark(const std::string)));
        p->show();
    }
}

/*设置P300-Oddball图片总数量，保存行为学数据时需要*/
void MainWindow::getImgNum(int n)
{
    p300OddballImgNum = n;
}

/*获取已输入的marker*/
void MainWindow::on_lineEdit_editingFinished()
{
    QString m = ui->lineEdit->text();
    if(!m.isEmpty())
        markerNames[0] = m.toStdString();
}

void MainWindow::on_lineEdit_2_editingFinished()
{
    QString m = ui->lineEdit_2->text();
    if(!m.isEmpty())
        markerNames[1] = m.toStdString();
}

void MainWindow::on_lineEdit_3_editingFinished()
{
    QString m = ui->lineEdit_3->text();
    if(!m.isEmpty())
        markerNames[2] = m.toStdString();
}

void MainWindow::on_lineEdit_4_editingFinished()
{
    QString m = ui->lineEdit_4->text();
    if(!m.isEmpty())
        markerNames[3] = m.toStdString();
}

void MainWindow::on_pushButton_2_clicked()
{
    createMark(markerNames[0]);
}

void MainWindow::on_pushButton_3_clicked()
{
    createMark(markerNames[1]);
}

void MainWindow::on_pushButton_4_clicked()
{
    createMark(markerNames[2]);
}

void MainWindow::on_pushButton_5_clicked()
{
    createMark(markerNames[3]);
}

/*选择带通滤波、凹陷滤波频率*/
void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    if(index != 0)
        lowCut = highPassFres[index - 1];
}

void MainWindow::on_comboBox_2_currentIndexChanged(int index)
{
    if(index != 0)
        highCut = lowPassFres[index - 1];
}

void MainWindow::on_comboBox_3_currentIndexChanged(int index)
{
    if(index != 0)
        notchCut = ((index == 1) ? 50.0 : 60.0);
}

/*按了"滤波"按钮后开始滤波*/
void MainWindow::on_filter_clicked()
{
    if(lowCut > 0.0 && highCut > 0.0 && notchCut > 0.0)
    {
        if(lowCut >= highCut)
        {
            QMessageBox::critical(this, tr("错误"), "滤波频率错误！", QMessageBox::Ok);
            return;
        }
        /*滤波*/
        emit doFilt(lowCut, highCut, notchCut);
    }
}

void MainWindow::isInFilt()
{
    ui->label_5->setStyleSheet("QLabel{background:#00FF00;}");
}
