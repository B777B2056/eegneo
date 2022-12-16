#include "backend.h"
#include "sampler.h"
#include "utils/filter.h"
#include "utils/fft.h"
#include "utils/file.h"

#define BUF_BYTES_LEN (this->mChannelNum_ * sizeof(double))

#define InitIpcServer(CMDType)   \
    do  \
    {   \
        mIpcWrapper_.setCmdHandler<eegneo::## CMDType## Cmd>([this](eegneo::## CMDType## Cmd* cmd)->void   \
        {   \
            this->handle##CMDType##Cmd(cmd);  \
        });  \
    }   \
    while (0)

// static std::fstream debug_file{"E:/jr/eegneo/debug.txt", std::ios::out};

namespace eegneo
{
    AcquisitionBackend::AcquisitionBackend(std::size_t channelNum)
        : mDataSampler_(new TestDataSampler(channelNum))
        , mChannelNum_(channelNum)
        , mSharedMemory_{"Sampler"}
        , mFilter_(new utils::Filter[channelNum]), mFiltBuf_(new double[channelNum])
        , mFFT_(new utils::FFTCalculator[channelNum])
    {
        if (mSharedMemory_.attach())
        {
            mSharedMemory_.detach();
        }
        if (!mSharedMemory_.create(10240))
        {
            throw "Shared memory create failed!";
        }

        InitIpcServer(Record);
        InitIpcServer(Filt);
        InitIpcServer(Shutdown);
        InitIpcServer(Marker);
        InitIpcServer(FileSave);

        // 连接主进程
        if (!mIpcWrapper_.start())
        {

        }
        mIpcWrapper_.sendIdentifyInfo(SessionId::AccquisitionInnerSession);
    }

    AcquisitionBackend::~AcquisitionBackend()
    {
        delete[] mFiltBuf_;
        delete[] mFilter_;
        delete[] mFFT_;
        delete mDataSampler_;
    }

    void AcquisitionBackend::run()
    {
        this->doSample(); 
        this->doFilt();
        this->doFFT(); 
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
        eegneo::utils::EDFWritter writter{cmd->filePath, cmd->channelNum, cmd->fileType};
        writter.setSampleFreqencyHz(cmd->sampleRate);
        writter.saveRecordData();
        writter.saveAnnotation();
        this->mIpcWrapper_.sendCmd(SessionId::AccquisitionInnerSession, FileSavedFinishedCmd{});
    }

    void AcquisitionBackend::doSample()
    {
        if (!mFiltCmd_.isFiltOn)
        {   
            if (!mSharedMemory_.lock()) return;
            ::memcpy(mSharedMemory_.data(), mDataSampler_->data(), BUF_BYTES_LEN);
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
        ::memcpy(mSharedMemory_.data(), mFiltBuf_, BUF_BYTES_LEN);
        mSharedMemory_.unlock();
    }

    void AcquisitionBackend::doFFT()
    {
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mFFT_[i].appendSignalData(mDataSampler_->data()[i]);
            mFFT_[i].doFFT();
        }

        void* pos = (void*)((char*)mSharedMemory_.data() + BUF_BYTES_LEN);

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
}   // namespace eegneo
