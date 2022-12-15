#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <tuple>
#include <QChart>
#include <QColor>
#include <QValueAxis>
#include <QLineSeries>
#include <QList>
#include <QPointF>
#include <QVector>
#include <QGraphicsSimpleTextItem>

namespace eegneo
{
    namespace utils { class Filter; }

    enum class Second : std::int8_t
    {
        INVALID = 0,
        ONE = 1,
        FIVE = 5,
        TEN = 10
    };

    enum class Frequency : std::int8_t
    {
        INVALID = 0,
        FIFTY = 50,
        SIXTY = 60
    };

    class WavePlotter
    {
    public:
        WavePlotter();
        WavePlotter(WavePlotter&&) = default;
        WavePlotter& operator=(WavePlotter&&) = default;
        virtual ~WavePlotter();

        virtual void update() = 0;

        QChart* chart() { return &mChart_; }

        void setAxisXScale(qreal xMin, qreal xMax);
        void setAxisYScale(qreal yMin, qreal yMax);

    protected:
        qreal mXMin_, mXMax_;
        qreal mYMin_, mYMax_;
        QValueAxis mAxisX_;
        QValueAxis mAxisY_;
        QChart mChart_;
        

        void addOneLineSeries(QLineSeries* line);
    };

    class EEGWavePlotter : public WavePlotter
    {
    public:
        EEGWavePlotter(std::size_t channelNum, std::size_t sampleRate, std::size_t freshMs, double* buf);
        ~EEGWavePlotter();

        void setAxisXScale(Second sec);
        void addOneMarkerLine(const QString& eventStr);
        void update() override;

    private:
        std::size_t mSampleRate_;
        std::size_t mFreshRate_;
        qreal mMoveOffset_;
        QLineSeries* mLineSeries_;
        QVector<QList<QPointF>> mData_;
        double* mBuf_;
        std::vector<std::tuple<QLineSeries*, QGraphicsSimpleTextItem*, QList<QPointF>>> mMarkerLineTbl_;

        void setLineColor(QLineSeries* line);   
    };

    class FFTWavePlotter : public WavePlotter
    {
    public:
        FFTWavePlotter(std::size_t channelNum, std::size_t sampleRate);
        ~FFTWavePlotter();

        void setAxisXScale(Frequency freqMax);
        void update() override;

        std::vector<float>& real(std::size_t idx) { return mFFTBuf_[idx * 2]; }
        std::vector<float>& im(std::size_t idx) { return mFFTBuf_[2 * idx + 1]; }

    private:
        std::size_t mFFTSize_;
        std::size_t mSampleRate_;
        std::size_t mChannelNum_;
        std::vector<std::vector<float>> mFFTBuf_;
        std::vector<std::tuple<QLineSeries*, QList<QPointF>>> mLines_;
    };
}   // namespace eegneo
