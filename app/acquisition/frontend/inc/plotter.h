#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <tuple>
#include <QChart>
#include <QColor>
#include <QValueAxis>
#include <QCategoryAxis>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
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

    struct AbstractPlotter
    {
        virtual void update() = 0;
    };

    class WavePlotter : public AbstractPlotter
    {
    public:
        WavePlotter() : mChart_(new QChart()) {}
        WavePlotter(WavePlotter&&) = default;
        WavePlotter& operator=(WavePlotter&&) = default;
        virtual ~WavePlotter() {  }

        virtual void update() = 0;

        QChart* chart() { return mChart_; }

    protected:
        QChart* mChart_;
    };

    class EEGWavePlotter : public WavePlotter
    {
    public:
        EEGWavePlotter(std::size_t channelNum, std::size_t sampleRate, std::size_t freshMs, double* buf);
        ~EEGWavePlotter();

        void setAxisXScale(Second sec);
        void setAxisYScale(int maxVoltage);
        void addOneMarkerLine(const QString& eventStr);
        void update() override;

    private:
        int mMaxVoltage_ = 0;
        std::size_t mSampleRate_;
        std::size_t mFreshRate_;
        qreal mMoveOffset_;
        QLineSeries* mLineSeries_;
        QVector<QList<QPointF>> mData_;
        double* mBuf_;

        QCategoryAxis mAxisX_;
        QCategoryAxis mAxisY_;

        std::vector<std::tuple<QLineSeries*, QGraphicsSimpleTextItem*, QList<QPointF>>> mMarkerLineTbl_;

        void setLineColor(QLineSeries* line);   
        void updateWave();
        void updateMarker();
    };

    class FFTWavePlotter : public WavePlotter
    {
    public:
        FFTWavePlotter(std::size_t channelNum, std::size_t sampleRate);
        ~FFTWavePlotter();

        void setAxisXScale(Frequency freqMax);
        void setAxisYScale(int maxVoltage);
        void update() override;

        std::vector<float>& real(std::size_t idx) { return mFFTBuf_[idx * 2]; }
        std::vector<float>& im(std::size_t idx) { return mFFTBuf_[2 * idx + 1]; }

    private:
        std::size_t mFFTSize_;
        std::size_t mSampleRate_;
        std::size_t mChannelNum_;

        QValueAxis mAxisX_;
        QValueAxis mAxisY_;

        std::vector<std::vector<float>> mFFTBuf_;
        std::vector<std::tuple<QLineSeries*, QList<QPointF>>> mLines_;
    };

    class TopographyPlotter : public AbstractPlotter
    {
    public:
        TopographyPlotter(QGraphicsView* view);
        ~TopographyPlotter();

        void showEvent();
        void update() override;

    private:
        QImage mImg_;
        QGraphicsView* mView_;
        QGraphicsScene* mGraphicsScene_;
        QGraphicsPixmapItem* mGraphicsPixmapItem_;
    };
}   // namespace eegneo
