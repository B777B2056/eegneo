#include "acquisitionwindow.h"
#include "ui_acquisitionwindow.h"
#include "ui_charthelp.h"

AcquisitionWindow::AcquisitionWindow(QWidget *parent)
    : QMainWindow(parent)
    , lowCut(-1.0), highCut(-1.0), notchCut(-1.0)
    , maxVoltage(50), timeInterval(5), threshold(5000)
    , eventCount(0), curLine(0), isRec(false), isSaveP300BH(false), isFinish(-1), tempFiles("")
    , ui(new Ui::AcquisitionWindow)
{
    ui->setupUi(this);
    /*滤波指示信号初始化：未滤波*/
    ui->label_5->setText("Off");
    /*动态分配内存*/
    markerNames = new std::string[MANUAL_MAKER];
    /*定时器初始化*/
    graphTimer = new QTimer(this);
    graphTimer->setInterval(GRAPH_FRESH);//设置定时周期，单位：毫秒
    impTimer = new QTimer(this);
    impTimer->setInterval(IMPEDANCE_FRESH);
    /*绘图板初始化*/
    help = new ChartHelp();
    montages[0] = help->ui->widget;
    montages[1] = help->ui->widget_2;
    montages[2] = help->ui->widget_3;
    montages[3] = help->ui->widget_4;
    montages[4] = help->ui->widget_5;
    montages[5] = help->ui->widget_6;
    montages[6] = help->ui->widget_7;
    montages[7] = help->ui->widget_8;
    montages[8] = help->ui->widget_9;
    montages[9] = help->ui->widget_10;
    montages[10] = help->ui->widget_11;
    montages[11] = help->ui->widget_12;
    montages[12] = help->ui->widget_13;
    montages[13] = help->ui->widget_14;
    montages[14] = help->ui->widget_15;
    montages[15] = help->ui->widget_16;
    /*信号与槽的链接*/
    connect(this, SIGNAL(returnMain()), parent, SLOT(goToMainWindow()));
    connect(this, SIGNAL(sendBasicInfo(QString, QString)), parent, SLOT(getBasicInfo(QString, QString)));
    connect(graphTimer, SIGNAL(timeout()), this, SLOT(graphFresh()));
    connect(impTimer, SIGNAL(timeout()), this, SLOT(getImpedanceFromBoard()));
    connect(ui->actionStart_Recording, SIGNAL(triggered()), this, SLOT(createTempTXT()));
    connect(ui->actionEDF, SIGNAL(triggered()), this, SLOT(saveEDF()));
    connect(ui->actionTXT, SIGNAL(triggered()), this, SLOT(saveTXT()));
    connect(ui->actionStop_Recording, SIGNAL(triggered()), this, SLOT(stopRec()));
    connect(ui->actionp300oddball, SIGNAL(triggered()), this, SLOT(p300Oddball()));
    connect(ui->action_10_10uV, SIGNAL(triggered()), this, SLOT(setVoltage10()));
    connect(ui->action_25_25uV, SIGNAL(triggered()), this, SLOT(setVoltage25()));
    connect(ui->action50uV, SIGNAL(triggered()), this, SLOT(setVoltage50()));
    connect(ui->action100uV, SIGNAL(triggered()), this, SLOT(setVoltage100()));
    connect(ui->action200uV, SIGNAL(triggered()), this, SLOT(setVoltage200()));
    connect(ui->action_500_500uV, SIGNAL(triggered()), this, SLOT(setVoltage500()));
    connect(ui->action_1000_1000uV, SIGNAL(triggered()), this, SLOT(setVoltage1000()));
    connect(ui->action0_1s, SIGNAL(triggered()), this, SLOT(setTime1()));
    connect(ui->action0_5s, SIGNAL(triggered()), this, SLOT(setTime5()));
    connect(ui->action0_10s, SIGNAL(triggered()), this, SLOT(setTime10()));
}

AcquisitionWindow::~AcquisitionWindow()
{
    /*停止数据获取子线程*/
    dpt->quit();
    dpt->wait();
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

/*初始化*/
void AcquisitionWindow::init()
{
    /*待用户输入基本信息*/
    SetInfo *siw = new SetInfo;
Retry1:
    int rec = siw->exec();
    siw->getInfo(participantNum, date, others, expName, channel_num, b);
    if(rec == QDialog::Accepted){
        if(participantNum.isEmpty() || date.isEmpty() || expName.isEmpty() || !channel_num || (b == Null)){
            /*被试信息必须项缺失，弹出错误信息后返回*/
            QMessageBox::StandardButton reply;
            reply = QMessageBox::critical(siw, siw->tr("错误"),
                                            "被试信息缺失！\n请检查被试编号、日期与实验名称。",
                                            QMessageBox::Retry | QMessageBox::Abort);
            if (reply == QMessageBox::Abort)
            {
                siw->close();
            }
            else
            {
                goto Retry1;
            }
        }
    }else{
        siw->close();
    }
    delete siw;
    /*被试信息初始化*/
    tempFiles += (participantNum.toStdString()+'_'+date.toStdString()+'_'+expName.toStdString()+'_'+others.toStdString());
    emit sendBasicInfo(participantNum, "temp files//" +participantNum+'_'+date+'_'+expName+'_'+others);
    if(b == Shanxi)
    {
        sp = 1000;
        dpt = new DataProcessThread(channel_num, sp, b);
    }
    else
    {
        sp = 250;
        ChooseCom c;
        QString com;
    Retry2:
        int rec = c.exec();
        c.getCom(com);
        if(rec == QDialog::Accepted){
            if(com.isEmpty()){
                /*被试信息必须项缺失，弹出错误信息后返回*/
                QMessageBox::critical(siw, siw->tr("错误"),
                                                "请填写com口！",
                                                QMessageBox::Retry);
                goto Retry2;
            }
        }
        dpt = new DataProcessThread(channel_num, sp, b, com);
    }
    /*子线程初始化并开始*/
    connect(dpt, SIGNAL(sendData(std::vector<double>)), this, SLOT(receiveData(std::vector<double>)));
    connect(dpt, SIGNAL(inFilt()), this, SLOT(isInFilt()));
    connect(this, SIGNAL(doFilt(int, int, int)), dpt, SLOT(startFilt(int, int, int)));
    connect(this, SIGNAL(doRec(std::string)), dpt, SLOT(startRec(std::string)));
    connect(this, SIGNAL(writeEvent(std::string)), dpt, SLOT(doEvent(std::string)));
    connect(this, SIGNAL(doneRec()), dpt, SLOT(stopRec()));
    dpt->start();
    /*初始化vector*/
    impedance.assign(channel_num, 0);
    graphData.assign(channel_num, 0.0);
    channelNames.assign(channel_num, "");
    pointQueue.assign(channel_num, QQueue<QPointF>());
    for(int i = 0; i < channel_num; i++)
    {
        impDisplay.push_back(new QLabel(this));
        series.push_back(new QSplineSeries);
        axisX.push_back(new QDateTimeAxis);
        axisY.push_back(new QValueAxis);
        charts.push_back(new QChart);
    }
    if(channel_num == 8)
    {
        for(int i = 8; i < 16; i++)
            montages[i]->hide();
    }
    /*绘图初始化*/
    initChart();
    /*启动定时器*/
    graphTimer->start();
    impTimer->start();
}

/*创建缓存TXT文件*/
void AcquisitionWindow::createTempTXT()
{
    /*缓存文件放入一个文件夹*/
    QDir dir;
    if (!dir.exists(QString::fromStdString("temp files")))
        dir.mkpath("temp files");
    tempFiles = "temp files//" + tempFiles;
    isRec = true;
    emit doRec(tempFiles);
}

/*设置文件保存的路径*/
void AcquisitionWindow::setFilePath(int s, std::string& path)
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
            SetChannelName scl(channel_num);
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
    emit sendBasicInfo(participantNum, participantNum+'_'+date+'_'+expName+'_'+others);
}

/*创建EDF文件*/
void AcquisitionWindow::saveEDF()
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
    double *buf_persec = new double[channel_num * sp];
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
        edf_set_samplefrequency(flag, i, sp);
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
            for(int row = 0; row < sp; row++)
            {
              if(sp* row + col < channel_num * sp)
                  ss >> buf_persec[sp * row + col];
            }
            /*1s结束*/
            if(!((col) % sp))
            {
                edf_blockwrite_physical_samples(flag, buf_persec);
                col = 0;
            }
        }
        ++col;
    }
    /*写入多余的空数据以保证标记存在*/
    for(i = 0; i < channel_num * sp; i++)
       buf_persec[i] = 0.0;
    for(i = 0; i < eventCount - curLine / sp + 1; i++)
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
void AcquisitionWindow::saveTXT()
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
    readme << "Data sampling rate: " << sp << std::endl;
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
void AcquisitionWindow::saveBehavioralP300(std::string path)
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
void AcquisitionWindow::createMark(const std::string event)
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
        emit writeEvent(event);
        eventCount++;
    }
}

void AcquisitionWindow::initChart()
{
    for(int index = 0; index < channel_num; index++)
    {
        //设置x轴
        axisX[index]->setRange(QDateTime::currentDateTime().addSecs(-timeInterval), QDateTime::currentDateTime());
        axisX[index]->setTickCount(5);
        axisX[index]->setFormat("hh:mm:ss");
        charts[index]->addAxis(axisX[index], Qt::AlignBottom);
        //设置y轴
        axisY[index]->setRange(-50, 50);
        axisY[index]->setTickCount(3);
        axisY[index]->setTitleText("uV");
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
        impDisplay[index]->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        impDisplay[index]->setMinimumSize(QSize(75, 75));
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
        item->setSizeHint(QSize(1700, 120));
        ui->listWidget->setItemWidget(item, widget);
    }
}

/*设置Y轴范围*/
void AcquisitionWindow::setVoltage10()
{
    maxVoltage = 10;
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-maxVoltage, maxVoltage);
    }
}

void AcquisitionWindow::setVoltage25()
{
    maxVoltage = 25;
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-maxVoltage, maxVoltage);
    }
}


void AcquisitionWindow::setVoltage50()
{
    maxVoltage = 50;
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-maxVoltage, maxVoltage);
    }
}

void AcquisitionWindow::setVoltage100()
{
    maxVoltage = 100;
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-maxVoltage, maxVoltage);
    }
}

void AcquisitionWindow::setVoltage200()
{
    maxVoltage = 200;
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-maxVoltage, maxVoltage);
    }
}

void AcquisitionWindow::setVoltage500()
{
    maxVoltage = 500;
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-maxVoltage, maxVoltage);
    }
}

void AcquisitionWindow::setVoltage1000()
{
    maxVoltage = 1000;
    for(int index = 0; index < channel_num; index++)
    {
        charts[index]->axisY()->setRange(-maxVoltage, maxVoltage);
    }
}

/*设置X轴范围*/
void AcquisitionWindow::setTime1()
{
    timeInterval = 1;
    threshold = 1000 * timeInterval;
}

void AcquisitionWindow::setTime5()
{
    timeInterval = 5;
    threshold = 1000 * timeInterval;
}

void AcquisitionWindow::setTime10()
{
    timeInterval = 10;
    threshold = 1000 * timeInterval;
}

/*波形更新*/
void AcquisitionWindow::updateWave()
{
    /*绘制波形*/
    for(int index = 0; index < channel_num; index++)
    {
        /*坐标轴刷新*/
        charts[index]->axisX()->setRange(QDateTime::currentDateTime().addSecs(-timeInterval), QDateTime::currentDateTime());
        if(pointQueue[index].size() >= threshold)
        {
            pointQueue[index].dequeue();
        }
        pointQueue[index].enqueue(QPointF(QDateTime::currentDateTime().toMSecsSinceEpoch(), graphData[index]));
        series[index]->replace(pointQueue[index]);
    }
}

void AcquisitionWindow::getImpedanceFromBoard()
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

/*数据获取*/
void AcquisitionWindow::receiveData(std::vector<double> vec)
{
    graphData = vec;
    ++rcnt;
    if(isRec) curLine++;
}

/*图像刷新*/
void AcquisitionWindow::graphFresh()
{
    /*波形刷新*/
    updateWave();
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
        BackgroundColor color;  // 背景色（0-20绿色，20-100黄色，100以上红色）
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
void AcquisitionWindow::stopRec()
{
  emit doneRec();
}

/*p300 Oddball范式*/
void AcquisitionWindow::p300Oddball()
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
void AcquisitionWindow::getImgNum(int n)
{
    p300OddballImgNum = n;
}

/*获取已输入的marker*/
void AcquisitionWindow::on_lineEdit_editingFinished()
{
    QString m = ui->lineEdit->text();
    if(!m.isEmpty())
        markerNames[0] = m.toStdString();
}

void AcquisitionWindow::on_lineEdit_2_editingFinished()
{
    QString m = ui->lineEdit_2->text();
    if(!m.isEmpty())
        markerNames[1] = m.toStdString();
}

void AcquisitionWindow::on_lineEdit_3_editingFinished()
{
    QString m = ui->lineEdit_3->text();
    if(!m.isEmpty())
        markerNames[2] = m.toStdString();
}

void AcquisitionWindow::on_lineEdit_4_editingFinished()
{
    QString m = ui->lineEdit_4->text();
    if(!m.isEmpty())
        markerNames[3] = m.toStdString();
}

void AcquisitionWindow::on_pushButton_2_clicked()
{
    createMark(markerNames[0]);
}

void AcquisitionWindow::on_pushButton_3_clicked()
{
    createMark(markerNames[1]);
}

void AcquisitionWindow::on_pushButton_4_clicked()
{
    createMark(markerNames[2]);
}

void AcquisitionWindow::on_pushButton_5_clicked()
{
    createMark(markerNames[3]);
}

/*选择带通滤波、凹陷滤波频率*/
void AcquisitionWindow::on_comboBox_currentIndexChanged(int index)
{
    if(index != 0)
        lowCut = highPassFres[index - 1];
}

void AcquisitionWindow::on_comboBox_2_currentIndexChanged(int index)
{
    if(index != 0)
        highCut = lowPassFres[index - 1];
}

void AcquisitionWindow::on_comboBox_3_currentIndexChanged(int index)
{
    if(index != 0)
        notchCut = ((index == 1) ? 50.0 : 60.0);
}

/*按了"滤波"按钮后开始滤波*/
void AcquisitionWindow::on_filter_clicked()
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

void AcquisitionWindow::isInFilt()
{
    ui->label_5->setText("On");
}

void AcquisitionWindow::on_pushButton_clicked()
{
    if(this->isFinish == 0)
    {
        QMessageBox::critical(this, tr("错误"), "数据正在写入文件！", QMessageBox::Ok);
    }
    else
    {
        /*发送信号*/
        emit returnMain();
    }
}
