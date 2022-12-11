#include "backend.h"
#include "sampler.h"
#include "utils/filter.h"

namespace eegneo
{
    constexpr const char* DATA_FILE_PATH = "E:/jr/eegneo/temp_data.txt";
    constexpr const char* EVENT_FILE_PATH = "E:/jr/eegneo/temp_event.txt";

    AcquisitionBackend::AcquisitionBackend(std::size_t channelNum)
        : mDataSampler_(new TestDataSampler(channelNum))
        , mChannelNum_(channelNum), mCurDataN_(0)
        , mDataFile_{DATA_FILE_PATH, std::ios::out}, mEventFile_{EVENT_FILE_PATH, std::ios::out}
        , mSharedMemory_{"Sampler"}, mIsStop_(false)
        , mFilter_(new utils::Filter[channelNum]), mFiltBuf_(new double[channelNum])
    {
        if (!mDataFile_.is_open())
        {
            // TODO: 文件创建失败，处理
        }
        if (mSharedMemory_.attach())
        {
            mSharedMemory_.detach();
        }
        if (!mSharedMemory_.create(mChannelNum_ * sizeof(double)))
        {
            throw "Shared memory create failed!";
        }
    }

    AcquisitionBackend::~AcquisitionBackend()
    {
        delete[] mFiltBuf_;
        delete[] mFilter_;
        delete mDataSampler_;
    }

    void AcquisitionBackend::start()
    {
        while (!mIsStop_)
        {
            std::unique_lock<std::mutex> lock(mMutex_);
            this->doSample(); 
            if (mRecCmd_.isRecordOn) this->doRecord();
            if (mFiltCmd_.isFiltOn) this->doFilt();

            if (!mSharedMemory_.lock()) continue;
            if (mFiltCmd_.isFiltOn)
            {
                ::memcpy(mSharedMemory_.data(), mFiltBuf_, mChannelNum_ * sizeof(double));
            }
            else
            {
                ::memcpy(mSharedMemory_.data(), mDataSampler_->data(), mChannelNum_ * sizeof(double));
            }
            if (!mSharedMemory_.unlock()) continue;
        }
    }

    void AcquisitionBackend::doSample()
    {
        mDataSampler_->sampleOnce();
    }

    void AcquisitionBackend::doRecord()
    {
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mDataFile_ << mDataSampler_->data()[i] << " ";
        }
        mDataFile_ << "\n";
        ++mCurDataN_;
    }

    void AcquisitionBackend::doFilt()
    {
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            double afterFiltData = -1.0;
            mFilter_[i].appendData(mDataSampler_->data()[i]);
            if ((mFiltCmd_.lowCutoff >= 0.0) && (mFiltCmd_.highCutoff < 0.0))   // 高通滤波
            {
                afterFiltData = mFilter_[i].lowPass(mFiltCmd_.lowCutoff);
            }
            else if ((mFiltCmd_.lowCutoff < 0.0) && (mFiltCmd_.highCutoff >= 0.0))  // 低通滤波
            {
                afterFiltData = mFilter_[i].highPass(mFiltCmd_.highCutoff);
            }
            else if ((mFiltCmd_.lowCutoff >= 0.0) && (mFiltCmd_.highCutoff >= 0.0)) // 带通滤波
            {
                afterFiltData = mFilter_[i].bandPass(mFiltCmd_.lowCutoff, mFiltCmd_.highCutoff);
            }
            else    // 无滤波
            {
                afterFiltData = mDataSampler_->data()[i];
            }
            if (mFiltCmd_.notchCutoff > 0.0)  // 陷波滤波
            {
                afterFiltData = mFilter_[i].notch(mFiltCmd_.notchCutoff);
            }
            mFiltBuf_[i] = afterFiltData;
        }
    }

    void AcquisitionBackend::handleRecordCmd(RecordCmd* cmd)
    {
        std::unique_lock<std::mutex> lock(this->mMutex_);
        this->mRecCmd_ = *cmd;   // 设置记录参数
    }

    void AcquisitionBackend::handleFiltCmd(FiltCmd* cmd)
    {
        std::unique_lock<std::mutex> lock(this->mMutex_);
        this->mFiltCmd_ = *cmd;  // 设置滤波参数
        for (std::size_t i = 0; i < mChannelNum_; ++i)
        {
            mFilter_[i].setSampleFreqHz(cmd->sampleRate);
        }
    }

    void AcquisitionBackend::handleShutdownCmd(ShutdownCmd* cmd)
    {
        this->mIsStop_ = true;  // 停止采集
    }

    void AcquisitionBackend::handleMarkerCmd(MarkerCmd* cmd)
    {
        std::unique_lock<std::mutex> lock(this->mMutex_);
        this->mEventFile_ << this->mCurDataN_ << ":" << cmd->msg << std::endl;
    }
}   // namespace eegneo
