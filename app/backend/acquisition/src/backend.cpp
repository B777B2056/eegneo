#include "backend.h"
#include "sampler.h"
#include "utils/filter.h"
#include "utils/fft.h"

#define BUF_BYTES_LEN (this->mChannelNum_ * sizeof(double))

static std::fstream debug_file{"E:/jr/eegneo/debug.txt", std::ios::out};

namespace eegneo
{
    constexpr const char* DATA_FILE_PATH = "E:/jr/eegneo/temp_data.txt";
    constexpr const char* EVENT_FILE_PATH = "E:/jr/eegneo/temp_event.txt";

    AcquisitionBackend::AcquisitionBackend(std::size_t channelNum)
        : mDataSampler_(new TestDataSampler(channelNum))
        , mChannelNum_(channelNum), mCurDataN_(0)
        , mDataFile_{DATA_FILE_PATH, std::ios::out}, mEventFile_{EVENT_FILE_PATH, std::ios::out}
        , mSharedMemory_{"Sampler"}
        , mFilter_(new utils::Filter[channelNum]), mFiltBuf_(new double[channelNum])
        , mFFT_(new utils::FFTCalculator[channelNum])
    {
        if (!mDataFile_.is_open())
        {
            // TODO: 文件创建失败，处理
        }
        if (mSharedMemory_.attach())
        {
            mSharedMemory_.detach();
        }
        if (!mSharedMemory_.create(10240))
        {
            throw "Shared memory create failed!";
        }
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
        this->doRecord(); 
        this->doFilt();
        this->doFFT(); 
    }

    void AcquisitionBackend::handleRecordCmd(RecordCmd* cmd)
    {
        this->mRecCmd_ = *cmd;   // 设置记录参数
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
        this->mEventFile_ << this->mCurDataN_ << ":" << cmd->msg << std::endl;
    }

    void AcquisitionBackend::doSample()
    {
        mDataSampler_->sampleOnce();
        if (!mFiltCmd_.isFiltOn)
        {   
            if (!mSharedMemory_.lock()) return;
            ::memcpy(mSharedMemory_.data(), mDataSampler_->data(), BUF_BYTES_LEN);
            mSharedMemory_.unlock();
        }
    }

    void AcquisitionBackend::doRecord()
    {
        if (!mRecCmd_.isRecordOn) return;
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mDataFile_ << mDataSampler_->data()[i] << " ";
        }
        mDataFile_ << "\n";
        ++mCurDataN_;
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
