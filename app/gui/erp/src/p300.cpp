#include "p300.h"
#include "ui_p300.h"
#include <cstring>
#include <sstream>
#include <fstream>

#if !defined(__unix__) && !defined(_MSC_VER)
    #include <mutex>
#endif

#include <QMessageBox>
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
        this->mIsInExperiment_ = true;
        this->sendMarker("Start");
        this->mExpStartTime_ = std::chrono::steady_clock::now();
        this->playImagesRound(this->mImagesInExperimentTotalCount_);
        this->sendMarker("End");
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
    // 记录时刻与时间
    auto curTime = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - this->mExpStartTime_).count();
    this->mEvents_.emplace_back(std::make_tuple(curTime, msg));
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
    // 保存行为学数据
    this->saveBehavioral();
    // 显示结束界面
    ui->label->setVisible(true);
    ui->label->setPixmap(QPixmap(RESOURCE_ROOT_PATH + "end.jpg"));
    // 退出全屏标志位
    this->mIsInPractice_ = false;
    this->mIsInExperiment_ = false;
    this->mIsEnded_ = true;
}

void ErpP300OddballWindow::saveBehavioral()
{
    this->writeIntoBehavioralFile(this->constructBehavioralData());
}

ErpP300OddballWindow::BehavioralInfo ErpP300OddballWindow::constructBehavioralData() const
{
    BehavioralInfo info;
    for(std::size_t i = 0; i < mEvents_.size() - 1; ++i)
    {
        const auto& [curTime, curEvent] = mEvents_[i];
        const auto& [nextTime, nextEvent] = mEvents_[i + 1];
        if(curEvent == "2")
        {
            if(nextEvent == "Response")
            {
                // 对2有反应，说明反应正确
                info.blankAcc.push_back(1);
                // 计算反应时间，单位为毫秒，在表格中对应2的位置
                info.blankRt.push_back((int)((nextTime - curTime)));
                // 对2有反应，按键正确
                info.blankResp.push_back(1);
                // 反应动作的时间，毫秒
                info.blankRtTime.push_back((int)(nextTime));
            }
            else
            {
                info.blankAcc.push_back(0);
                info.blankResp.push_back(0);
                info.blankRt.push_back(0);
                info.blankRtTime.push_back(0);
            }
            info.code.push_back("2");
            info.onSetTime.push_back((int)(curTime));
        }
        else if(curEvent == "8")
        {
            // 对8有反应，说明反应错误
            if(nextEvent == "Response")
            {
                info.blankAcc.push_back(0);
                info.blankRt.push_back((int)((nextTime - curTime)));
                info.blankResp.push_back(1);
                info.blankRtTime.push_back((int)(nextTime));
            }
            // 对8无反应，说明反应正确
            else
            {
                info.blankAcc.push_back(1);
                info.blankResp.push_back(-1);
                info.blankRt.push_back(-1);
                info.blankRtTime.push_back(-1);
            }
            info.code.push_back("8");
            info.onSetTime.push_back((int)(curTime));
        }
    }
    return info;
}

static std::tm localtime_xp(std::time_t timer)
{
    std::tm bt {};
#if defined(__unix__)
    ::localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
    ::localtime_s(&bt, &timer);
#else
    static std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    bt = *std::localtime(&timer);
#endif
    return bt;
}

static std::string FormatTimePoint(const std::string& fmt = "%Y_%m_%d_%X")
{
    auto bt = localtime_xp(std::time(0));
    char buf[64];
    return {buf, std::strftime(buf, sizeof(buf), fmt.c_str(), &bt)};
}

void ErpP300OddballWindow::writeIntoBehavioralFile(const BehavioralInfo& info)
{
    std::string todayDateStr = ::FormatTimePoint();
    std::fstream beh_file(todayDateStr + "_behavioral.csv", std::ios::out);
    if (!beh_file.is_open())
    {
        QMessageBox::critical(this, tr("错误"), "行为学数据无法保存：无权限操作文件", QMessageBox::Ok);
        return;
    }
    beh_file << "Experiment Name: " << "NO NAME SETED"
             << ", Date: " << todayDateStr
             << std::endl;
    beh_file << "Subject,Trial,blank.ACC,blank.CRESP,blank.OnsetTime,blank.RESP,blank.RT,blank.RTTime,code,correct,pic" << std::endl;
    // 预实验
    for(int i = 0; i < this->mImagesInPracticeTotalCount_; ++i)
    {
        std::string pre_exp{"Subject"};
        pre_exp += ("," + std::to_string(i + 1));
        for(int j = 0; j < 9; j++)
        {
            pre_exp += ",";
        }
        beh_file << pre_exp << std::endl;
    }
    // 正式实验
    for(int i = 0; i < this->mImagesInExperimentTotalCount_; i++)
    {
        //Subject
        beh_file << "Subject";
        beh_file << ",";
        //Trial
        beh_file << std::to_string(i + 1);
        beh_file << ",";
        //blank.ACC
        beh_file << info.blankAcc[i];
        beh_file << ",";
        //blank.CRESP
        if(info.code[i] == "2")
            beh_file << "1";
        beh_file << ",";
        //blank.OnsetTime
        if(info.code[i] == "2")
            beh_file << info.onSetTime[i];
        beh_file << ",";
        //blank.RESP
        if(info.blankResp[i] == 1)
            beh_file << info.blankResp[i];
        beh_file << ",";
        //blank.RT
        if(info.code[i] == "2" || info.blankResp[i] == 1)
            beh_file << info.blankRt[i];
        else
            beh_file << 0;
        beh_file << ",";
        //blank.RTTime
        if(info.code[i] == "2" || info.blankResp[i] == 1)
            beh_file << info.blankRtTime[i];
        else
            beh_file << 0;
        beh_file << ",";
        //code
        beh_file << info.code[i];
        beh_file << ",";
        //correct
        if(info.code[i] == "2")
            beh_file << "1";
        beh_file << ",";
        //pic
        if(info.code[i] == "2")
            beh_file << "2.bmp" << std::endl;
        else
            beh_file << "8.bmp" << std::endl;
    }
}
