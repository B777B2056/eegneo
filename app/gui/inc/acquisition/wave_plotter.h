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

    class WavePlotImpl
    {
    public:
        WavePlotImpl() = default;
        WavePlotImpl(std::size_t n, std::size_t sampleRate, std::size_t freshMs);
        WavePlotImpl(WavePlotImpl&&) = default;
        WavePlotImpl& operator=(WavePlotImpl&&) = default;
        virtual ~WavePlotImpl();

        virtual void update(double* data);

        QChart* chart() { return &mChart_; }

        void setAxisXScale(Second sec);
        void setAxisYScale(qreal yMin, qreal yMax);

    protected:
        std::size_t mSampleRate_;
        std::size_t mFreshRate_;
        qreal mMoveOffset_;
        Second mXRange_;
        qreal mYMin_, mYMax_;
        QValueAxis mAxisX_;
        QValueAxis mAxisY_;
        QLineSeries* mLineSeries_;
        QChart mChart_;
        QVector<QList<QPointF>> mData_;

        void addOneLineSeries(QLineSeries* line);
    };

    class EEGWavePlotImpl : public WavePlotImpl
    {
    public:
        EEGWavePlotImpl() = default;
        EEGWavePlotImpl(std::size_t channelNum, std::size_t sampleRate, std::size_t freshMs);
        ~EEGWavePlotImpl();

        void addOneMarkerLine(const QString& eventStr);
        void update(double* data) override;

    private:
        std::vector<std::tuple<QLineSeries*, QGraphicsSimpleTextItem*, QList<QPointF>>> mMarkerLineTbl_;

        void setLineColor(QLineSeries* line);   
    };
}   // namespace eegneo
