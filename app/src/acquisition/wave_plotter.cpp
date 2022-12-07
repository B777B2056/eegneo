#include "acquisition/wave_plotter.h"
#include <QDateTime>

namespace eegneo
{
    constexpr static std::size_t PointInChartNum = 50;

    EEGWavePlotImpl::EEGWavePlotImpl(std::size_t channelNum)
        : mChannelNum_(channelNum)
        , mXRange_(Second::INVALID)
        , mLineSeries_(new QLineSeries[channelNum])
        , mData_(channelNum)
    {
        // mAxisX_.setTitleText("X");
        mAxisY_.setTitleText("Voltage (uV)");

        this->setAxisXScale(mXRange_);
        this->setAxisYScale(0, 0);
        
        mChart_.addAxis(&mAxisX_, Qt::AlignBottom);                    // 将X轴添加到图表上
        mChart_.addAxis(&mAxisY_, Qt::AlignLeft);                      // 将Y轴添加到图表上

        mAxisX_.setFormat("hh:mm:ss");
        mAxisX_.setTickCount(5);

        for (int i = 0; i < 8; ++i)
        {
            mChart_.addSeries(&mLineSeries_[i]);                              // 将曲线对象添加到图表上
            mLineSeries_[i].attachAxis(&mAxisX_);                             // 曲线对象关联上X轴，此步骤必须在m_chart->addSeries之后
            mLineSeries_[i].attachAxis(&mAxisY_);                             // 曲线对象关联上Y轴，此步骤必须在m_chart->addSeries之后
            mLineSeries_[i].setUseOpenGL(true);
        }
    }

    EEGWavePlotImpl::~EEGWavePlotImpl()
    {
        delete[] mLineSeries_;
    }

    void EEGWavePlotImpl::update(double* data)
    {
        if(mData_.size() >= PointInChartNum)
        {
            for (int i = 0; i < mChannelNum_; ++i)
            {
                mData_[i].pop_front();
            }
        }
        auto currentTime{QDateTime::currentDateTime()};
        mAxisX_.setRange(currentTime.addSecs(-static_cast<int>(mXRange_)), currentTime);
        for (int i = 0; i < mChannelNum_; ++i)
        {
            mData_[i].emplace_back(currentTime.toMSecsSinceEpoch(), data[i] + 10 * i);
            mLineSeries_[i].replace(mData_[i]);
        }
    }

    void EEGWavePlotImpl::setAxisXScale(Second sec)
    {
        this->mXRange_ = sec;
    }

    void EEGWavePlotImpl::setAxisYScale(qreal yMin, qreal yMax)
    {
        this->mYMin_ = yMin;
        this->mYMax_ = yMax;
        mAxisY_.setMin(mYMin_);
        mAxisY_.setMax(mYMax_);
    }
}   // namespace eegneo
