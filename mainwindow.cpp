#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isFilt(false), lowCut(-1.0), highCut(-1.0), notchCut(-1.0)
    , maxVoltage(50), threshold(1000 * TIME_INTERVAL / GRAPH_FRESH)
    , eventCount(0), curLine(0), isRec(false), isSaveP300BH(false), isFinish(-1), tempFiles("")
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /*动态分配内存*/
    impedance = new int[CHANNELS];
    filtData = new double[CHANNELS];
    markerNames = new std::string[MANUAL_MAKER];
    pointQueue = new QQueue<QPointF>[CHANNELS];
    series = new  QSplineSeries[CHANNELS];
    axisX = new QDateTimeAxis[CHANNELS];
    axisY = new QValueAxis[CHANNELS];
    charts = new QChart[CHANNELS];
    originalData = new double[CHANNELS];
    channelNames = new std::string[CHANNELS];
    /*定时器初始化*/
    graphTimer = new QTimer(this);
    graphTimer->setInterval(GRAPH_FRESH);//设置定时周期，单位：毫秒
    graphTimer->start();
    impTimer = new QTimer(this);
    impTimer->setInterval(IMPEDANCE_FRESH);
    impTimer->start();
#ifdef NO_BOARD
    dataTimer = new QTimer(this);
    dataTimer->setInterval(DATA_FRESH);
    dataTimer->start();
    /*随机数初始化*/
    qsrand(QDateTime::currentDateTime().toTime_t());
#endif
    /*绘图板初始化*/
    initChart();
    /*信号与槽的链接*/
    connect(graphTimer, SIGNAL(timeout()), this, SLOT(graphFresh()));
#ifdef NO_BOARD
    connect(dataTimer, SIGNAL(timeout()), this, SLOT(getDataFromBoard()));
#endif
    connect(impTimer, SIGNAL(timeout()), this, SLOT(getImpedanceFromBoard()));
    connect(ui->actionStart_Recording, SIGNAL(triggered()), SLOT(createTempTXT()));
    connect(ui->actionEDF, SIGNAL(triggered()), SLOT(saveEDF()));
    connect(ui->actionTXT, SIGNAL(triggered()), SLOT(saveTXT()));
    connect(ui->actionStop_Recording, SIGNAL(triggered()), SLOT(stopRec()));
    connect(ui->actionp300oddball, SIGNAL(triggered()), SLOT(p300Oddball()));
    connect(ui->action50uV, SIGNAL(triggered()), SLOT(setVoltage50()));
    connect(ui->action100uV, SIGNAL(triggered()), SLOT(setVoltage100()));
    connect(ui->action200uV, SIGNAL(triggered()), SLOT(setVoltage200()));
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
    for(int i = 0; i < CHANNELS; i++)
    {
        delete []bandPassCoff[i];
        delete []notchCoff[i];
    }
    delete []markerNames;
    delete []originalData;
    delete []filtData;
    delete []impedance;
    delete []series;
    delete []pointQueue;
    delete []charts;
    delete []axisX;
    delete []axisY;
    delete []channelNames;
    delete graphTimer;
#ifdef NO_BOARD
    delete dataTimer;
#endif
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

/*设置被试基本信息子窗口*/
void MainWindow::setInfo(){
    SetInfo *siw = new SetInfo;
    siw->setMainWindow(this);
Retry:
    int rec = siw->exec();
    siw->getInfo(participantNum, date, others, expName);
    if(rec == QDialog::Accepted){
        if(participantNum.isEmpty() || date.isEmpty() || expName.isEmpty()){
            /*被试信息必须项缺失，弹出错误信息后返回*/
            QMessageBox::StandardButton reply;
            reply = QMessageBox::critical(this, tr("错误"),
                                            "被试信息缺失！\n请检查被试编号、日期与实验名称。",
                                            QMessageBox::Retry | QMessageBox::Abort);
            if (reply == QMessageBox::Abort){
                siw->close();
            }else{
                goto Retry;
            }
        }
    }else{
        siw->close();
    }
    ui->label_6->setText(participantNum);
    tempFiles += (participantNum.toStdString()+'_'+date.toStdString()+'_'+others.toStdString());
    delete siw;
}

/*创建缓存TXT文件*/
void MainWindow::createTempTXT()
{
    samplesWrite.open(tempFiles + "_samples.txt");
    samplesWrite.close();
    eventsWrite.open(tempFiles + "_events.txt");
    eventsWrite.close();
    samplesWrite.open(tempFiles + "_samples.txt", std::ios::app);
    for(int i = 0; i < CHANNELS; i++)
    {
        if(i < 7)
            samplesWrite << i + 1 << " ";
        else
            samplesWrite << i + 1 << std::endl;
    }
    eventsWrite.open(tempFiles + "_events.txt", std::ios::app);
    eventsWrite << "latency type" << std::endl;
    isRec = true;
}

/*设置文件保存的路径*/
void MainWindow::setFilePath(int s, std::string path)
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
                for(int i = 0; i < CHANNELS; i++)
                    channelNames[i] = scl.names[i].toStdString();
            }
            else
            {
                goto Again;
            }
        }
        else{
            /*各通道默认标签*/
            for(int i = 0; i < CHANNELS; i++)
                channelNames[i] = std::to_string(i + 1);
        }
    }
    else
    {
        /*各通道默认标签*/
        for(int i = 0; i < CHANNELS; i++)
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
    double *buf_persec = new double[CHANNELS * SAMPLE_RATE];
    std::string file_name = participantNum.toStdString()+'_'+date.toStdString()+'_'+others.toStdString();
    setFilePath(0, edf_path);
    //新建文件夹
    QDir dir;
    if (!dir.exists(QString::fromStdString(edf_path)))
    {
        dir.mkpath(QString::fromStdString(edf_path));

    }
    //设置EDF文件参数
    flag = edfopen_file_writeonly((edf_path + "\\" + file_name + ".edf").c_str(), EDFLIB_FILETYPE_EDFPLUS, CHANNELS);
    for(i = 0; i < CHANNELS; i++)
    {
        //设置各通道采样率
        edf_set_samplefrequency(flag, i, SAMPLE_RATE);
        //设置信号最大与最小数字值(EDF为16位文件，一般设置为-32768~32767)
        edf_set_digital_maximum(flag, i, 32767);
        edf_set_digital_minimum(flag, i, -32768);
        //设置信号最大与最小物理值(即信号在物理度量上的最大、最小值)
        edf_set_physical_maximum(flag, i, MAX_VOLTAGE);
        edf_set_physical_minimum(flag, i, MIN_VOLTAGE);
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
              if(SAMPLE_RATE * row + col < CHANNELS * SAMPLE_RATE)
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
    for(i = 0; i < CHANNELS * SAMPLE_RATE; i++)
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
    for(std::size_t i = 0; i < CHANNELS; i++)
    {
        if(i < CHANNELS - 1)
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
    readme << "Number of channels: " << CHANNELS << std::endl;
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
    for(int i = 0; i < 200; i++)
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
    charts[0].addSeries(line);
    charts[0].setAxisX(&axisX[0], line);
    charts[0].setAxisY(&axisY[0], line);
    /*添加marker文字注释*/
    QGraphicsSimpleTextItem *pItem = new QGraphicsSimpleTextItem(&charts[0]);
    pItem->setText(QString::fromStdString(event) + "\n" + QDateTime::currentDateTime().toString("hh:mm:ss"));
    marks[line] = std::make_pair(QDateTime::currentDateTime().toMSecsSinceEpoch(), pItem);
    //marks[line] = std::make_pair(threshold - 1, pItem);
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
    for(int index = 0; index < CHANNELS; index++)
    {
        //设置x轴
        axisX[index].setRange(QDateTime::currentDateTime().addSecs(-TIME_INTERVAL), QDateTime::currentDateTime());
        axisX[index].setTickCount(TIME_INTERVAL);
        axisX[index].setFormat("hh:mm:ss");
        charts[index].addAxis(&axisX[index], Qt::AlignBottom);
        //设置y轴
        axisY[index].setRange(-50, 50);
        axisY[index].setTickCount(3);
        axisY[index].setTitleText("Voltage/uV");
        axisY[index].setLabelFormat("%d");
        charts[index].addAxis(&axisY[index], Qt::AlignLeft);
        //链接数据
        series[index].append(QPointF(0, 0));
        charts[index].addSeries(&series[index]);
        QPen splinePen;
        splinePen.setBrush(Qt::blue);
        splinePen.setColor(Qt::blue);
        series[index].setPen(splinePen);
        //设置界面显示
        charts[index].setAxisX(&axisX[index], &series[index]);
        charts[index].setAxisY(&axisY[index], &series[index]);
        charts[index].legend()->hide();
        charts[index].setTheme(QChart::ChartThemeLight);
        charts[index].setMargins({-20, 0, 0, -10});
        charts[index].axisX()->setGridLineVisible(false);
        charts[index].axisY()->setGridLineVisible(false);
        switch(index)
        {
            case 0:
                ui->wave->setChart(&charts[index]);
                break;
            case 1:
                ui->wave_2->setChart(&charts[index]);
                break;
            case 2:
                ui->wave_3->setChart(&charts[index]);
                break;
            case 3:
                ui->wave_4->setChart(&charts[index]);
                break;
            case 4:
                ui->wave_5->setChart(&charts[index]);
                break;
            case 5:
                ui->wave_6->setChart(&charts[index]);
                break;
            case 6:
                ui->wave_7->setChart(&charts[index]);
                break;
            case 7:
                ui->wave_8->setChart(&charts[index]);
                break;
        }
    }
}

/*设置Y轴范围*/
void MainWindow::setVoltage50()
{
    for(int index = 0; index < CHANNELS; index++)
    {
        charts[index].axisY()->setRange(-50, 50);
    }
    maxVoltage = 50;
}

void MainWindow::setVoltage100()
{
    for(int index = 0; index < CHANNELS; index++)
    {
        charts[index].axisY()->setRange(-100, 100);
    }
    maxVoltage = 100;
}

void MainWindow::setVoltage200()
{
    for(int index = 0; index < CHANNELS; index++)
    {
        charts[index].axisY()->setRange(-200, 200);
    }
    maxVoltage = 200;
}

/*波形更新*/
void MainWindow::updateWave(double *channelData)
{
    /*绘制波形*/
    for(int index = 0; index < CHANNELS; index++)
    {
        /*坐标轴刷新*/
        charts[index].axisX()->setRange(QDateTime::currentDateTime().addSecs(-TIME_INTERVAL), QDateTime::currentDateTime());
        if(pointQueue[index].size() >= threshold)
        {
            pointQueue[index].dequeue();
        }
        pointQueue[index].enqueue(QPointF(QDateTime::currentDateTime().toMSecsSinceEpoch(), channelData[index]));
        series[index].replace(pointQueue[index]);
    }

}

void MainWindow::getImpedanceFromBoard()
{
    for(int i = 0; i < CHANNELS; i++)
    {
    #ifdef NO_BOARD
        impedance[i] = 500.0 * rand() / (RAND_MAX);
    #endif
        if(impedance[i] >= 490)
            impedance[i] = -1;  // 阻抗无穷大
    }
}

/*数据获取，暂用随机数方式代替实际方法*/
void MainWindow::getDataFromBoard()
{
    for(int i = 0; i < CHANNELS; i++)
    {
    #ifdef NO_BOARD
        originalData[i] = 45 * sin(msecCnt * (2 * 3.1415926535 / 10)) + (rand() % 10 - 5);
    #endif
        if(isFilt)
        {
            Filter f;
            /*原始数据进入带通滤波缓冲区*/
            if(bandPassBuffer[i].size() < FILTER_ORDER + 1)
            {
                bandPassBuffer[i].enqueue(originalData[i]);
            }
            /*带通滤波*/
            else
            {
                /*计算带通滤波器冲激响应*/
                double y_n;
                double *cur_bp_h = new double[FILTER_ORDER + 1];
                bandPassCoff[i] = cur_bp_h;
                f.countBandPassCoef(FILTER_ORDER, SAMPLE_RATE, cur_bp_h, lowCut, highCut);
                /*计算滤波后的值*/
                y_n = conv(BandPass, i);
                /*队列左移一位*/
                bandPassBuffer[i].dequeue();
                /*滤波后的值入队*/
                bandPassBuffer[i].enqueue(y_n);
                if(notchBuffer[i].size() < FILTER_ORDER + 1)
                    notchBuffer[i].enqueue(y_n);
                else
                {   /*陷波*/
                    double *cur_n_h = new double[FILTER_ORDER + 1];
                    notchCoff[i] = cur_n_h;
                    f.countNotchCoef(FILTER_ORDER, SAMPLE_RATE, cur_n_h, notchCut);
                    y_n = conv(Notch, i);
                    notchBuffer[i].dequeue();
                    notchBuffer[i].enqueue(y_n);
                }
                filtData[i] = y_n;
            }
        }
        if(isRec)
        {
            /*写入缓存txt文件*/
            if(i < CHANNELS - 1)
                samplesWrite << originalData[i] << " ";
            else
                samplesWrite << originalData[i] << std::endl;
            if(i == 0) curLine++;
        }
    }
    msecCnt++;
}

/*时域序列卷积*/
double MainWindow::conv(filt type, int index)
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

/*图像刷新*/
void MainWindow::graphFresh()
{
    /*波形刷新*/
    isFilt ? updateWave(filtData) : updateWave(originalData);
    /*在图形上绘制红色铅直线标记marker*/
    std::map<QLineSeries *, std::pair<qint64, QGraphicsSimpleTextItem *>>::iterator iter;
    for(iter = marks.begin(); iter != marks.end(); iter++)
    {
        QList<QPointF> marker_point;
        marker_point.append(QPointF((iter->second).first, -maxVoltage));
        marker_point.append(QPointF((iter->second).first, maxVoltage));
        iter->first->replace(marker_point);
        /*文字标记*/
        (iter->second).second->setPos(charts[0].mapToPosition(QPointF((iter->second).first, 50.0), iter->first));
    }
    /*阻抗刷新*/
    for(int i = 0; i < CHANNELS; i++)
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
            text = QString::number(impedance[i]);
            if(impedance[i] <= 20)
                color = Green;
            else if(impedance[i] > 20 && impedance[i] <= 100)
                color = Yellow;
            else
                color = Red;
        }
        switch (i) {
            case 0:
                ui->r_0->setText(text);
                if(color == Green)
                    ui->r_0->setStyleSheet("QLabel{background:#00FF00;}");
                else if(color == Yellow)
                    ui->r_0->setStyleSheet("QLabel{background:#DAA520;}");
                else
                    ui->r_0->setStyleSheet("QLabel{background:#FF0000;}");
                break;
            case 1:
                ui->r_1->setText(text);
                if(color == Green)
                    ui->r_1->setStyleSheet("QLabel{background:#00FF00;}");
                else if(color == Yellow)
                    ui->r_1->setStyleSheet("QLabel{background:#DAA520;}");
                else
                    ui->r_1->setStyleSheet("QLabel{background:#FF0000;}");
                break;
            case 2:
                ui->r_2->setText(text);
                if(color == Green)
                    ui->r_2->setStyleSheet("QLabel{background:#00FF00;}");
                else if(color == Yellow)
                    ui->r_2->setStyleSheet("QLabel{background:#DAA520;}");
                else
                    ui->r_2->setStyleSheet("QLabel{background:#FF0000;}");
                break;
            case 3:
                ui->r_3->setText(text);
                if(color == Green)
                    ui->r_3->setStyleSheet("QLabel{background:#00FF00;}");
                else if(color == Yellow)
                    ui->r_3->setStyleSheet("QLabel{background:#DAA520;}");
                else
                    ui->r_3->setStyleSheet("QLabel{background:#FF0000;}");
                break;
            case 4:
                ui->r_4->setText(text);
                if(color == Green)
                    ui->r_4->setStyleSheet("QLabel{background:#00FF00;}");
                else if(color == Yellow)
                    ui->r_4->setStyleSheet("QLabel{background:#DAA520;}");
                else
                    ui->r_4->setStyleSheet("QLabel{background:#FF0000;}");
                break;
            case 5:
                ui->r_5->setText(text);
                if(color == Green)
                    ui->r_5->setStyleSheet("QLabel{background:#00FF00;}");
                else if(color == Yellow)
                    ui->r_5->setStyleSheet("QLabel{background:#DAA520;}");
                else
                    ui->r_5->setStyleSheet("QLabel{background:#FF0000;}");
                break;
            case 6:
                ui->r_6->setText(text);
                if(color == Green)
                    ui->r_6->setStyleSheet("QLabel{background:#00FF00;}");
                else if(color == Yellow)
                    ui->r_6->setStyleSheet("QLabel{background:#DAA520;}");
                else
                    ui->r_6->setStyleSheet("QLabel{background:#FF0000;}");
                break;
            case 7:
                ui->r_7->setText(text);
                if(color == Green)
                    ui->r_7->setStyleSheet("QLabel{background:#00FF00;}");
                else if(color == Yellow)
                    ui->r_7->setStyleSheet("QLabel{background:#DAA520;}");
                else
                    ui->r_7->setStyleSheet("QLabel{background:#FF0000;}");
                break;
        }
    }
}

/*停止写入数据并保存缓存txt文件*/
void MainWindow::stopRec()
{
  isRec = false;
  //关闭txt文件输入流
  samplesWrite.close();
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
                                    "实验时长：7分钟左右\n"
                                    "实验内容：数字2(20%概率出现)与数字8交替闪烁(80%概率出现)。",
                                    QMessageBox::Ok);
    if(reply == QMessageBox::Ok)
    {
        isSaveP300BH = true;
        //进入实验
        P300Oddball *p = new P300Oddball();
        connect(p, SIGNAL(sendMark(const std::string)), this, SLOT(createMark(const std::string)));
        p->show();
    }
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

void MainWindow::on_lineEdit_5_editingFinished()
{
    QString m = ui->lineEdit_5->text();
    if(!m.isEmpty())
        markerNames[4] = m.toStdString();
}

void MainWindow::on_lineEdit_6_editingFinished()
{
    QString m = ui->lineEdit_6->text();
    if(!m.isEmpty())
        markerNames[5] = m.toStdString();
}

void MainWindow::on_lineEdit_7_editingFinished()
{
    QString m = ui->lineEdit_7->text();
    if(!m.isEmpty())
        markerNames[6] = m.toStdString();
}

void MainWindow::on_lineEdit_8_editingFinished()
{
    QString m = ui->lineEdit_8->text();
    if(!m.isEmpty())
        markerNames[7] = m.toStdString();
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

void MainWindow::on_pushButton_6_clicked()
{
    createMark(markerNames[4]);
}

void MainWindow::on_pushButton_7_clicked()
{
    createMark(markerNames[5]);
}

void MainWindow::on_pushButton_8_clicked()
{
    createMark(markerNames[6]);
}

void MainWindow::on_pushButton_9_clicked()
{
    createMark(markerNames[7]);
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
        isFilt = true;
    }
}
