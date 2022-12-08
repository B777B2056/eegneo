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
        mAxisX_.setTitleText("Time (s)");
        mAxisY_.setTitleText("Voltage (uV)");

        mAxisX_.setMin(0);
        this->setAxisYScale(0, 0);
        
        mChart_.legend()->hide();
        mChart_.addAxis(&mAxisX_, Qt::AlignBottom);                    // 将X轴添加到图表上
        mChart_.addAxis(&mAxisY_, Qt::AlignLeft);                      // 将Y轴添加到图表上

        mAxisX_.setTickCount(5);
        mAxisX_.setTickCount(5);

        for (int i = 0; i < channelNum; ++i)
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
        if(static_cast<std::size_t>(mData_[0].size()) >= (PointInChartNum * static_cast<std::size_t>(this->mXRange_)))
        {
            for (int i = 0; i < mChannelNum_; ++i)
            {
                mData_[i].pop_front();
                for (QPointF& point : mData_[i])
                {
                    point.setX(point.x() - 1);
                }
            }
        }
        for (int i = 0; i < mChannelNum_; ++i)
        {
            mData_[i].emplace_back(mData_[0].size(), data[i] + 10 * i);
            mLineSeries_[i].replace(mData_[i]);
        }
    }

    void EEGWavePlotImpl::setAxisXScale(Second sec)
    {
        this->mXRange_ = sec;
        mAxisX_.setMax(PointInChartNum * static_cast<std::size_t>(sec));
    }

    void EEGWavePlotImpl::setAxisYScale(qreal yMin, qreal yMax)
    {
        this->mYMin_ = yMin;
        this->mYMax_ = yMax;
        mAxisY_.setMin(mYMin_);
        mAxisY_.setMax(mYMax_);
    }
}   // namespace eegneo
