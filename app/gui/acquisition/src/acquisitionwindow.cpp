#include "acquisitionwindow.h"
#include "ui_acquisitionwindow.h"
#include <cstdlib>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QSharedMemory>
#include <QStringList>
#include "setinfo.h"
#include "utils/config.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

namespace 
{
    enum FileSaveState : std::uint8_t
    {
        FILE_SAVE_NOT_START = 1,
        FILE_SAVE_IN_PROCESS,
        FILE_SAVE_FINISHED
    };

    constexpr std::uint16_t GRAPH_FRESH_FREQ = 50;   // 触发波形显示定时器的时间，单位为ms
}

AcquisitionWindow::AcquisitionWindow(QWidget *parent)
    : QMainWindow(parent)
    , mSampleRate_(0), mChannelNum_(0)
    , mPlotTimer_(new QTimer())
    , mIpcWrapper_(nullptr)
    , mFileSaveFinishedFlag_(FILE_SAVE_NOT_START)
    , ui(new Ui::AcquisitionWindow)
{
    ui->setupUi(this);
    this->initUI();
    this->initIPCSvr();
    this->connectSignalAndSlot();
}

AcquisitionWindow::~AcquisitionWindow()
{
    mPlotTimer_->stop();
    delete mIpcWrapper_;
    delete mPlotTimer_;
    delete mSharedMemory_;
    delete[] mSignalBuf_;
    delete mSignalPlotter_;
    delete mFFTPlotter_;
    delete mTopoPlotter_;
    delete ui;
}

void AcquisitionWindow::showEvent(QShowEvent *event)
{
    this->mTopoPlotter_->showEvent();
    QMainWindow::showEvent(event);
}

void AcquisitionWindow::initUI()
{
    ui->label_5->setText("Off");    // 滤波指示信号初始化：未滤波
    mTopoPlotter_ = new eegneo::TopographyPlotter(ui->graphicsView_3);
    ui->graphicsView_3->show();
}

void AcquisitionWindow::initIPCSvr()
{
    auto& config = eegneo::utils::ConfigLoader::instance();
    auto port = config.get<std::uint16_t>("IpcServerIpPort");
    mIpcWrapper_ = new eegneo::utils::IpcService(port);
    
    mIpcWrapper_->setSessionHandler(eegneo::SessionId::AccquisitionInnerSession, 
    [this](eegneo::utils::IpcClient* channel)->void
    {
        channel->setCmdHandler<eegneo::FileSavedFinishedCmd>([this](eegneo::FileSavedFinishedCmd* cmd)->void
        {
            this->mFileSaveFinishedFlag_ = FILE_SAVE_FINISHED;
            QMessageBox::information(this, tr("数据采集"), "文件保存成功", QMessageBox::Ok);
        });

        channel->setCmdHandler<eegneo::ErrorCmd>([this](eegneo::ErrorCmd* cmd)->void
        {
            QMessageBox::warning(this, tr("警告"), cmd->errmsg, QMessageBox::Ok);
        });
    });

    mIpcWrapper_->setSessionHandler(eegneo::SessionId::ERPSession,
    [this](eegneo::utils::IpcClient* channel)->void
    {
        channel->setCmdHandler<eegneo::MarkerCmd>([this](eegneo::MarkerCmd* cmd)->void
        {
            this->createMark(cmd->msg);
        });
        
        channel->setCmdHandler<eegneo::ErrorCmd>([this](eegneo::ErrorCmd* cmd)->void
        {
            QMessageBox::warning(this, tr("警告"), cmd->errmsg, QMessageBox::Ok);
        });
    });
}

void AcquisitionWindow::show()
{
    // 待用户输入基本信息
    SetInfo siw;
USER_INPUT:
    if(int rec = siw.exec(); QDialog::Accepted == rec)
    {   
        if (!siw.isValid())
        {
            QMessageBox::critical(this, tr("错误"), "参数设置错误", QMessageBox::Ok);
            goto USER_INPUT;
        }
        this->mChannelNum_ = siw.channelNum();
        this->mSampleRate_ = siw.sampleRate();
        this->mFileName_ = siw.subjectNum() + "_" + QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");

        this->startDataSampler();
        this->initSignalChart();
        this->initFFTChart();

        this->mPlotTimer_->start(GRAPH_FRESH_FREQ);

        QMainWindow::show();
    }
    else
    {
        this->close();
        std::exit(0);
    }
}

void AcquisitionWindow::createMark(const QString& event)
{
    if(event.isNull() || event.isEmpty()) return;
    mSignalPlotter_->addOneMarkerLine(event);
    // 将marker写入文件
    if (mRecCmd_.isRecordOn)
    {
        eegneo::MarkerCmd cmd;
        ::memcpy(cmd.msg, event.toStdString().data(), event.length());
        mIpcWrapper_->session(eegneo::SessionId::AccquisitionInnerSession)->sendCmd(cmd);
    }
}

void AcquisitionWindow::startDataSampler()
{
    QStringList args;
    args << QString::number(mChannelNum_) << QString::number(mSampleRate_);

    QObject::connect(&mBackend_, &QProcess::started, [this]()->void
    {
        mSharedMemory_ = new QSharedMemory{"Sampler"};
        while (!mSharedMemory_->attach());
    });

    mBackend_.start(_BACKEND_EXE_PATH, args);
}

void AcquisitionWindow::stopDataSampler()
{
    mIpcWrapper_->session(eegneo::SessionId::AccquisitionInnerSession)->sendCmd(eegneo::ShutdownCmd{});
    mBackend_.close();
    mSharedMemory_->detach();
}

void AcquisitionWindow::setVoltageAxisScale(int curMaxVoltage) { mSignalPlotter_->setAxisYScale(curMaxVoltage ); }
void AcquisitionWindow::setTimeAxisScale(std::int8_t t) { mSignalPlotter_->setAxisXScale(static_cast<eegneo::Second>(t)); }

void AcquisitionWindow::initSignalChart()
{
    this->mSignalBuf_ = new double[this->mChannelNum_];
    this->mSignalPlotter_= new eegneo::EEGWavePlotter(this->mChannelNum_, this->mSampleRate_, GRAPH_FRESH_FREQ, mSignalBuf_);

    ui->graphicsView->setChart(this->mSignalPlotter_->chart());                                   
    this->mSignalPlotter_->setAxisXScale(eegneo::Second::FIVE);
    this->mSignalPlotter_->setAxisYScale(10);
}

void AcquisitionWindow::initFFTChart()
{
    this->mFFTPlotter_ = new eegneo::FFTWavePlotter(this->mChannelNum_, this->mSampleRate_);
    ui->graphicsView_2->setChart(this->mFFTPlotter_->chart());
    this->mFFTPlotter_->setAxisXScale(eegneo::Frequency::SIXTY);
    this->mFFTPlotter_->setAxisYScale(50);
}

// 波形更新
void AcquisitionWindow::updateEEG()
{
    if (!mSharedMemory_->lock()) return;
    ::memcpy(mSignalBuf_, mSharedMemory_->data(), mChannelNum_ * sizeof(double));
    if (!mSharedMemory_->unlock()) return;

    mSignalPlotter_->update();
}

void AcquisitionWindow::updateFFT()
{
    if (!mSharedMemory_->lock()) return;

    const void* pos = (const void*)((const char*)mSharedMemory_->data() + mChannelNum_ * sizeof(double));

    for (std::size_t i = 0; i < mChannelNum_; ++i)
    {
        auto& real = mFFTPlotter_->real(i);
        auto& im = mFFTPlotter_->im(i);
        ::memcpy(real.data(), pos, real.size());
        pos = (const void*)((const char*)pos + real.size());
        ::memcpy(im.data(), pos, im.size());
        pos = (const void*)((const char*)pos + im.size());
    }
    
    if (!mSharedMemory_->unlock()) return;

    mFFTPlotter_->update();
}

void AcquisitionWindow::updateTopography()
{
    mTopoPlotter_->update();
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
    mIpcWrapper_->session(eegneo::SessionId::AccquisitionInnerSession)->sendCmd(cmd);
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
    mIpcWrapper_->session(eegneo::SessionId::AccquisitionInnerSession)->sendCmd(cmd);
    mFileSaveFinishedFlag_ = FILE_SAVE_IN_PROCESS;
}

// 信号与槽的链接
void AcquisitionWindow::connectSignalAndSlot()
{
    QObject::connect(mPlotTimer_, &QTimer::timeout, [this]()->void
    { 
        this->updateEEG(); 
        this->updateFFT(); 
        this->updateTopography();
    });

    QObject::connect(ui->filter, &QPushButton::clicked, [this]()->void
    { 
        bool isFilt = ((!mFiltCmd_.isFiltOn) && mFiltCmd_.isValid());
        mFiltCmd_.sampleRate = this->mSampleRate_;
        mFiltCmd_.isFiltOn = isFilt;  
        mIpcWrapper_->session(eegneo::SessionId::AccquisitionInnerSession)->sendCmd(mFiltCmd_);    
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
                this->close();
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

    QObject::connect(ui->actionStart_Recording, &QAction::triggered, [this]()->void{ mRecCmd_.isRecordOn = true; mIpcWrapper_->session(eegneo::SessionId::AccquisitionInnerSession)->sendCmd(mRecCmd_); });
    QObject::connect(ui->actionStop_Recording, &QAction::triggered, [this]()->void{ mRecCmd_.isRecordOn = false; mIpcWrapper_->session(eegneo::SessionId::AccquisitionInnerSession)->sendCmd(mRecCmd_); });

    QObject::connect(ui->actionEDF, &QAction::triggered, [this]()->void{ this->saveToEDFFormatFile(); });

    QObject::connect(ui->actionBDF, &QAction::triggered, [this]()->void{ this->saveToBDFFormatFile(); });

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
