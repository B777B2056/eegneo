#include "acquisitionwindow.h"
#include "ui_charthelp.h"
#include <QMessageBox>
#include <QQueue>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMutex>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <memory>
#include "setinfo.h"
#include "setchannelname.h"
#include "p300.h"
#include "workthread.h"
#include "choosecom.h"

extern "C"
{
    #include "edflib.h"
}

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

const static double highPassFres[7] = {0.1, 0.3, 3.5, 8.0, 12.5, 16.5, 20.5};  // 高通滤波频率选择
const static double lowPassFres[7] = {8.0, 12.5, 16.5, 20.5, 28.0, 45.0, 50.0};  // 低通滤波频率选择

ElectrodesInfo::ElectrodesInfo(WaveDrawingInfo& waveInfo)
    : montages{waveInfo.help.ui->widget, waveInfo.help.ui->widget_2, waveInfo.help.ui->widget_3,
               waveInfo.help.ui->widget_4, waveInfo.help.ui->widget_5, waveInfo.help.ui->widget_6,
               waveInfo.help.ui->widget_7, waveInfo.help.ui->widget_8, waveInfo.help.ui->widget_9,
               waveInfo.help.ui->widget_10, waveInfo.help.ui->widget_11, waveInfo.help.ui->widget_12,
               waveInfo.help.ui->widget_13, waveInfo.help.ui->widget_14, waveInfo.help.ui->widget_15,
               waveInfo.help.ui->widget_16}
{
    impTimer.setInterval(IMPEDANCE_FRESH);
}

AcquisitionWindow::AcquisitionWindow(QWidget *parent)
    : QMainWindow(parent)
    , _waveDrawingInfo()
    , _electrodes(_waveDrawingInfo)
    , _parent(parent)
    , ui(new Ui::AcquisitionWindow)
{
    ui->setupUi(this);
    // 滤波指示信号初始化：未滤波
    ui->label_5->setText("Off");
}

AcquisitionWindow::~AcquisitionWindow()
{
    // 停止数据获取子线程
    _dpt->quit();
    _dpt->wait();
    // 释放内存
    std::vector<QLineSeries *> qls;
    std::map<QLineSeries *, std::pair<qint64, QGraphicsSimpleTextItem *>>::iterator iter;
    for(iter = _markerInfo.marks.begin(); iter != _markerInfo.marks.end(); iter++)
    {
        qls.push_back(iter->first);
        QGraphicsSimpleTextItem *t = (iter->second).second;
        (iter->second).second = nullptr;
        delete t;
    }
    _markerInfo.marks.clear();
    for(std::size_t i = 0; i < qls.size(); i++)
    {
        delete qls[i];
    }
    for(int i = 0; i < static_cast<int>(_electrodes.channelNum); ++i)
    {
        delete _electrodes.impDisplay[i];
        delete _waveDrawingInfo.series[i];
        delete _waveDrawingInfo.axisX[i];
        delete _waveDrawingInfo.axisY[i];
        delete _waveDrawingInfo.charts[i];
    }
    delete ui;
}

void AcquisitionWindow::start()
{
    showParticipantInfoWindow();
    showComChooseWindow();
    setSignalSlotConnect();
    // 绘图初始化
    initChart();
    // 启动定时器
    _waveDrawingInfo.graphTimer.start();
    _electrodes.impTimer.start();
}

void AcquisitionWindow::setSignalSlotConnect()
{
    // 信号与槽的链接
    QObject::connect(this, SIGNAL(returnMain()), _parent, SLOT(goToMainWindow()));
    QObject::connect(this, SIGNAL(sendBasicInfo(QString, QString)), _parent, SLOT(getBasicInfo(QString, QString)));
    QObject::connect(&_waveDrawingInfo.graphTimer, SIGNAL(timeout()), this, SLOT(graphFresh()));
    QObject::connect(&_electrodes.impTimer, SIGNAL(timeout()), this, SLOT(getImpedanceFromBoard()));

    QObject::connect(ui->lineEdit, SIGNAL(editingFinished()), this, SLOT(onLineEditEditingFinished()));
    QObject::connect(ui->lineEdit_2, SIGNAL(editingFinished()), this, SLOT(onLineEdit2EditingFinished()));
    QObject::connect(ui->lineEdit_3, SIGNAL(editingFinished()), this, SLOT(onLineEdit3EditingFinished()));
    QObject::connect(ui->lineEdit_4, SIGNAL(editingFinished()), this, SLOT(onLineEdit4EditingFinished()));

    QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(onPushButtonClicked()));
    QObject::connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(onPushButton2Clicked()));
    QObject::connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(onPushButton3Clicked()));
    QObject::connect(ui->pushButton_4, SIGNAL(clicked()), this, SLOT(onPushButton4Clicked()));
    QObject::connect(ui->pushButton_5, SIGNAL(clicked()), this, SLOT(onPushButton5Clicked()));

    QObject::connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)));
    QObject::connect(ui->comboBox_2, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBox2CurrentIndexChanged(int)));
    QObject::connect(ui->comboBox_3, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBox3CurrentIndexChanged(int)));

    QObject::connect(ui->filter, SIGNAL(clicked()), this, SLOT(onFilterClicked()));

    QObject::connect(ui->actionStart_Recording, SIGNAL(triggered()), this, SLOT(createTempTXT()));
    QObject::connect(ui->actionEDF, SIGNAL(triggered()), this, SLOT(saveEdfPlus()));
    QObject::connect(ui->actionTXT, SIGNAL(triggered()), this, SLOT(saveTxt()));
    QObject::connect(ui->actionStop_Recording, SIGNAL(triggered()), this, SLOT(stopRec()));
    QObject::connect(ui->actionp300oddball, SIGNAL(triggered()), this, SLOT(p300Oddball()));
    QObject::connect(ui->action_10_10uV, SIGNAL(triggered()), this, SLOT(setVoltage10()));
    QObject::connect(ui->action_25_25uV, SIGNAL(triggered()), this, SLOT(setVoltage25()));
    QObject::connect(ui->action50uV, SIGNAL(triggered()), this, SLOT(setVoltage50()));
    QObject::connect(ui->action100uV, SIGNAL(triggered()), this, SLOT(setVoltage100()));
    QObject::connect(ui->action200uV, SIGNAL(triggered()), this, SLOT(setVoltage200()));
    QObject::connect(ui->action_500_500uV, SIGNAL(triggered()), this, SLOT(setVoltage500()));
    QObject::connect(ui->action_1000_1000uV, SIGNAL(triggered()), this, SLOT(setVoltage1000()));
    QObject::connect(ui->action0_1s, SIGNAL(triggered()), this, SLOT(setTime1()));
    QObject::connect(ui->action0_5s, SIGNAL(triggered()), this, SLOT(setTime5()));
    QObject::connect(ui->action0_10s, SIGNAL(triggered()), this, SLOT(setTime10()));
    // 数据获取线程
    QObject::connect(_dpt, SIGNAL(sendData(std::vector<double>)), this, SLOT(receiveData(std::vector<double>)));
    QObject::connect(_dpt, SIGNAL(inFilt()), this, SLOT(setInFilt()));
    QObject::connect(this, SIGNAL(doFilt(int, int, int)), _dpt, SLOT(startFilt(int, int, int)));
    QObject::connect(this, SIGNAL(doRec(std::string)), _dpt, SLOT(startRec(std::string)));
    QObject::connect(this, SIGNAL(writeEvent(std::string)), _dpt, SLOT(doEvent(std::string)));
    QObject::connect(this, SIGNAL(doneRec()), _dpt, SLOT(stopRec()));
}

void AcquisitionWindow::showParticipantInfoWindow()
{
    // 待用户输入基本信息
    SetInfo siw;
ParticipantInfoRetry:
    int rec = siw.exec();
    int channelNum = 0;
    siw.getInfo(_participant.participantNum, _participant.date, _participant.others, _participant.expName, channelNum, _board.type);
    _electrodes.channelNum = static_cast<ChannelNum>(channelNum);
    if(rec == QDialog::Accepted)
    {
        if(_participant.participantNum.isEmpty() || _participant.date.isEmpty() || _participant.expName.isEmpty() || (_board.type == Null))
        {
            // 被试信息必须项缺失，弹出错误信息后返回
            QMessageBox::StandardButton reply;
            reply = QMessageBox::critical(&siw, siw.tr("错误"),
                                            "被试信息缺失\n请检查被试编号、日期与实验名称。",
                                            QMessageBox::Retry | QMessageBox::Abort);
            if (reply == QMessageBox::Abort)
            {
                siw.close();
            }
            else
            {
                goto ParticipantInfoRetry;
            }
        }
    }
    else
    {
        siw.close();
    }
    // 被试信息初始化
    QString particInfoStr = _participant.participantNum+'_'+_participant.date+'_'+_participant.expName+'_'+_participant.others;
    _fileInfo.tempFiles += particInfoStr.toStdString();
    emit sendBasicInfo(_participant.participantNum, "temp files//" + particInfoStr);
    _electrodes.impedance.assign(static_cast<int>(_electrodes.channelNum), 0);
    _waveDrawingInfo.graphData.assign(static_cast<int>(_electrodes.channelNum), 0.0);
    _fileInfo.channelNames.assign(static_cast<int>(_electrodes.channelNum), "");
    _waveDrawingInfo.pointQueue.assign(static_cast<int>(_electrodes.channelNum), QQueue<QPointF>());
    for(int i = 0; i < static_cast<int>(_electrodes.channelNum); ++i)
    {
        _electrodes.impDisplay.push_back(new QLabel(this));
        _waveDrawingInfo.series.push_back(new QSplineSeries);
        _waveDrawingInfo.axisX.push_back(new QDateTimeAxis);
        _waveDrawingInfo.axisY.push_back(new QValueAxis);
        _waveDrawingInfo.charts.push_back(new QChart);
    }
    if(_electrodes.channelNum == ChannelNum::EIGHT)
    {
        for(int i = 8; i < 16; i++)
            _electrodes.montages[i]->hide();
    }
}

void AcquisitionWindow::showComChooseWindow()
{
    if(_board.type == Shanxi)
    {
        _board.sampleRate = 1000;
        _dpt = new DataProcessThread(static_cast<int>(_electrodes.channelNum), _board.sampleRate, _board.type);
    }
    else
    {
        _board.sampleRate = 250;
        ChooseCom c;
        QString com;
ComRetry:
        int rec = c.exec();
        c.getCom(com);
        if(rec == QDialog::Accepted){
            if(com.isEmpty()){
                // 被试信息必须项缺失，弹出错误信息后返回
                QMessageBox::critical(&c, c.tr("错误"),
                                                "请填写com口！",
                                                QMessageBox::Retry);
                goto ComRetry;
            }
        }
        _dpt = new DataProcessThread(static_cast<int>(_electrodes.channelNum), _board.sampleRate, _board.type, com);
    }
    _dpt->start();
}

// 创建缓存TXT文件
void AcquisitionWindow::createTempTXT()
{
    // 缓存文件放入一个文件夹
    QDir dir;
    if (!dir.exists(QString::fromStdString("temp files")))
    {
        dir.mkpath("temp files");
    }
    _fileInfo.tempFiles = "temp files//" + _fileInfo.tempFiles;
    _fileInfo.isRec = true;
    emit doRec(_fileInfo.tempFiles);
}

void AcquisitionWindow::setFilePath(int s, std::string& path)
{
    path = QFileDialog::getExistingDirectory(this, tr("文件保存路径选择"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                               | QFileDialog::DontResolveSymlinks).toStdString();
    path += ("/"+_participant.participantNum.toStdString()+'_'+_participant.date.toStdString()+'_'+_participant.others.toStdString());
    QString q = QString::fromStdString(path);
    q.replace("/", "\\");
    path = q.toStdString();
    // 询问用户是否采用默认的通道名称(仅EDF可用)
    if(!s)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("通道名称"),
                                        "是否自定义通道名称?\n默认名称为数字编号，例如1~8。",
                                        QMessageBox::Ok | QMessageBox::No);
        if (reply == QMessageBox::Ok){
            // 弹窗口让用户输入各通道名称
            SetChannelName scl(static_cast<int>(_electrodes.channelNum));
      Again:
            int rec = scl.exec();
            if(rec == QDialog::Accepted)
            {
                for(int i = 0; i < static_cast<int>(_electrodes.channelNum); i++)
                {
                    _fileInfo.channelNames[i] = scl.names[i].toStdString();
                }
            }
            else
            {
                goto Again;
            }
        }
        else{
            // 各通道默认标签
            for(int i = 0; i < static_cast<int>(_electrodes.channelNum); i++)
            {
                _fileInfo.channelNames[i] = std::to_string(i + 1);
            }
        }
    }
    else
    {
        // 各通道默认标签
        for(int i = 0; i < static_cast<int>(_electrodes.channelNum); i++)
        {
            _fileInfo.channelNames[i] = std::to_string(i + 1);
        }
    }
    emit sendBasicInfo(_participant.participantNum, _participant.participantNum+'_'+_participant.date+'_'+_participant.expName+'_'+_participant.others);
}

void AcquisitionWindow::saveEdfPlus()
{
    QFileInfo samples_file(QString::fromStdString(_fileInfo.tempFiles) + "_samples.txt");
    QFileInfo events_file(QString::fromStdString(_fileInfo.tempFiles) + "_events.txt");
    if(!samples_file.isFile() || !events_file.isFile())
    {
        QMessageBox::critical(this, tr("错误"), "数据未记录，无法导出！", QMessageBox::Ok);
        return;
    }
    _fileInfo.isFinish = 0;
    int i, col = 0;
    std::string edf_path;
    std::ifstream samples_read;  // 8通道缓存txt文件输入流
    std::ifstream events_read;  // 标记缓存txt文件输入流
    double *buf_persec = new double[static_cast<int>(_electrodes.channelNum) * _board.sampleRate];
    std::string file_name = _participant.participantNum.toStdString()+'_'+_participant.date.toStdString()+'_'+_participant.others.toStdString();
    setFilePath(0, edf_path);
    // 新建文件夹
    QDir dir;
    if (!dir.exists(QString::fromStdString(edf_path)))
    {
        dir.mkpath(QString::fromStdString(edf_path));

    }
    // 设置EDF文件参数
    _fileInfo.flag = ::edfopen_file_writeonly((edf_path + "\\" + file_name + ".edf").c_str(), EDFLIB_FILETYPE_EDFPLUS,
                                              static_cast<int>(_electrodes.channelNum));
    for(i = 0; i < static_cast<int>(_electrodes.channelNum); i++)
    {
        // 设置各通道采样率
        ::edf_set_samplefrequency(_fileInfo.flag, i, _board.sampleRate);
        // 设置信号最大与最小数字值(EDF为16位文件，一般设置为-32768~32767)
        ::edf_set_digital_maximum(_fileInfo.flag, i, 32767);
        ::edf_set_digital_minimum(_fileInfo.flag, i, -32768);
        // 设置信号最大与最小物理值(即信号在物理度量上的最大、最小值)
        ::edf_set_physical_maximum(_fileInfo.flag, i, _waveDrawingInfo.maxVoltage);
        ::edf_set_physical_minimum(_fileInfo.flag, i, -_waveDrawingInfo.maxVoltage);
        // 设置各通道数据的单位
        ::edf_set_physical_dimension(_fileInfo.flag, i, "uV");
        // 设置各通道名称
        ::edf_set_label(_fileInfo.flag, i, _fileInfo.channelNames[i].data());
    }
    // 从缓存txt文件中读取数据，并写入edf文件
    samples_read.open(_fileInfo.tempFiles + "_samples.txt");
    events_read.open(_fileInfo.tempFiles + "_events.txt");
    // 写入数据
    while(samples_read.peek() != EOF)
    {
        if(col)
        {
            std::string str;
            std::getline(samples_read, str);
            std::stringstream ss(str);
            for(int row = 0; row < _board.sampleRate; row++)
            {
              if(_board.sampleRate * row + col < static_cast<int>(_electrodes.channelNum) * _board.sampleRate)
                  ss >> buf_persec[_board.sampleRate * row + col];
            }
            /*1s结束*/
            if(!((col) % _board.sampleRate))
            {
                edf_blockwrite_physical_samples(_fileInfo.flag, buf_persec);
                col = 0;
            }
        }
        ++col;
    }
    // 写入多余的空数据以保证标记存在
    for(i = 0; i < static_cast<int>(_electrodes.channelNum) * _board.sampleRate; i++)
    {
       buf_persec[i] = 0.0;
    }
    for(i = 0; i < _fileInfo.eventCount - _fileInfo.curLine / _board.sampleRate + 1; i++)
    {
        ::edf_blockwrite_physical_samples(_fileInfo.flag, buf_persec);
    }
    delete []buf_persec;
    // 写入事件
    while(events_read.peek() != EOF)
    {
        long long run_time;
        std::string str, event;
        std::getline(events_read, str);
        std::stringstream ss(str);
        ss >> run_time >> event;
        ::edfwrite_annotation_utf8(_fileInfo.flag, run_time, 10, event.c_str());
    }
    // 关闭文件流
    samples_read.close();
    events_read.close();
    // 关闭edf文件
    ::edfclose_file(_fileInfo.flag);
    // 保存行为学数据
    if(_fileInfo.isSaveP300BH)
    {
        saveBehavioralP300(edf_path);
    }
    // EDF文件写入完成
    _fileInfo.isFinish = 1;
}

// 保存为3个txt文档（样本数据点，事件信息，描述文档）
void AcquisitionWindow::saveTxt()
{
    QFileInfo samples_file(QString::fromStdString(_fileInfo.tempFiles) + "_samples.txt");
    QFileInfo events_file(QString::fromStdString(_fileInfo.tempFiles) + "_events.txt");
    if(!samples_file.isFile() || !events_file.isFile())
    {
        QMessageBox::critical(this, tr("错误"), "数据未记录，无法导出！", QMessageBox::Ok);
        return;
    }
    _fileInfo.isFinish = 0;
    std::string txt_path;
    std::string file_name = _participant.participantNum.toStdString()+'_'+_participant.date.toStdString()+'_'+_participant.others.toStdString();
    std::ifstream samples_read;  // 8通道缓存txt文件输入流
    std::ofstream samples_txt;  // 数据点txt文件输出流
    std::ifstream events_read;
    std::ofstream events_txt_eeglab;  // eeglab风格
    std::ofstream events_txt_brainstorm;  // brainstorm风格
    setFilePath(1, txt_path);
    // 新建文件夹，把三个txt放入一个文件夹
    QDir dir;
    if (!dir.exists(QString::fromStdString(txt_path)))
    {
        dir.mkpath(QString::fromStdString(txt_path));
    }
    // 写入数据点
    int col = 0;
    samples_txt.open(txt_path + "\\" + file_name + "_samples.txt");
    samples_txt.close();
    samples_read.open(_fileInfo.tempFiles + "_samples.txt");
    samples_txt.open(txt_path + "\\" + file_name + "_samples.txt", std::ios::app);
    for(int i = 0; i < static_cast<int>(_electrodes.channelNum); i++)
    {
        if(i < static_cast<int>(_electrodes.channelNum) - 1)
        {
            samples_txt << _fileInfo.channelNames[i] << " ";
        }
        else
        {
            samples_txt << _fileInfo.channelNames[i] << std::endl;
        }
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
    // 写入事件(EEGLAB风格)
    col = 0;
    events_txt_eeglab.open(txt_path + "\\" + file_name + "_events(eeglab).txt");
    events_txt_eeglab.close();
    events_read.open(_fileInfo.tempFiles + "_events.txt");
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
    // 写入事件(Brainstorm风格)
    col = 0;
    events_txt_brainstorm.open(txt_path + "\\" + file_name + "_events(brainstorm).txt");
    events_txt_brainstorm.close();
    events_read.open(_fileInfo.tempFiles + "_events.txt");
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
    // 写描述性文件文件
    std::ofstream readme;
    readme.open(txt_path + "\\" + file_name + "_readme.txt");
    readme.close();
    readme.open(txt_path + "\\" + file_name + "_readme.txt", std::ios::app);
    readme << "Data sampling rate: " << _board.sampleRate << std::endl;
    readme << "Number of channels: " << static_cast<int>(_electrodes.channelNum) << std::endl;
    readme << "Input field(column) names: latency type" << std::endl;
    readme << "Number of file header lines: 1" << std::endl;
    readme << "Time unit(sec): 1" << std::endl;
    readme.close();
    // 保存行为学数据
    if(_fileInfo.isSaveP300BH)
    {
        saveBehavioralP300(txt_path);
    }
    _fileInfo.isFinish = 1;
}

// 保存行为学数据
void AcquisitionWindow::saveBehavioralP300(std::string path)
{
    int col = 0;
    std::ifstream events_read;
    std::vector<int> blank_acc, blank_rt, blank_resp, on_set_time, blank_rttime;
    std::vector<std::string> code;
    std::vector<std::pair<double, std::string>> vec;
    events_read.open(_fileInfo.tempFiles + "_events.txt");
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
                // 对2有反应，说明反应正确
                blank_acc.push_back(1);
                // 计算反应时间，单位为毫秒，在表格中对应2的位置
                blank_rt.push_back((int)((vec[i + 1].first - vec[i].first) / 10));
                // 对2有反应，按键正确
                blank_resp.push_back(1);
                // 反应动作的时间，毫秒
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
            // 对8有反应，说明反应错误
            if(vec[i + 1].second == "Response")
            {
                blank_acc.push_back(0);
                blank_resp.push_back(1);
                blank_rt.push_back((int)((vec[i + 1].first - vec[i].first) / 10));
                blank_rttime.push_back((int)(vec[i + 1].first / 10));
            }
            // 对8无反应，说明反应正确
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
    std::string file_name = _participant.participantNum.toStdString()+'_'+_participant.date.toStdString()+'_'+_participant.others.toStdString();
    std::ofstream beh_file;
    beh_file.open(path + "\\" + file_name + "_behavioral.csv");
    beh_file.close();
    beh_file.open(path + "\\" + file_name + "_behavioral.csv", std::ios::app);
    beh_file << "Experiment Name: " << _participant.expName.toStdString()
             << ", Date: " << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss").toStdString()
             << std::endl;
    beh_file << "Subject,Trial,blank.ACC,blank.CRESP,blank.OnsetTime,blank.RESP,blank.RT,blank.RTTime,code,correct,pic" << std::endl;
    // 预实验
    for(int i = 0; i < 40; i++)
    {
        std::string pre_exp = _participant.participantNum.toStdString() + "," + std::to_string(i + 1);
        for(int j = 0; j < 9; j++)
        {
            pre_exp += ",";
        }
        beh_file << pre_exp << std::endl;
    }
    // 正式实验
    for(int i = 0; i < _p300OddballImgNum; i++)
    {
        //Subject
        beh_file << _participant.participantNum.toStdString();
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

// 发送marker //
void AcquisitionWindow::createMark(std::string event)
{
    if(event.empty()) return;
    // 添加新的marker直线
    QLineSeries *line = new QLineSeries;
    QPen splinePen; // 设置直线的颜色
    splinePen.setBrush(Qt::red);
    splinePen.setColor(Qt::red);
    line->setPen(splinePen);
    _waveDrawingInfo.charts[0]->addSeries(line);
    _waveDrawingInfo.charts[0]->setAxisX(_waveDrawingInfo.axisX[0], line);
    _waveDrawingInfo.charts[0]->setAxisY(_waveDrawingInfo.axisY[0], line);
    // 添加marker文字注释
    QGraphicsSimpleTextItem *pItem = new QGraphicsSimpleTextItem(_waveDrawingInfo.charts[0]);
    pItem->setText(QString::fromStdString(event) + "\n" + QDateTime::currentDateTime().toString("hh:mm:ss"));
    _markerInfo.marks[line] = std::make_pair(QDateTime::currentDateTime().toMSecsSinceEpoch(), pItem);
    // 将marker写入文件
    if(_fileInfo.isRec)
    {
        emit writeEvent(event);
        _fileInfo.eventCount++;
    }
}

void AcquisitionWindow::initChart()
{
    for(int index = 0; index < static_cast<int>(_electrodes.channelNum); index++)
    {
        // 设置x轴
        _waveDrawingInfo.axisX[index]->setRange(QDateTime::currentDateTime().addSecs(-_waveDrawingInfo.timeInterval), QDateTime::currentDateTime());
        _waveDrawingInfo.axisX[index]->setTickCount(5);
        _waveDrawingInfo.axisX[index]->setFormat("hh:mm:ss");
        _waveDrawingInfo.charts[index]->addAxis(_waveDrawingInfo.axisX[index], Qt::AlignBottom);
        // 设置y轴
        _waveDrawingInfo.axisY[index]->setRange(-50, 50);
        _waveDrawingInfo.axisY[index]->setTickCount(3);
        _waveDrawingInfo.axisY[index]->setTitleText("uV");
        _waveDrawingInfo.axisY[index]->setLabelFormat("%d");
        _waveDrawingInfo.charts[index]->addAxis(_waveDrawingInfo.axisY[index], Qt::AlignLeft);
        // 链接数据
        _waveDrawingInfo.series[index]->setUseOpenGL(true);
        _waveDrawingInfo.series[index]->append(QPointF(0, 0));
        _waveDrawingInfo.charts[index]->addSeries(_waveDrawingInfo.series[index]);
        QPen splinePen;
        splinePen.setBrush(Qt::blue);
        splinePen.setColor(Qt::blue);
        _waveDrawingInfo.series[index]->setPen(splinePen);
        // 设置界面显示
        _waveDrawingInfo.charts[index]->setAxisX(_waveDrawingInfo.axisX[index], _waveDrawingInfo.series[index]);
        _waveDrawingInfo.charts[index]->setAxisY(_waveDrawingInfo.axisY[index], _waveDrawingInfo.series[index]);
        _waveDrawingInfo.charts[index]->legend()->hide();
        _waveDrawingInfo.charts[index]->setTheme(QChart::ChartThemeLight);
        _waveDrawingInfo.charts[index]->setMargins({-10, 0, 0, -10});
        _waveDrawingInfo.charts[index]->axisX()->setGridLineVisible(false);
        _waveDrawingInfo.charts[index]->axisY()->setGridLineVisible(false);
        _electrodes.montages[index]->setChart(_waveDrawingInfo.charts[index]);
        _electrodes.impDisplay[index]->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _electrodes.impDisplay[index]->setMinimumSize(QSize(75, 75));
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
        horLayout->addWidget(_electrodes.montages[index]);
        horLayout->addWidget(_electrodes.impDisplay[index]);
        widget->setLayout(horLayout);
        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        item->setSizeHint(QSize(1700, 120));
        ui->listWidget->setItemWidget(item, widget);
    }
}

// 波形更新
void AcquisitionWindow::updateWave()
{
    for(int index = 0; index < static_cast<int>(_electrodes.channelNum); index++)
    {
        // 坐标轴刷新
        _waveDrawingInfo.charts[index]->axisX()->setRange(QDateTime::currentDateTime().addSecs(-_waveDrawingInfo.timeInterval),
                                                          QDateTime::currentDateTime());
        // 数据点刷新
        if(_waveDrawingInfo.pointQueue[index].size() >= _waveDrawingInfo.threshold)
        {
            _waveDrawingInfo.pointQueue[index].dequeue();
        }
        _waveDrawingInfo.pointQueue[index].enqueue(QPointF(QDateTime::currentDateTime().toMSecsSinceEpoch(),
                                                           _waveDrawingInfo.graphData[index]));
        _waveDrawingInfo.series[index]->replace(_waveDrawingInfo.pointQueue[index]);
    }
}

void AcquisitionWindow::getImpedanceFromBoard()
{
    for(int i = 0; i < static_cast<int>(_electrodes.channelNum); i++)
    {
    #ifdef NO_BOARD
        _electrodes.impedance[i] = 500.0 * rand() / (RAND_MAX);
    #endif
        if(_electrodes.impedance[i] >= 490)
            _electrodes.impedance[i] = -1;  // 阻抗无穷大
    }
    // 阻抗刷新
    for(int i = 0; i < static_cast<int>(_electrodes.channelNum); i++)
    {
        BackgroundColor color;  // 背景色（0-20绿色，20-100黄色，100以上红色）
        QString text;
        if(_electrodes.impedance[i] < 0)
        {
            text = "Inf";
            color = Red;
        }
        else
        {
            text = QString::number(_electrodes.impedance[i]) + "KOhm";
            if(_electrodes.impedance[i] <= 20)
                color = Green;
            else if(_electrodes.impedance[i] > 20 && _electrodes.impedance[i] <= 100)
                color = Yellow;
            else
                color = Red;
        }
        _electrodes.impDisplay[i]->setText(text);
        if(color == Green)
            _electrodes.impDisplay[i]->setStyleSheet("QLabel{background:#00FF00;}");
        else if(color == Yellow)
            _electrodes.impDisplay[i]->setStyleSheet("QLabel{background:#DAA520;}");
        else
            _electrodes.impDisplay[i]->setStyleSheet("QLabel{background:#FF0000;}");
    }
}

// 数据获取
void AcquisitionWindow::receiveData(std::vector<double> vec)
{
    _waveDrawingInfo.graphData = vec;
    if(_fileInfo.isRec)
    {
        _fileInfo.curLine++;
    }
}

// 图像刷新
void AcquisitionWindow::graphFresh()
{
    // 波形刷新
    updateWave();
    /*绘制marker*/
    std::map<QLineSeries *, std::pair<qint64, QGraphicsSimpleTextItem *>>::iterator iter;
    for(iter = _markerInfo.marks.begin(); iter != _markerInfo.marks.end(); iter++)
    {
        QList<QPointF> marker_point;
        marker_point.append(QPointF((iter->second).first, -_waveDrawingInfo.maxVoltage));
        marker_point.append(QPointF((iter->second).first, _waveDrawingInfo.maxVoltage));
        iter->first->replace(marker_point);
        // 文字标记
        (iter->second).second->setPos(_waveDrawingInfo.charts[0]->mapToPosition(QPointF((iter->second).first, 50.0), iter->first));
    }
}

// 停止写入数据并保存缓存txt文件
void AcquisitionWindow::stopRec()
{
    emit doneRec();
}

// p300 Oddball范式
void AcquisitionWindow::p300Oddball()
{
    // 弹窗提示用户本实验相关信息
    QMessageBox::StandardButton reply;
    reply = QMessageBox::information(this, tr("p300-oddball"),
                                    "实验名称：P300诱发电位刺激\n"
                                    "实验范式：Oddball\n"
                                    "实验内容：数字2与数字8交替闪烁。",
                                    QMessageBox::Ok);
    if(reply == QMessageBox::Ok)
    {
        _fileInfo.isSaveP300BH = true;
        // 进入实验
        P300Oddball *p = new P300Oddball();
        QObject::connect(p, SIGNAL(sendImgNum(int)), this, SLOT(getImgNum(int)));
        QObject::connect(p, SIGNAL(sendMark(const std::string)), this, SLOT(createMark(const std::string)));
        p->show();
    }
}

void AcquisitionWindow::onPushButtonClicked()
{
    if(this->_fileInfo.isFinish == 0)
    {
        QMessageBox::critical(this, tr("错误"), "数据正在写入文件", QMessageBox::Ok);
    }
    else
    {
        /*返回开始界面*/
        emit returnMain();
    }
}

// 选择带通滤波、凹陷滤波频率
void AcquisitionWindow::onComboBoxCurrentIndexChanged(int index)
{
    if(index != 0)
    {
        _filtParam.lowCut = highPassFres[index - 1];
    }
}

void AcquisitionWindow::onComboBox2CurrentIndexChanged(int index)
{
    if(index != 0)
    {
        _filtParam.highCut = lowPassFres[index - 1];
    }
}

void AcquisitionWindow::onComboBox3CurrentIndexChanged(int index)
{
    if(index != 0)
    {
        _filtParam.notchCut = ((index == 1) ? 50.0 : 60.0);
    }
}

// 按了"滤波"按钮后开始滤波
void AcquisitionWindow::onFilterClicked()
{
    if(_filtParam.lowCut > 0.0 && _filtParam.highCut > 0.0 && _filtParam.notchCut > 0.0)
    {
        if(_filtParam.lowCut >= _filtParam.highCut)
        {
            QMessageBox::critical(this, tr("错误"), "滤波频率错误", QMessageBox::Ok);
            return;
        }
        emit doFilt(_filtParam.lowCut, _filtParam.highCut, _filtParam.notchCut);
    }
}

void AcquisitionWindow::setInFilt()
{
    ui->label_5->setText("On");
}
