#include "acquisition/plotter.h"
#include <QDateTime>
#include "utils/fft.h"
#include <iostream>

#define POW2(x) ((x) * (x))

namespace eegneo
{
    WavePlotter::WavePlotter()
    {
        this->setAxisXScale(0, 5);
        this->setAxisYScale(0, 0);
        
        mChart_.legend()->hide();
        mChart_.addAxis(&mAxisX_, Qt::AlignBottom);                    // 将X轴添加到图表上
        mChart_.addAxis(&mAxisY_, Qt::AlignLeft);                      // 将Y轴添加到图表上        
    }

    WavePlotter::~WavePlotter()
    {
        
    }

    void WavePlotter::addOneLineSeries(QLineSeries* line)
    {
        mChart_.addSeries(line);                              // 将曲线对象添加到图表上
        line->attachAxis(&mAxisX_);                             // 曲线对象关联上X轴，此步骤必须在m_chart->addSeries之后
        line->attachAxis(&mAxisY_);                             // 曲线对象关联上Y轴，此步骤必须在m_chart->addSeries之后
        line->setUseOpenGL(true);
    }

    void WavePlotter::setAxisXScale(qreal xMin, qreal xMax)
    {
        this->mXMin_ = xMin; this->mXMax_ = xMax;
        mAxisX_.setMin(mXMin_); mAxisX_.setMax(mXMax_);
    }

    void WavePlotter::setAxisYScale(qreal yMin, qreal yMax)
    {
        this->mYMin_ = yMin; this->mYMax_ = yMax;
        mAxisY_.setMin(mYMin_); mAxisY_.setMax(mYMax_);
    }

    EEGWavePlotter::EEGWavePlotter(std::size_t channelNum, std::size_t sampleRate, std::size_t freshMs, double* buf)
        : WavePlotter()
        , mSampleRate_(sampleRate)
        , mFreshRate_((1000.0 / freshMs))
        , mMoveOffset_((double)sampleRate / mFreshRate_)
        , mLineSeries_(new QLineSeries[channelNum])
        , mData_(channelNum)
        , mBuf_(buf)
    {
        mAxisX_.setTitleText("Time (s)");
        mAxisY_.setTitleText("Voltage (uV)");

        mAxisX_.setTickCount(5);
        mAxisY_.setTickCount(static_cast<int>(channelNum));

        for (int i = 0; i < mData_.size(); ++i)
        {
            this->addOneLineSeries(&mLineSeries_[i]);
        }
    }

    EEGWavePlotter::~EEGWavePlotter()
    {
        // delete[] mLineSeries_;
    }

    void EEGWavePlotter::setAxisXScale(Second sec)
    {
        WavePlotter::setAxisXScale(0, this->mSampleRate_ * static_cast<std::size_t>(sec));
    }

    void EEGWavePlotter::addOneMarkerLine(const QString& eventStr)
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

    void EEGWavePlotter::update()
    {
        // 更新波形
        if(static_cast<std::size_t>(mData_[0].size()) >= (mFreshRate_ * static_cast<std::size_t>(mXMax_ - mXMin_) / mSampleRate_))
        {
            for (int i = 0; i < mData_.size(); ++i)
            {
                mData_[i].pop_front();
                for (QPointF& point : mData_[i])
                {
                    point.setX(point.x() - mMoveOffset_);
                }
            }
        }

        for (int i = 0; i < mData_.size(); ++i)
        {
            mData_[i].emplace_back(mData_[0].size() * mMoveOffset_, mBuf_[i] + ((mAxisY_.max() - mAxisY_.min()) / mData_.size()) * i);
            mLineSeries_[i].replace(mData_[i]);
        }
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

    void EEGWavePlotter::setLineColor(QLineSeries* line)
    {
        QPen splinePen; 
        splinePen.setBrush(Qt::red);
        splinePen.setColor(Qt::red);
        line->setPen(splinePen);
    }

    FFTWavePlotter::FFTWavePlotter(std::size_t channelNum, std::size_t sampleRate)
        : WavePlotter()
        , mFFTSize_(audiofft::AudioFFT::ComplexSize(eegneo::utils::FFTCalculator::fftsize()))
        , mSampleRate_(sampleRate)
        , mChannelNum_(channelNum)
        , mFFTBuf_(channelNum * 2)
    {
        mAxisX_.setTitleText("Freqency (Hz)");
        mAxisY_.setTitleText("FFT magnitude (uV)");

        for (auto& buf : mFFTBuf_)
        {
            buf.resize(mFFTSize_);
        }

        for (std::size_t n = 0; n < mFFTSize_; ++n)
        {
            QList<QPointF> points; 
            double freq = n * mSampleRate_ / ((mFFTSize_ + 1) * 2);
            points.emplace_back(freq, 0.0);
            points.emplace_back(freq, 0.0);

            auto* line = new QLineSeries();
            mLines_.emplace_back(std::make_tuple(line, points));
            this->addOneLineSeries(line);
        }
    }

    FFTWavePlotter::~FFTWavePlotter()
    {
        for (auto& [line, _] : mLines_)
        {
            delete line;
        }
    }

    void FFTWavePlotter::setAxisXScale(Frequency freqMax)
    {
        WavePlotter::setAxisXScale(0, static_cast<std::size_t>(freqMax));
    }

    void FFTWavePlotter::update()
    {
        for (std::size_t n = 0; n < mFFTSize_; ++n)
        {
            float fftval = 0.0;
            for (std::size_t i = 0; i < mChannelNum_; ++i)
            {
                fftval += (POW2(real(i)[n]) + POW2(im(i)[n]));
            }
            fftval /= mChannelNum_;
            auto& [line, points] = mLines_[n];
            points.back().setY(mYMin_ + (std::size_t)fftval % (std::size_t)(mYMax_ - mYMin_));
            line->replace(points);
        }
    }
}   // namespace eegneo