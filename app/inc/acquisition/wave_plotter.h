#pragma once
#include <cstddef>
#include <cstdint>
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>
#include <QList>
#include <QPointF>
#include <QVector>

namespace eegneo
{
    enum class Second : std::int8_t
    {
        INVALID = 0,
        ONE = 1,
        FIVE = 5,
        TEN = 10
    };

    class EEGWavePlotImpl
    {
    public:
        EEGWavePlotImpl(std::size_t channelNum);
        ~EEGWavePlotImpl();

        void update(double* data);

        QChart* chart() { return &mChart_; }

        void setAxisXScale(Second sec);
        void setAxisYScale(qreal yMin, qreal yMax);

    private:
        std::size_t mChannelNum_;
        Second mXRange_;
        qreal mYMin_, mYMax_;
        QValueAxis mAxisX_;
        QValueAxis mAxisY_;
        QLineSeries* mLineSeries_;
        QChart mChart_;
        QVector<QList<QPointF>> mData_;
    };
}   // namespace eegneo
