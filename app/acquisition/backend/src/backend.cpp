#include "backend.h"
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
        : mSmphSignalA_(0), mSmphSignalB_(0) 
        , mIsStop_(false), mThreadPool_(SUB_THREAD_TASK_NUM)
        , mIpcWrapper_(nullptr)
        , mDataSampler_(new UsbDataSampler(channelNum))
        , mChannelNum_(channelNum)
        , mSharedMemory_("Sampler")
        , mFilter_(channelNum), mFiltBuf_(channelNum)
        , mFFT_(channelNum), mFFTBuf_(mChannelNum_*(mFFT_[0].real().size()+mFFT_[0].im().size()))
        , mTopoPlot_(new TopoPlot(static_cast<double>(sampleFreqHz), channelNum))
    {
        this->initSharedMemory();
        this->initIpc();
        this->initTaskThreads();
    }

    AcquisitionBackend::~AcquisitionBackend()
    {
        this->mIsStop_.store(true);
        delete mIpcWrapper_;
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
        if (mSharedMemory_.isAttached())
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
        this->mThreadPool_.submitTask([this]()->void { this->doTaskInSampleThread(); });
        this->mThreadPool_.submitTask([this]()->void { this->doTaskInCalculateThread(); });
    }

    void AcquisitionBackend::doTaskInMainThread()
    {
        this->mSmphSignalB_.acquire();
        this->doTopoPlot();

        if (const auto& pyerrmsg = this->mTopoPlot_->error(); !pyerrmsg.empty())
        {
            this->handleError(pyerrmsg.c_str(), pyerrmsg.size());
        }
        else
        {
            this->mIpcWrapper_->sendCmd(TopoReadyCmd{});
        }
    }

    void AcquisitionBackend::doTaskInSampleThread()
    {
        while (!this->mIsStop_.load())
        {
            this->doSample(); 
            this->mSmphSignalA_.release();
            this->mSmphSignalB_.release();
        }
    }

    void AcquisitionBackend::doTaskInCalculateThread()
    {
        while (!this->mIsStop_.load())
        {
            this->mSmphSignalA_.acquire();
            this->doFilt();
            this->doFFT();
            this->transferData();
        }
    }

    void AcquisitionBackend::handleError(const char* msg, std::size_t len)
    {
        ErrorCmd cmd;
        ::memcpy(cmd.errmsg, msg, len);
        this->mIpcWrapper_->sendCmd(cmd);
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

            if (mFiltCmd_.type() & FiltType_LowPass)
            {
                mFilter_[i].setupLowPassFilter(mFiltCmd_.highCutoff);
            }
            if (mFiltCmd_.type() & FiltType_HighPass)
            {
                mFilter_[i].setupHighPassFilter(mFiltCmd_.lowCutoff);
            }
            if (mFiltCmd_.type() & FiltType_BandPass)
            {
                mFilter_[i].setupBandPassFilter(mFiltCmd_.lowCutoff, mFiltCmd_.highCutoff);
            }
            if (mFiltCmd_.type() & FiltType_Notch)
            {
                mFilter_[i].setupNotchFilter(mFiltCmd_.notchCutoff);
            }
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
    }

    void AcquisitionBackend::doFilt()
    {
        if (!mFiltCmd_.isFiltOn) return;
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            if (mFiltCmd_.type() & FiltType_LowPass)
            {
                mFiltBuf_[i] = mFilter_[i].lowPass(mDataSampler_->data()[i]);
            }
            if (mFiltCmd_.type() & FiltType_HighPass)
            {
                mFiltBuf_[i] = mFilter_[i].highPass(mDataSampler_->data()[i]);
            }
            if (mFiltCmd_.type() & FiltType_BandPass)
            {
                mFiltBuf_[i] = mFilter_[i].bandPass(mDataSampler_->data()[i]);
            }
            if (mFiltCmd_.type() & FiltType_Notch)
            {
                mFiltBuf_[i] = mFilter_[i].notch(mFiltBuf_[i]);
            }
        }
    }

    void AcquisitionBackend::doFFT()
    {
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mFFT_[i].appendSignalData(mDataSampler_->data()[i]);
            mFFT_[i].doFFT();

            auto& real = mFFT_[i].real();
            auto& im = mFFT_[i].im();
            char* pos = reinterpret_cast<char*>(this->mFFTBuf_.data() + i * (real.size()+im.size()));
            ::memcpy(pos, real.data(), real.size()*sizeof(float));
            ::memcpy(pos + real.size()*sizeof(float), im.data(), im.size()*sizeof(float));
        }
    }

    void AcquisitionBackend::doTopoPlot()
    {
        mTopoPlot_->appendNewData(mDataSampler_->data());
        mTopoPlot_->plotAndSaveFigure();
    }

    void AcquisitionBackend::transferData()
    {
        if (!mSharedMemory_.lock()) return;

        ::memcpy(mSharedMemory_.data(), 
                 mFiltCmd_.isFiltOn ? mFiltBuf_.data() : mDataSampler_->data(), 
                 mFiltBuf_.size()*sizeof(double));

        void* pos = (void*)((char*)mSharedMemory_.data() + mFiltBuf_.size()*sizeof(double));
        ::memcpy(pos, this->mFFTBuf_.data(), this->mFFTBuf_.size()*sizeof(float));

        mSharedMemory_.unlock();
    }
}   // namespace eegneo
