#include "backend.h"
#include <chrono>
#include "sampler.h"
#include "eegtopo.h"
#include "filter.h"
#include "fft.h"
#include "file.h"
#include "utils/config.h"
#include "utils/ipc.h"

#define InitIpcServer(CMDType)   \
    do  \
    {   \
        mIpcWrapper_->setCmdHandler<eegneo::## CMDType## Cmd>([this](eegneo::## CMDType## Cmd* cmd)->void   \
        {   \
            this->handle##CMDType##Cmd(cmd);  \
        });  \
    }   \
    while (0)

#define SUB_THREAD_TASK_NUM 3

namespace eegneo
{
    AcquisitionBackend::AcquisitionBackend(std::size_t channelNum, std::size_t sampleFreqHz)
        : mIsStop_(false), mIsOnceSampleDone_(false), mThreadPool_(SUB_THREAD_TASK_NUM)
        , mIpcWrapper_(nullptr)
        , mDataSampler_(new TestDataSampler(channelNum))
        , mChannelNum_(channelNum)
        , mSharedMemory_("Sampler")
        , mFilter_(new Filter[channelNum]), mFiltBuf_(new double[channelNum])
        , mFFT_(new FFTCalculator[channelNum])
        , mTopoPlot_(new TopoPlot(static_cast<double>(sampleFreqHz), channelNum))
    {
        this->initSharedMemory();
        this->initIpc();
        this->initTaskThreads();

        // if (const auto& pyerrmsg = this->mTopoPlot_->error(); !pyerrmsg.empty())
        // {
        //     ErrorCmd cmd;
        //     ::memcpy(cmd.errmsg, pyerrmsg.c_str(), pyerrmsg.size());
        //     this->mIpcWrapper_->sendCmd(cmd);
        // }
    }

    AcquisitionBackend::~AcquisitionBackend()
    {
        this->mIsStop_.store(true);
        this->mIsOnceSampleDone_.store(false);
        delete mIpcWrapper_;
        delete[] mFiltBuf_;
        delete[] mFilter_;
        delete[] mFFT_;
        delete mTopoPlot_;
        delete mDataSampler_;
    }

    void AcquisitionBackend::initIpc()
    {
        auto& config = eegneo::utils::ConfigLoader::instance();
        auto port = config.get<std::uint16_t>("IpcServerIpPort");
        mIpcWrapper_ = new eegneo::utils::IpcClient(SessionId::AccquisitionInnerSession, "127.0.0.1", port);
        InitIpcServer(Record); InitIpcServer(Filt); InitIpcServer(Shutdown); 
        InitIpcServer(Marker); InitIpcServer(FileSave);
    }

    void AcquisitionBackend::initSharedMemory()
    {
        if (mSharedMemory_.attach())
        {
            mSharedMemory_.detach();
        }
        if (!mSharedMemory_.create(10240))
        {
            throw "Shared memory create failed!";
        }
    }

    void AcquisitionBackend::initTaskThreads()
    {
        this->mThreadPool_.submitTask([this]()->void
        {
            while (!this->mIsStop_.load())
            {
                this->mIsOnceSampleDone_.store(false);
                this->doSample(); 
                this->mIsOnceSampleDone_.store(true);
            }
        });
        this->mThreadPool_.submitTask([this]()->void
        {
            while (!this->mIsStop_.load())
            {
                if (this->mIsOnceSampleDone_.load())
                {
                    this->doFilt();
                    this->doFFT();
                    // 降低cpu使用率
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(50ms);
                }
            }
        });
    }

    void AcquisitionBackend::doTaskInMainThread()
    {
        if (this->mIsOnceSampleDone_.load())
        {
            this->doTopoPlot();
        }
    }

    void AcquisitionBackend::handleRecordCmd(RecordCmd* cmd)
    {
        if (cmd->isRecordOn)
        {
            mDataSampler_->setRecordingFlag();
        }
        else
        {
            mDataSampler_->clearRecordingFlag();
        }
    }

    void AcquisitionBackend::handleFiltCmd(FiltCmd* cmd)
    {
        this->mFiltCmd_ = *cmd;  // 设置滤波参数
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mFilter_[i].setSampleFreqHz(cmd->sampleRate);
        }
    }

    void AcquisitionBackend::handleShutdownCmd(ShutdownCmd* cmd)
    {
        // 停止采集
    }

    void AcquisitionBackend::handleMarkerCmd(MarkerCmd* cmd)
    {
        mDataSampler_->doRecordEvent(cmd->msg);
    }

    void AcquisitionBackend::handleFileSaveCmd(FileSaveCmd* cmd)
    {
        auto& config = eegneo::utils::ConfigLoader::instance();
        auto names = config.get<std::vector<std::string>>("Acquisition", "Electrodes");

        eegneo::EDFWritter writter{cmd->filePath, cmd->channelNum, cmd->fileType};
        writter.setSampleFreqencyHz(cmd->sampleRate);

        for (std::size_t i = 0; i < cmd->channelNum; ++i)
        {
            writter.setChannelName(i, names[i].c_str());
        }

        writter.saveRecordData();
        writter.saveAnnotation();
        this->mIpcWrapper_->sendCmd(FileSavedFinishedCmd{});
    }

    void AcquisitionBackend::doSample()
    {
        mDataSampler_->sampleOnce();
        mDataSampler_->doRecordData();
        if (!mFiltCmd_.isFiltOn)
        {   
            if (!mSharedMemory_.lock()) return;
            ::memcpy(mSharedMemory_.data(), mDataSampler_->data(), (this->mChannelNum_ * sizeof(double)));
            mSharedMemory_.unlock();
        }
    }

    void AcquisitionBackend::doFilt()
    {
        if (!mFiltCmd_.isFiltOn) return;
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            if ((mFiltCmd_.lowCutoff >= 0.0) && (mFiltCmd_.highCutoff < 0.0))   // 高通滤波
            {
                mFiltBuf_[i] = mFilter_[i].lowPass(mDataSampler_->data()[i], mFiltCmd_.lowCutoff);
            }
            else if ((mFiltCmd_.lowCutoff < 0.0) && (mFiltCmd_.highCutoff >= 0.0))  // 低通滤波
            {
                mFiltBuf_[i] = mFilter_[i].highPass(mDataSampler_->data()[i], mFiltCmd_.highCutoff);
            }
            else if ((mFiltCmd_.lowCutoff >= 0.0) && (mFiltCmd_.highCutoff >= 0.0)) // 带通滤波
            {
                mFiltBuf_[i] = mFilter_[i].bandPass(mDataSampler_->data()[i], mFiltCmd_.lowCutoff, mFiltCmd_.highCutoff);
            }
            else    // 无滤波
            {
                mFiltBuf_[i] = mDataSampler_->data()[i];
            }
            if (mFiltCmd_.notchCutoff > 0.0)  // 陷波滤波（在上述滤波的基础上滤波）
            {
                mFiltBuf_[i] = mFilter_[i].notch(mFiltBuf_[i], mFiltCmd_.notchCutoff);
            }
        }

        if (!mSharedMemory_.lock()) return;
        ::memcpy(mSharedMemory_.data(), mFiltBuf_, (this->mChannelNum_ * sizeof(double)));
        mSharedMemory_.unlock();
    }

    void AcquisitionBackend::doFFT()
    {
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mFFT_[i].appendSignalData(mDataSampler_->data()[i]);
            mFFT_[i].doFFT();
        }

        void* pos = (void*)((char*)mSharedMemory_.data() + (this->mChannelNum_ * sizeof(double)));

        if (!mSharedMemory_.lock()) return;

        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            auto& real = mFFT_[i].real();
            auto& im = mFFT_[i].im();
            ::memcpy(pos, real.data(), real.size());
            pos = (void*)((char*)pos + real.size());
            ::memcpy(pos, im.data(), im.size());
            pos = (void*)((char*)pos + im.size());
        }

        mSharedMemory_.unlock();
    }

    void AcquisitionBackend::doTopoPlot()
    {
        mTopoPlot_->appendNewData(mDataSampler_->data());
        mTopoPlot_->plotAndSaveFigure();
    }
}   // namespace eegneo
