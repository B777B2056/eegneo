#include "acquisition/wave_plotter.h"
#include <iterator>
#include <QDateTime>
#include "acquisition/filter.h"

namespace eegneo
{
    WavePlotImpl::WavePlotImpl(std::size_t n, std::size_t sampleRate, std::size_t freshMs)
        : mLineSeriesNum_(n)
        , mSampleRate_(sampleRate)
        , mFreshRate_((1000.0 / freshMs))
        , mMoveOffset_((double)sampleRate / mFreshRate_)
        , mXRange_(Second::INVALID)
        , mLineSeries_(new QLineSeries[n])
        , mData_(n)
    {
        mAxisX_.setTitleText("Time (s)");
        mAxisY_.setTitleText("Voltage (uV)");

        mAxisX_.setMin(0);
        this->setAxisYScale(0, 0);
        
        mChart_.legend()->hide();
        mChart_.addAxis(&mAxisX_, Qt::AlignBottom);                    // 将X轴添加到图表上
        mChart_.addAxis(&mAxisY_, Qt::AlignLeft);                      // 将Y轴添加到图表上

        mAxisX_.setTickCount(5);
        mAxisY_.setTickCount(static_cast<int>(n));

        for (std::size_t i = 0; i < n; ++i)
        {
            this->addOneLineSeries(&mLineSeries_[i]);
        }
    }

    WavePlotImpl::~WavePlotImpl()
    {
        delete[] mLineSeries_;
    }

    void WavePlotImpl::addOneLineSeries(QLineSeries* line)
    {
        mChart_.addSeries(line);                              // 将曲线对象添加到图表上
        line->attachAxis(&mAxisX_);                             // 曲线对象关联上X轴，此步骤必须在m_chart->addSeries之后
        line->attachAxis(&mAxisY_);                             // 曲线对象关联上Y轴，此步骤必须在m_chart->addSeries之后
        line->setUseOpenGL(true);
    }

    void WavePlotImpl::update(double* data)
    {
        if(static_cast<std::size_t>(mData_[0].size()) >= (mFreshRate_ * static_cast<std::size_t>(this->mXRange_)))
        {
            for (int i = 0; i < mLineSeriesNum_; ++i)
            {
                mData_[i].pop_front();
                for (QPointF& point : mData_[i])
                {
                    point.setX(point.x() - mMoveOffset_);
                }
            }
        }
        for (int i = 0; i < mLineSeriesNum_; ++i)
        {
            mData_[i].emplace_back(mData_[0].size() * mMoveOffset_, data[i] + ((mAxisY_.max() - mAxisY_.min()) / mLineSeriesNum_) * i);
            mLineSeries_[i].replace(mData_[i]);
        }
    }

    void WavePlotImpl::setAxisXScale(Second sec)
    {
        this->mXRange_ = sec;
        mAxisX_.setMax(this->mSampleRate_ * static_cast<std::size_t>(sec));
    }

    void WavePlotImpl::setAxisYScale(qreal yMin, qreal yMax)
    {
        this->mYMin_ = yMin;
        this->mYMax_ = yMax;
        mAxisY_.setMin(mYMin_);
        mAxisY_.setMax(mYMax_);
    }

    EEGWavePlotImpl::EEGWavePlotImpl(std::size_t channelNum, std::size_t sampleRate, std::size_t freshMs)
        : WavePlotImpl(channelNum, sampleRate, freshMs)
        , mFilter_(new utils::Filter(sampleRate))
        , mOriginalSignals_(channelNum)
        , mFiltResults_(channelNum)
    {
        for (std::size_t i = 0; i < mLineSeriesNum_; ++i)
        {
            mFiltResults_[i].resize(utils::Filter::numTaps());
        }
    }

    EEGWavePlotImpl::~EEGWavePlotImpl()
    {
        delete mFilter_;
    }

    void EEGWavePlotImpl::addOneMarkerLine(const QString& eventStr)
    {
        auto* line = new QLineSeries();
        this->setLineColor(line);
        this->addOneLineSeries(line);

        QList<QPointF> points;
        points.emplace_back(mData_[0].size() * mMoveOffset_, mAxisY_.max());
        points.emplace_back(mData_[0].size() * mMoveOffset_, mAxisY_.max() - (mAxisY_.max() / 5));

        auto* text = new QGraphicsSimpleTextItem(&mChart_);
        text->setText(eventStr);

        mMarkerLineTbl_.emplace_back(std::make_tuple(line, text, points));
    }

    void EEGWavePlotImpl::update(double* data, bool isFilt, double lowCutoff, double highCutoff, double notchCutoff)
    {
        if(mOriginalSignals_[0].size() >= utils::Filter::numTaps())
        {
            for (std::size_t i = 0; i < mLineSeriesNum_; ++i)
            {
                mOriginalSignals_[i].erase(mOriginalSignals_[i].begin());
                mOriginalSignals_[i].push_back(data[i]);
            }
        }
        else
        {
            for (std::size_t i = 0; i < mLineSeriesNum_; ++i)
            {
                mOriginalSignals_[i].push_back(data[i]);
            }
        }
        if (isFilt)
        {
            this->filt(data, lowCutoff, highCutoff, notchCutoff);
        }
        this->update(data);
    }

    void EEGWavePlotImpl::update(double* data)
    {
        // 更新波形
        WavePlotImpl::update(data);
        // 更新Marker
        for (auto iter = mMarkerLineTbl_.begin(); iter != mMarkerLineTbl_.end(); )
        {
            auto& [line, text, points] = *iter;

            QPointF& topPoint = points[0];
            QPointF& bottomPoint = points[1];

            if (!topPoint.x())
            {
                // Marker超出绘图范围，销毁对象
                delete line;
                delete text;
                iter = mMarkerLineTbl_.erase(iter);
            }
            else
            {
                // 更新Marker位置
                auto newPosX = topPoint.x() - mMoveOffset_;
                topPoint.setX(newPosX);
                bottomPoint.setX(newPosX);
                text->setPos(mChart_.mapToPosition(QPointF{newPosX, mAxisY_.max() + 10}, line));
                line->replace(points);

                ++iter;
            }
        }
    }

    void EEGWavePlotImpl::setLineColor(QLineSeries* line)
    {
        QPen splinePen; 
        splinePen.setBrush(Qt::red);
        splinePen.setColor(Qt::red);
        line->setPen(splinePen);
    }

    void EEGWavePlotImpl::filt(double* data, double lowCutoff, double highCutoff, double notchCutoff)
    {
        for (std::size_t i = 0; i < mLineSeriesNum_; ++i)
        {
            auto& signal = mOriginalSignals_[i];
            auto& result = mFiltResults_[i];
            if ((lowCutoff >= 0.0) && (highCutoff < 0.0))   // 高通滤波
            {
                mFilter_->lowPass(lowCutoff, signal, result);
            }
            else if ((lowCutoff < 0.0) && (highCutoff >= 0.0))  // 低通滤波
            {
                mFilter_->highPass(highCutoff, signal, result);
            }
            else if ((lowCutoff >= 0.0) && (highCutoff >= 0.0)) // 带通滤波
            {
                mFilter_->bandPass(lowCutoff, highCutoff, signal, result);
            }
            else    // 无滤波
            {

            }
            if (notchCutoff > 0.0)  // 陷波滤波
            {
                mFilter_->notch(notchCutoff, signal, result);
            }
            data[i] = result.back();
        }
    }
}   // namespace eegneo
