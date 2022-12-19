#include "plotter.h"
#include <QDateTime>
#include "utils/config.h"
#include "utils/fft.h"
#include <iostream>

#define POW2(x) ((x) * (x))

namespace eegneo
{
    static void AddOneLineSeries(QLineSeries* line, QAbstractAxis* axisX, QAbstractAxis* axisY, QChart* chart)
    {
        chart->addSeries(line);                              // 将曲线对象添加到图表上
        line->attachAxis(axisX);                             // 曲线对象关联上X轴，此步骤必须在m_chart->addSeries之后
        line->attachAxis(axisY);                             // 曲线对象关联上Y轴，此步骤必须在m_chart->addSeries之后
        line->setUseOpenGL(true);
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

        mAxisX_.setTickCount(5);
        mAxisY_.setTickCount(static_cast<int>(channelNum));  
        
        mChart_->legend()->hide();
        mChart_->addAxis(&mAxisX_, Qt::AlignBottom);                    // 将X轴添加到图表上
        mChart_->addAxis(&mAxisY_, Qt::AlignLeft);                      // 将Y轴添加到图表上  

        for (int i = 0; i < mData_.size(); ++i)
        {
            AddOneLineSeries(&mLineSeries_[i], &mAxisX_, &mAxisY_, mChart_);
        }
    }

    EEGWavePlotter::~EEGWavePlotter()
    {
        // delete[] mLineSeries_;
    }

    void EEGWavePlotter::setAxisXScale(Second sec)
    {
        mAxisX_.setMin(0); mAxisX_.setMax(this->mSampleRate_ * static_cast<std::size_t>(sec));

        for (int c = 0; c < static_cast<int>(sec) + 1; ++c)
        {
            mAxisX_.remove(QString::number(c));
        }
        for (int c = 0; c < static_cast<int>(sec) + 1; ++c)
        {
            mAxisX_.append(QString::number(c), ((mAxisX_.max() - mAxisX_.min()) / static_cast<int>(sec)) * c);
        }

        mAxisX_.setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    }

    void EEGWavePlotter::setAxisYScale(int maxVoltage)
    {
        mAxisY_.setMin(-maxVoltage); mAxisY_.setMax(-maxVoltage + mData_.size() * maxVoltage);

        auto& config = eegneo::utils::ConfigLoader::instance();
        auto names = config.get<std::vector<std::string>>("Acquisition", "Electrodes");
        for (int c = 0; c < mData_.size(); ++c)
        {
            mAxisY_.remove(QString::fromStdString(names[c]));
        }
        for (int c = 0; c < mData_.size(); ++c)
        {
            mAxisY_.append(QString::fromStdString(names[c]), -maxVoltage + ((mAxisY_.max() - mAxisY_.min()) / mData_.size()) * (c + 0.5));
        }
        
        mAxisY_.setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    }

    void EEGWavePlotter::addOneMarkerLine(const QString& eventStr)
    {
        auto* line = new QLineSeries();
        this->setLineColor(line);
        AddOneLineSeries(line, &mAxisX_, &mAxisY_, mChart_);

        QList<QPointF> points;
        points.emplace_back(mData_[0].size() * mMoveOffset_, mAxisY_.max());
        points.emplace_back(mData_[0].size() * mMoveOffset_, mAxisY_.max() - (mAxisY_.max() / 5));

        auto* text = new QGraphicsSimpleTextItem(mChart_);
        text->setText(eventStr);

        mMarkerLineTbl_.emplace_back(std::make_tuple(line, text, points));
    }

    void EEGWavePlotter::update()
    {
        // 更新波形
        if(static_cast<std::size_t>(mData_[0].size()) >= (mFreshRate_ * static_cast<std::size_t>(mAxisX_.max() - mAxisX_.min()) / mSampleRate_))
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
                text->setPos(mChart_->mapToPosition(QPointF{newPosX, mAxisY_.max() + 10}, line));
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
        
        mChart_->legend()->hide();
        mChart_->addAxis(&mAxisX_, Qt::AlignBottom);                    // 将X轴添加到图表上
        mChart_->addAxis(&mAxisY_, Qt::AlignLeft);                      // 将Y轴添加到图表上  

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
            AddOneLineSeries(line, &mAxisX_, &mAxisY_, mChart_);
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
        mAxisX_.setMin(0); mAxisX_.setMax(static_cast<std::size_t>(freqMax));
    }

    void FFTWavePlotter::setAxisYScale(int maxVoltage)
    {
        mAxisY_.setMin(0); mAxisY_.setMax(maxVoltage);
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
            points.back().setY(mAxisY_.min() + (std::size_t)fftval % (std::size_t)(mAxisY_.max() - mAxisY_.min()));
            line->replace(points);
        }
    }

    TopographyPlotter::TopographyPlotter(QGraphicsView* view)
        : mView_(view)
        , mGraphicsScene_(new QGraphicsScene())
        , mGraphicsPixmapItem_(new QGraphicsPixmapItem(QPixmap(":/images/resource/Images/eeg_10_20.png")))
    {
        mGraphicsScene_->setSceneRect(500, 500, 190, 190);  
        mGraphicsPixmapItem_->setPos(mGraphicsScene_->width()/2, mGraphicsScene_->height()/2);  
        mGraphicsScene_->addItem(mGraphicsPixmapItem_);
        mView_->setScene(mGraphicsScene_);
    }

    TopographyPlotter::~TopographyPlotter()
    {
        delete mGraphicsScene_;
        delete mGraphicsPixmapItem_;
    }

    void TopographyPlotter::showEvent()
    {
        QRectF bounds = mGraphicsScene_->itemsBoundingRect();
        bounds.setWidth(bounds.width()*0.9);         
        bounds.setHeight(bounds.height()*0.9);
        mView_->fitInView(bounds, Qt::KeepAspectRatio);
        mView_->centerOn(mGraphicsPixmapItem_);
    }

    void TopographyPlotter::update()
    {
        // TODO
    }
}   // namespace eegneo
