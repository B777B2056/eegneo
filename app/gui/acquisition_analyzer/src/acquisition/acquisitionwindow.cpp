#include "acquisition/acquisitionwindow.h"
#include "ui_acquisitionwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QSharedMemory>
#include <QStringList>
#include <map>
#include <vector>
#include <sstream>
#include <string>
#include <memory>
#include "settings/setinfo.h"
#include "settings/setchannelname.h"
#include "settings/choosecom.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#define FILE_SAVE_NOT_START 1
#define FILE_SAVE_IN_PROCESS 2
#define FILE_SAVE_FINISHED 3

constexpr const char* BACKEND_PATH = "E:/jr/eegneo/build/app/backend/acquisition/Debug/eegneo_sampler.exe";

AcquisitionWindow::AcquisitionWindow(QWidget *parent)
    : QMainWindow(parent)
    , mSampleRate_(0), mChannelNum_(0)
    , mPlotTimer_(new QTimer(this))
    , mFileSaveFinishedFlag_(FILE_SAVE_NOT_START)
    , ui(new Ui::AcquisitionWindow)
{
    ui->setupUi(this);
    ui->label_5->setText("Off");    // 滤波指示信号初始化：未滤波

    mIpcWrapper_ = new eegneo::utils::IpcWrapper();
    mIpcWrapper_->setMainProcess();
    mIpcWrapper_->setCmdHandler<eegneo::FileSavedFinishedCmd>([this](eegneo::FileSavedFinishedCmd* cmd)->void
    {
        this->mFileSaveFinishedFlag_ = FILE_SAVE_FINISHED;
        QMessageBox::information(this, tr("数据采集"), "文件保存成功", QMessageBox::Ok);
    });

    if (!mIpcWrapper_->start())
    {
        //TODO
        return;
    }

    this->connectSignalAndSlot();
}

AcquisitionWindow::~AcquisitionWindow()
{
    delete mIpcWrapper_;
    delete mPlotTimer_;
    delete mSharedMemory_;
    delete[] mSignalBuf_;
    delete mSignalChart_;
    delete ui;
}

void AcquisitionWindow::start()
{
    // 待用户输入基本信息
    SetInfo siw;
    if(int rec = siw.exec(); QDialog::Accepted == rec)
    {
        this->mChannelNum_ = siw.channelNum();
        this->mFiltCmd_.sampleRate = this->mSampleRate_ = siw.sampleRate();
        this->mFileName_ = siw.subjectNum() + "_" + QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");

        this->startDataSampler();
        this->initSignalChart();
        this->initFFTChart();

        QObject::connect(mPlotTimer_, &QTimer::timeout, [this]()->void{ this->updateWave(); this->updateFFT(); });
        this->mPlotTimer_->start(GRAPH_FRESH);

        this->show();
    }
    else
    {
        emit closeAll();
    }
}

// 发送marker
void AcquisitionWindow::createMark(const QString& event)
{
    if(event.isNull() || event.isEmpty()) return;
    mSignalChart_->addOneMarkerLine(event);
    // 将marker写入文件
    if (mRecCmd_.isRecordOn)
    {
        eegneo::MarkerCmd cmd;
        ::memcpy(cmd.msg, event.toStdString().data(), event.length());
        mIpcWrapper_->sendCmd(cmd);
    }
}

void AcquisitionWindow::startDataSampler()
{
    QStringList args;
    args << QString::number(mChannelNum_);
    mBackend_.start(BACKEND_PATH, args);
    if (mBackend_.waitForStarted(-1))
    {
        mSharedMemory_ = new QSharedMemory{"Sampler"};
        while (!mSharedMemory_->attach());
    }
    else
    {

    }
}

void AcquisitionWindow::stopDataSampler()
{
    mIpcWrapper_->sendCmd(eegneo::ShutdownCmd{});
    mBackend_.close();
    mSharedMemory_->detach();
}

void AcquisitionWindow::setVoltageAxisScale(int curMaxVoltage) { mSignalChart_->setAxisYScale(0, mChannelNum_ * curMaxVoltage); }
void AcquisitionWindow::setTimeAxisScale(std::int8_t t) { mSignalChart_->setAxisXScale(static_cast<eegneo::Second>(t)); }

void AcquisitionWindow::initSignalChart()
{
    this->mSignalBuf_ = new double[this->mChannelNum_];
    this->mSignalChart_= new eegneo::EEGWavePlotter(this->mChannelNum_, this->mSampleRate_, GRAPH_FRESH, mSignalBuf_);

    ui->graphicsView->setChart(this->mSignalChart_->chart());                                   
    this->mSignalChart_->setAxisXScale(eegneo::Second::FIVE);
    this->mSignalChart_->setAxisYScale(0, 10 * mChannelNum_);
}

void AcquisitionWindow::initFFTChart()
{
    this->mFFTChart_ = new eegneo::FFTWavePlotter(this->mChannelNum_, this->mSampleRate_);

    ui->graphicsView_2->setChart(this->mFFTChart_->chart());
    this->mFFTChart_->setAxisXScale(eegneo::Frequency::SIXTY);
    this->mFFTChart_->setAxisYScale(0.0, 50.0);
}

// 波形更新
void AcquisitionWindow::updateWave()
{
    if (!mSharedMemory_->lock()) return;
    ::memcpy(mSignalBuf_, mSharedMemory_->data(), mChannelNum_ * sizeof(double));
    if (!mSharedMemory_->unlock()) return;

    mSignalChart_->update();
}

void AcquisitionWindow::updateFFT()
{
    if (!mSharedMemory_->lock()) return;

    const void* pos = (const void*)((const char*)mSharedMemory_->data() + mChannelNum_ * sizeof(double));

    for (std::size_t i = 0; i < mChannelNum_; ++i)
    {
        auto& real = mFFTChart_->real(i);
        auto& im = mFFTChart_->im(i);
        ::memcpy(real.data(), pos, real.size());
        pos = (const void*)((const char*)pos + real.size());
        ::memcpy(im.data(), pos, im.size());
        pos = (const void*)((const char*)pos + im.size());
    }
    
    if (!mSharedMemory_->unlock()) return;

    mFFTChart_->update();
}

// 信号与槽的链接
void AcquisitionWindow::connectSignalAndSlot()
{
    QObject::connect(ui->filter, &QPushButton::clicked, [this]()->void
    { 
        bool isFilt = ((!mFiltCmd_.isFiltOn) && mFiltCmd_.isValid());
        mFiltCmd_.isFiltOn = isFilt;  
        mIpcWrapper_->sendCmd(mFiltCmd_);    
        ui->label_5->setText(isFilt ? "On" : "Off"); 
    });

    QObject::connect(ui->pushButton, &QPushButton::clicked, [this]()->void
    {
        if(FILE_SAVE_IN_PROCESS == this->mFileSaveFinishedFlag_)
        {
            QMessageBox::critical(this, tr("错误"), "数据正在写入文件", QMessageBox::Ok);
        }
        else
        {
            auto reply = QMessageBox::question(this, tr("通知"), "确定要退出吗？", QMessageBox::Ok | QMessageBox::No);
            if (reply == QMessageBox::Ok)
            {
                this->stopDataSampler();
                this->hide();
                emit closeAll();
            }
        }
    });

    QObject::connect(ui->pushButton_2, &QPushButton::clicked, [this]()->void{ this->createMark(ui->lineEdit->text()); });
    QObject::connect(ui->pushButton_3, &QPushButton::clicked, [this]()->void{ this->createMark(ui->lineEdit_2->text()); });
    QObject::connect(ui->pushButton_4, &QPushButton::clicked, [this]()->void{ this->createMark(ui->lineEdit_3->text()); });
    QObject::connect(ui->pushButton_5, &QPushButton::clicked, [this]()->void{ this->createMark(ui->lineEdit_4->text()); });

    QObject::connect(ui->comboBox, &QComboBox::currentTextChanged, [this](const QString& text)->void{ mFiltCmd_.lowCutoff = text.toDouble(); });
    QObject::connect(ui->comboBox_2, &QComboBox::currentTextChanged, [this](const QString& text)->void{ mFiltCmd_.highCutoff = text.toDouble(); });
    QObject::connect(ui->comboBox_3, &QComboBox::currentTextChanged, [this](const QString& text)->void{ mFiltCmd_.notchCutoff = text.toDouble(); });

    QObject::connect(ui->actionStart_Recording, &QAction::triggered, [this]()->void{ mRecCmd_.isRecordOn = true; mIpcWrapper_->sendCmd(mRecCmd_); });
    QObject::connect(ui->actionStop_Recording, &QAction::triggered, [this]()->void{ mRecCmd_.isRecordOn = false; mIpcWrapper_->sendCmd(mRecCmd_); });

    QObject::connect(ui->actionEDF, &QAction::triggered, [this]()->void{ this->saveToEDFFormatFile(); });

    QObject::connect(ui->actionBDF, &QAction::triggered, [this]()->void{ this->saveToBDFFormatFile(); });
    // QObject::connect(ui->actionp300oddball, SIGNAL(triggered()), this, SLOT(p300Oddball()));

    QObject::connect(ui->action_10_10uV, &QAction::triggered, [this]()->void{ this->setVoltageAxisScale(10); });
    QObject::connect(ui->action_25_25uV, &QAction::triggered, [this]()->void{ this->setVoltageAxisScale(25); });
    QObject::connect(ui->action50uV, &QAction::triggered, [this]()->void{ this->setVoltageAxisScale(50); });
    QObject::connect(ui->action100uV, &QAction::triggered, [this]()->void{ this->setVoltageAxisScale(100); });
    QObject::connect(ui->action200uV, &QAction::triggered, [this]()->void{ this->setVoltageAxisScale(200); });
    QObject::connect(ui->action_500_500uV, &QAction::triggered, [this]()->void{ this->setVoltageAxisScale(500); });
    QObject::connect(ui->action_1000_1000uV, &QAction::triggered, [this]()->void{ this->setVoltageAxisScale(1000); });

    QObject::connect(ui->action0_1s, &QAction::triggered, [this]()->void{ this->setTimeAxisScale(1); });
    QObject::connect(ui->action0_5s, &QAction::triggered, [this]()->void{ this->setTimeAxisScale(5); });
    QObject::connect(ui->action0_10s, &QAction::triggered, [this]()->void{ this->setTimeAxisScale(10); });
}

void AcquisitionWindow::setChannelName()
{
    SetChannelName scl(mChannelNum_);
    if(int rec = scl.exec(); rec == QDialog::Accepted)
    {
        for(std::size_t i = 0; i < mChannelNum_; i++)
        {
            this->mChannelNames_[i] = scl.names[i];
        }
    }
}

void AcquisitionWindow::saveToEDFFormatFile()
{
    QString targetFilePath = QFileDialog::getSaveFileName(this, tr("文件保存路径选择"), this->mFileName_, 
                             tr("EEG Files (*.edf *.EDF)"));
    eegneo::FileSaveCmd cmd;
    cmd.sampleRate = this->mSampleRate_;
    cmd.channelNum = this->mChannelNum_;
    cmd.fileType = eegneo::EDFFileType::EDF;
    ::memcpy(cmd.filePath, targetFilePath.toStdString().c_str(), targetFilePath.length());
    mIpcWrapper_->sendCmd(cmd);
    mFileSaveFinishedFlag_ = FILE_SAVE_IN_PROCESS;
}

void AcquisitionWindow::saveToBDFFormatFile()
{
    QString targetFilePath = QFileDialog::getSaveFileName(this, tr("文件保存路径选择"), this->mFileName_, 
                             tr("EEG Files (*.bdf *.BDF)"));
    eegneo::FileSaveCmd cmd;
    cmd.sampleRate = this->mSampleRate_;
    cmd.channelNum = this->mChannelNum_;
    cmd.fileType = eegneo::EDFFileType::BDF;
    ::memcpy(cmd.filePath, targetFilePath.toStdString().c_str(), targetFilePath.length());
    mIpcWrapper_->sendCmd(cmd);
    mFileSaveFinishedFlag_ = FILE_SAVE_IN_PROCESS;
}

// P300实验结束后保存行为学数据
// void AcquisitionWindow::saveBehavioralP300(const std::string& path)
// {
//     int col = 0;
//     std::ifstream events_read;
//     std::vector<int> blank_acc, blank_rt, blank_resp, on_set_time, blank_rttime;
//     std::vector<std::string> code;
//     std::vector<std::pair<double, std::string>> vec;
//     events_read.open(_fileInfo.tempFiles + "_events.txt");
//     while(events_read.peek() != EOF)
//     {
//         std::string str;
//         std::getline(events_read, str);
//         if(col)
//         {
//             long long run_time;
//             std::string event;
//             std::stringstream ss(str);
//             ss >> run_time >> event;
//             vec.push_back(std::make_pair(run_time, event));
//         }
//         ++col;
//     }
//     for(std::size_t i = 0; i < vec.size() - 1; ++i)
//     {
//         if(vec[i].second == "2")
//         {
//             if(vec[i + 1].second == "Response")
//             {
//                 // 对2有反应，说明反应正确
//                 blank_acc.push_back(1);
//                 // 计算反应时间，单位为毫秒，在表格中对应2的位置
//                 blank_rt.push_back((int)((vec[i + 1].first - vec[i].first) / 10));
//                 // 对2有反应，按键正确
//                 blank_resp.push_back(1);
//                 // 反应动作的时间，毫秒
//                 blank_rttime.push_back((int)(vec[i + 1].first / 10));
//             }
//             else
//             {
//                 blank_acc.push_back(0);
//                 blank_resp.push_back(0);
//                 blank_rt.push_back(0);
//                 blank_rttime.push_back(0);
//             }
//             code.push_back("2");
//             on_set_time.push_back((int)(vec[i].first / 10));
//         }
//         else if(vec[i].second == "8")
//         {
//             // 对8有反应，说明反应错误
//             if(vec[i + 1].second == "Response")
//             {
//                 blank_acc.push_back(0);
//                 blank_resp.push_back(1);
//                 blank_rt.push_back((int)((vec[i + 1].first - vec[i].first) / 10));
//                 blank_rttime.push_back((int)(vec[i + 1].first / 10));
//             }
//             // 对8无反应，说明反应正确
//             else
//             {
//                 blank_acc.push_back(1);
//                 blank_resp.push_back(-1);
//                 blank_rt.push_back(-1);
//                 blank_rttime.push_back(-1);
//             }
//             code.push_back("8");
//             on_set_time.push_back((int)(vec[i].first / 10));
//         }
//     }
//     events_read.close();
//     std::string file_name = this->mFileName_.toStdString();
//     std::ofstream beh_file;
//     beh_file.open(path + "\\" + file_name + "_behavioral.csv");
//     beh_file.close();
//     beh_file.open(path + "\\" + file_name + "_behavioral.csv", std::ios::app);
//     beh_file << "Experiment Name: " << "NO NAME SETED"
//              << ", Date: " << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss").toStdString()
//              << std::endl;
//     beh_file << "Subject,Trial,blank.ACC,blank.CRESP,blank.OnsetTime,blank.RESP,blank.RT,blank.RTTime,code,correct,pic" << std::endl;
//     // 预实验
//     for(int i = 0; i < 40; i++)
//     {
//         std::string pre_exp = this->mFileName_.toStdString() + "," + std::to_string(i + 1);
//         for(int j = 0; j < 9; j++)
//         {
//             pre_exp += ",";
//         }
//         beh_file << pre_exp << std::endl;
//     }
//     // 正式实验
//     for(int i = 0; i < _p300OddballImgNum; i++)
//     {
//         //Subject
//         beh_file << this->mFileName_.toStdString();
//         beh_file << ",";
//         //Trial
//         beh_file << std::to_string(i + 1);
//         beh_file << ",";
//         //blank.ACC
//         beh_file << blank_acc[i];
//         beh_file << ",";
//         //blank.CRESP
//         if(code[i] == "2")
//             beh_file << "1";
//         beh_file << ",";
//         //blank.OnsetTime
//         if(code[i] == "2")
//             beh_file << on_set_time[i];
//         beh_file << ",";
//         //blank.RESP
//         if(blank_resp[i] == 1)
//             beh_file << blank_resp[i];
//         beh_file << ",";
//         //blank.RT
//         if(code[i] == "2" || blank_resp[i] == 1)
//             beh_file << blank_rt[i];
//         else
//             beh_file << 0;
//         beh_file << ",";
//         //blank.RTTime
//         if(code[i] == "2" || blank_resp[i] == 1)
//             beh_file << blank_rttime[i];
//         else
//             beh_file << 0;
//         beh_file << ",";
//         //code
//         beh_file << code[i];
//         beh_file << ",";
//         //correct
//         if(code[i] == "2")
//             beh_file << "1";
//         beh_file << ",";
//         //pic
//         if(code[i] == "2")
//             beh_file << "2.bmp" << std::endl;
//         else
//             beh_file << "8.bmp" << std::endl;
//     }
//     beh_file.close();
// }
