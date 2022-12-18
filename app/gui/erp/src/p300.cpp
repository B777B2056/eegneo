#include "p300.h"
#include "ui_p300.h"
#include <chrono>
#include <cstring>
#include <QString>
#include "common/common.h"
#include "utils/config.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

static QString RESOURCE_ROOT_PATH = ":/erp/p300/resource/p300/oddball/";

ErpP300OddballWindow::ErpP300OddballWindow(QWidget *parent)
    : QMainWindow(parent)
    , mIpc_(nullptr)
    , mIsInPractice_(true), mIsInExperiment_(false), mIsEnded_(false), mStimulusImageCount_(0)
    , ui(new Ui::p300)
{
    ui->setupUi(this);
    // 设置全黑背景色
    QPalette palette(this->palette());
    palette.setColor(QPalette::Window, Qt::black);
    this->setPalette(palette);
    // 显示准备界面
    ui->label->setAlignment(Qt::AlignCenter);
    ui->label->setPixmap(QPixmap(RESOURCE_ROOT_PATH + "gotask.jpg"));
    ui->label->show();
    this->setCentralWidget(ui->label);
    // 初始化
    this->init(); 
}

ErpP300OddballWindow::~ErpP300OddballWindow()
{
    delete mIpc_;
    delete ui;
}

void ErpP300OddballWindow::init()
{
    auto& config = eegneo::utils::ConfigLoader::instance();
    // 设置实验参数
    this->mImagesInPracticeTotalCount_ = config.get<int>("ERP", "ImagesInPracticeTotalCount");
    this->mImagesInExperimentTotalCount_ = config.get<int>("ERP", "ImagesInExperimentTotalCount");
    this->mStimulusImageRatio_ = config.get<double>("ERP", "StimulusImageRatio");
    this->mCrossDurationMs_ = config.get<int>("ERP", "CrossDurationMs");
    this->mImageDurationMs_ = config.get<int>("ERP", "ImageDurationMs");
    this->mBlankDurationMsLowerBound_ = config.get<int>("ERP", "BlankDurationMsLowerBound");
    this->mBlankDurationMsUpperBound_ = config.get<int>("ERP", "BlankDurationMsUpperBound");
    // 采集软件所在电脑的Ip地址和端口号
    auto ip = config.get<std::string>("ERP", "AcquisitionIpAddr");
    auto port = config.get<std::uint16_t>("IpcServerIpPort");
    this->mIpc_ = new eegneo::utils::IpcClient(eegneo::SessionId::ERPSession, ip.c_str(), port);
    // 是否全屏显示
    if (config.get<bool>("ERP", "IsFullScreen"))   this->showFullScreen();
}

void ErpP300OddballWindow::keyPressEvent(QKeyEvent *event)
{
    // 空按格或数字1开始练习
    if(this->mIsInPractice_ && ((event->key() == Qt::Key_Space) || (event->key() == Qt::Key_1)))
    {
        this->playImagesRound(this->mImagesInPracticeTotalCount_);
        this->practiceEnd();
    }
    // 按2开始正式实验
    if(!this->mIsInPractice_ && (event->key() == Qt::Key_2))
    {
        mIsInExperiment_ = true;
        this->sendMarker("Start");
        this->playImagesRound(this->mImagesInExperimentTotalCount_);
        this->experimentEnd();
    }
    // 结束后可按Esc退出全屏
    if(this->mIsEnded_ && (event->key() == Qt::Key_Escape))
    {
        this->showNormal();
    }
}

void ErpP300OddballWindow::mousePressEvent(QMouseEvent *event)
{
    if(this->mIsInExperiment_ && (event->button() == Qt::LeftButton))
    {
        this->sendMarker("Response");
    }
}

void ErpP300OddballWindow::sendMarker(const char* msg)
{
    eegneo::MarkerCmd cmd;
    ::memcpy(cmd.msg, msg, std::strlen(msg));
    mIpc_->sendCmd(cmd);
}

static void DelayMs(int ms)
{
    for(auto t1 = std::chrono::steady_clock::now(); std::chrono::steady_clock::now() < t1 + std::chrono::milliseconds(ms); )
    {
        QApplication::processEvents();
    }
}

ErpP300OddballWindow::ImgLabel ErpP300OddballWindow::chooseImg(int imgNumRound) const
{
    int n = std::rand() % imgNumRound;
    if((n < (int)(imgNumRound * this->mStimulusImageRatio_)) && (this->mStimulusImageCount_ < (int)(imgNumRound * this->mStimulusImageRatio_)))
    {
        return Stimulation;
    }
    else
    {   
        return NonStimulation;
    }
}

void ErpP300OddballWindow::playImagesRound(int imgNumRound)
{
    for (int i = 0; i < imgNumRound; ++i)
    {
        // 十字准星显示周期（默认）：800ms
        ui->label->setVisible(true);
        ui->label->setPixmap(QPixmap(RESOURCE_ROOT_PATH + "cross.png"));
        ::DelayMs(this->mCrossDurationMs_);
        // 数字显示周期（默认）：50ms
        if(Stimulation == this->chooseImg(imgNumRound))
        {
            ui->label->setPixmap(QPixmap(RESOURCE_ROOT_PATH + "2.bmp"));
            ++this->mStimulusImageCount_;
            if(!this->mIsInPractice_)    this->sendMarker("2");
        }
        else
        {
            ui->label->setPixmap(QPixmap(RESOURCE_ROOT_PATH + "8.bmp"));
            if(!this->mIsInPractice_)    this->sendMarker("8");
        }
        ::DelayMs(this->mImageDurationMs_);
        // 空白周期（默认）：1000ms-1200ms
        ui->label->setVisible(false);
        ::DelayMs(this->mBlankDurationMsLowerBound_ + std::rand() % (this->mBlankDurationMsUpperBound_ - this->mBlankDurationMsLowerBound_));
    }
}

void ErpP300OddballWindow::practiceEnd()
{
    this->mIsInPractice_ = false;
    // 显示选择界面
    ui->label->setVisible(true);
    ui->label->setPixmap(QPixmap(RESOURCE_ROOT_PATH + "ifgo.jpg"));
    // 计数器重置
    this->mStimulusImageCount_ = 0;
}

void ErpP300OddballWindow::experimentEnd()
{
    // 显示结束界面
    ui->label->setVisible(true);
    ui->label->setPixmap(QPixmap(RESOURCE_ROOT_PATH + "end.jpg"));
    this->sendMarker("End");
    // 退出全屏标志位
    this->mIsInPractice_ = false;
    this->mIsInExperiment_ = false;
    this->mIsEnded_ = true;
}
