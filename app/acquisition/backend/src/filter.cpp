#include "filter.h"

namespace eegneo
{
    Filter::Filter()
        : Filter(0.0)
    {

    }

    Filter::Filter(double sampleFreqHz)
        : mSampleFreqHz_(sampleFreqHz)
    {

    }

    void Filter::setupLowPassFilter(double cutoffFreq)
    {
        mLowPassFilter_.setup(mSampleFreqHz_, cutoffFreq);
    }

    void Filter::setupHighPassFilter(double cutoffFreq)
    {
        mHighPassFilter_.setup(mSampleFreqHz_, cutoffFreq);
    }

    void Filter::setupBandPassFilter(double lowCutoffFreq, double highCutoffFreq)
    {
        double centerFrequency = lowCutoffFreq + (highCutoffFreq - lowCutoffFreq) / 2;
        double widthFrequency = (highCutoffFreq - lowCutoffFreq) / 2;
        mBandPassFilter_.setup(mSampleFreqHz_, centerFrequency, widthFrequency);
    }   

    void Filter::setupNotchFilter(double cutoffFreq)
    {
        mNotchFilter_.setup(mSampleFreqHz_, cutoffFreq, 5.0);
    }

    double Filter::lowPass(double data)
    {
        return mLowPassFilter_.filter(data);
    }

    double Filter::highPass(double data)
    {
        return mHighPassFilter_.filter(data);
    }

    double Filter::bandPass(double data)
    {
        return mBandPassFilter_.filter(data);
    }

    double Filter::notch(double data)
    {
        return mNotchFilter_.filter(data);
    }
}   // namespace eegneo
