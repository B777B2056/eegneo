#include "acquisition/acquisitionwindow.h"
#include "ui_acquisitionwindow.h"
#include "ui_charthelp.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QSharedMemory>
#include <QStringList>
#include <map>
#include <vector>
#include <sstream>
#include <memory>
#include "settings/setinfo.h"
#include "settings/setchannelname.h"
#include "erp/p300.h"
#include "settings/choosecom.h"

extern "C"
{
    #include "third/edflib/edflib.h"
}

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

const static double highPassFres[7] = {0.1, 0.3, 3.5, 8.0, 12.5, 16.5, 20.5};  // 高通滤波频率选择
const static double lowPassFres[7] = {8.0, 12.5, 16.5, 20.5, 28.0, 45.0, 50.0};  // 低通滤波频率选择

AcquisitionWindow::AcquisitionWindow(QWidget *parent)
    : QMainWindow(parent)
    , mSampleRate_(0), mChannelNum_(0)
    , mPlotTimer_(new QTimer(this)), mSharedMemory_(nullptr), mChart_(nullptr)
    , ui(new Ui::AcquisitionWindow)
{
    ui->setupUi(this);
    ui->label_5->setText("Off");    // 滤波指示信号初始化：未滤波
    this->setSignalSlotConnect();
}

AcquisitionWindow::~AcquisitionWindow()
{
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
    delete mPlotTimer_;
    delete mSharedMemory_;
    delete[] mBuf_;
    delete mChart_;
    delete ui;
}

void AcquisitionWindow::setSignalSlotConnect()
{
    // 信号与槽的链接：太尼玛恶心了，谁看谁傻逼
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
}

void AcquisitionWindow::showParticipantInfoWindow()
{
    // 待用户输入基本信息
    SetInfo siw;
    if(int rec = siw.exec(); QDialog::Accepted == rec)
    {
        this->mChannelNum_ = siw.channelNum();
        this->mSampleRate_ = siw.sampleRate();
        this->mFileName_ = siw.subjectNum() + QDateTime::currentDateTime().toString("yyyy.MM.dd.hh:mm:ss");
        this->mBuf_ = new double[this->mChannelNum_];
        this->mChart_ = new eegneo::EEGWavePlotImpl(this->mChannelNum_);
        this->startDataSampler();
        this->initChart();
        this->show();

        this->_fileInfo.tempFiles = mFileName_.toStdString();
    }
    else
    {
        emit returnMain();
    }
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
}

void AcquisitionWindow::setFilePath(int s, QString& path)
{
    path = QFileDialog::getExistingDirectory(this, tr("文件保存路径选择"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                               | QFileDialog::DontResolveSymlinks);
    path += ("/" + this->mFileName_);
    path.replace("/", "\\");
    // 询问用户是否采用默认的通道名称(仅EDF可用)
    if(!s)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("通道名称"),
                                        "是否自定义通道名称?\n默认名称为数字编号，例如1~8。",
                                        QMessageBox::Ok | QMessageBox::No);
        if (reply == QMessageBox::Ok){
            // 弹窗口让用户输入各通道名称
            SetChannelName scl(mChannelNum_);
      Again:
            int rec = scl.exec();
            if(rec == QDialog::Accepted)
            {
                for(int i = 0; i < mChannelNum_; i++)
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
            for(int i = 0; i < mChannelNum_; i++)
            {
                _fileInfo.channelNames[i] = std::to_string(i + 1);
            }
        }
    }
    else
    {
        // 各通道默认标签
        for(int i = 0; i < mChannelNum_; i++)
        {
            _fileInfo.channelNames[i] = std::to_string(i + 1);
        }
    }
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
    QString edf_path;
    std::ifstream samples_read;  // 8通道缓存txt文件输入流
    std::ifstream events_read;  // 标记缓存txt文件输入流
    double *buf_persec = new double[mChannelNum_ * mSampleRate_];
    std::string file_name = this->mFileName_.toStdString();
    setFilePath(0, edf_path);
    // 新建文件夹
    QDir dir;
    if (!dir.exists(edf_path))
    {
        dir.mkpath(edf_path);

    }
    // 设置EDF文件参数
    {
        std::string edf_path_temp = edf_path.toStdString();
        _fileInfo.flag = ::edfopen_file_writeonly((edf_path_temp + "\\" + file_name + ".edf").c_str(), 
                                                EDFLIB_FILETYPE_EDFPLUS,
                                                static_cast<int>(mChannelNum_));
    }
    for(i = 0; i < mChannelNum_; i++)
    {
        // 设置各通道采样率
        ::edf_set_samplefrequency(_fileInfo.flag, i, static_cast<int>(mSampleRate_));
        // 设置信号最大与最小数字值(EDF为16位文件，一般设置为-32768~32767)
        ::edf_set_digital_maximum(_fileInfo.flag, i, 32767);
        ::edf_set_digital_minimum(_fileInfo.flag, i, -32768);
        // 设置信号最大与最小物理值(即信号在物理度量上的最大、最小值)
        // ::edf_set_physical_maximum(_fileInfo.flag, i, _waveDrawingInfo.maxVoltage);
        // ::edf_set_physical_minimum(_fileInfo.flag, i, -_waveDrawingInfo.maxVoltage);
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
            for(int row = 0; row < mSampleRate_; row++)
            {
              if(mSampleRate_ * row + col < mChannelNum_ * mSampleRate_)
                  ss >> buf_persec[mSampleRate_ * row + col];
            }
            /*1s结束*/
            if(!((col) % mSampleRate_))
            {
                edf_blockwrite_physical_samples(_fileInfo.flag, buf_persec);
                col = 0;
            }
        }
        ++col;
    }
    // 写入多余的空数据以保证标记存在
    for(i = 0; i < mChannelNum_ * mSampleRate_; i++)
    {
       buf_persec[i] = 0.0;
    }
    for(i = 0; i < _fileInfo.eventCount - _fileInfo.curLine / mSampleRate_ + 1; i++)
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
        saveBehavioralP300(edf_path.toStdString());
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
    QString txt_path;
    std::string file_name = this->mFileName_.toStdString();
    std::ifstream samples_read;  // 8通道缓存txt文件输入流
    std::ofstream samples_txt;  // 数据点txt文件输出流
    std::ifstream events_read;
    std::ofstream events_txt_eeglab;  // eeglab风格
    std::ofstream events_txt_brainstorm;  // brainstorm风格
    setFilePath(1, txt_path);
    // 新建文件夹，把三个txt放入一个文件夹
    if (QDir dir; !dir.exists(txt_path))
    {
        dir.mkpath(txt_path);
    }
    // 写入数据点
    int col = 0;
    samples_txt.open((txt_path.toStdString() + "\\" + file_name + "_samples.txt"));
    samples_txt.close();
    samples_read.open(_fileInfo.tempFiles + "_samples.txt");
    samples_txt.open((txt_path.toStdString() + "\\" + file_name + "_samples.txt"), std::ios::app);
    for(int i = 0; i < mChannelNum_; i++)
    {
        if(i < mChannelNum_ - 1)
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
    events_txt_eeglab.open((txt_path.toStdString() + "\\" + file_name + "_events(eeglab).txt"));
    events_txt_eeglab.close();
    events_read.open(_fileInfo.tempFiles + "_events.txt");
    events_txt_eeglab.open((txt_path.toStdString() + "\\" + file_name + "_events(eeglab).txt"), std::ios::app);
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
    events_txt_brainstorm.open((txt_path.toStdString() + "\\" + file_name + "_events(brainstorm).txt"));
    events_txt_brainstorm.close();
    events_read.open(_fileInfo.tempFiles + "_events.txt");
    events_txt_brainstorm.open((txt_path.toStdString() + "\\" + file_name + "_events(brainstorm).txt"), std::ios::app);
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
    readme.open(txt_path.toStdString() + "\\" + file_name + "_readme.txt");
    readme.close();
    readme.open(txt_path.toStdString() + "\\" + file_name + "_readme.txt", std::ios::app);
    readme << "Data sampling rate: " << mSampleRate_ << std::endl;
    readme << "Number of channels: " << mChannelNum_ << std::endl;
    readme << "Input field(column) names: latency type" << std::endl;
    readme << "Number of file header lines: 1" << std::endl;
    readme << "Time unit(sec): 1" << std::endl;
    readme.close();
    // 保存行为学数据
    if(_fileInfo.isSaveP300BH)
    {
        saveBehavioralP300(txt_path.toStdString());
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
    std::string file_name = this->mFileName_.toStdString();
    std::ofstream beh_file;
    beh_file.open(path + "\\" + file_name + "_behavioral.csv");
    beh_file.close();
    beh_file.open(path + "\\" + file_name + "_behavioral.csv", std::ios::app);
    beh_file << "Experiment Name: " << "NO NAME SETED"
             << ", Date: " << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss").toStdString()
             << std::endl;
    beh_file << "Subject,Trial,blank.ACC,blank.CRESP,blank.OnsetTime,blank.RESP,blank.RT,blank.RTTime,code,correct,pic" << std::endl;
    // 预实验
    for(int i = 0; i < 40; i++)
    {
        std::string pre_exp = this->mFileName_.toStdString() + "," + std::to_string(i + 1);
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
        beh_file << this->mFileName_.toStdString();
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
    // _waveDrawingInfo.charts[0]->addSeries(line);
    // _waveDrawingInfo.charts[0]->setAxisX(_waveDrawingInfo.axisX[0], line);
    // _waveDrawingInfo.charts[0]->setAxisY(_waveDrawingInfo.axisY[0], line);
    // 添加marker文字注释
    QGraphicsSimpleTextItem *pItem = new QGraphicsSimpleTextItem(mChart_->chart());
    pItem->setText(QString::fromStdString(event) + "\n" + QDateTime::currentDateTime().toString("hh:mm:ss"));
    _markerInfo.marks[line] = std::make_pair(QDateTime::currentDateTime().toMSecsSinceEpoch(), pItem);
    // 将marker写入文件
    if(_fileInfo.isRec)
    {
        emit writeEvent(event);
        _fileInfo.eventCount++;
    }
}

void AcquisitionWindow::startDataSampler()
{
    QStringList args;
    args << QString::number(mChannelNum_);
    mDataSampler_.start("E:/jr/eegneo/build/sampler/Debug/eegneo_sampler.exe", args);
    if (mDataSampler_.waitForStarted(-1))
    {
        mSharedMemory_ = new QSharedMemory{"Sampler"};
        while (!mSharedMemory_->attach())
        {

        }
    }
}

void AcquisitionWindow::lineEditHelper(int N)
{
    std::array<decltype (ui->lineEdit), 4> lineEditUi = {ui->lineEdit, ui->lineEdit_2, ui->lineEdit_3, ui->lineEdit_4};
    if(!lineEditUi[N]->text().isEmpty())
    {
        _markerInfo.markerNames[N] = lineEditUi[N]->text().toStdString();
    }
}

void AcquisitionWindow::initChart()
{
    ui->graphicsView->setChart(mChart_->chart());                                   
    // ui->graphicsView->setRenderHint(QPainter::Antialiasing);  

    mChart_->setAxisXScale(eegneo::Second::FIVE);
    mChart_->setAxisYScale(0, 10 * mChannelNum_);

    QObject::connect(mPlotTimer_, SIGNAL(timeout()), this, SLOT(graphFresh()));
    mPlotTimer_->start(GRAPH_FRESH);
}

// 波形更新
void AcquisitionWindow::updateWave()
{
    if (!mSharedMemory_->lock()) return;
    ::memcpy(mBuf_, mSharedMemory_->data(), mChannelNum_ * sizeof(double));
    if (!mSharedMemory_->unlock()) return;

    mChart_->update(mBuf_);
}

// 数据获取
// void AcquisitionWindow::receiveData(std::vector<double> vec)
// {
//     _waveDrawingInfo.graphData = vec;
//     if(_fileInfo.isRec)
//     {
//         _fileInfo.curLine++;
//     }
// }

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
        marker_point.append(QPointF((iter->second).first, 0));
        marker_point.append(QPointF((iter->second).first, 80));
        iter->first->replace(marker_point);
        // 文字标记
        (iter->second).second->setPos(mChart_->chart()->mapToPosition(QPointF((iter->second).first, 50.0), iter->first));
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
        this->hide();
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
        // emit doFilt(_filtParam.lowCut, _filtParam.highCut, _filtParam.notchCut);
    }
}

void AcquisitionWindow::setInFilt()
{
    ui->label_5->setText("On");
}
