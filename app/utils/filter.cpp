#include "filter.h"

namespace eegneo
{
    namespace utils
    {
        Filter::Filter()
            : Filter(0.0)
        {

        }

        Filter::Filter(double sampleFreqHz)
            : mSampleFreqHz_(sampleFreqHz)
        {

        }

        double Filter::lowPass(double data, double cutoffFreq)
        {
            mLowPassFilter_.setup(mSampleFreqHz_, cutoffFreq);
            return mLowPassFilter_.filter(data);
        }

        double Filter::highPass(double data, double cutoffFreq)
        {
            mHighPassFilter_.setup(mSampleFreqHz_, cutoffFreq);
            return mHighPassFilter_.filter(data);
        }

        double Filter::bandPass(double data, double lowCutoffFreq, double highCutoffFreq)
        {
            mBandPassFilter_.setup(mSampleFreqHz_, lowCutoffFreq, highCutoffFreq);
            return mBandPassFilter_.filter(data);
        }

        double Filter::notch(double data, double notchFreq)
        {
            mNotchFilter_.setup(mSampleFreqHz_, notchFreq, 1.0);
            return mNotchFilter_.filter(data);
        }
    }   // namespace utils
}   // namespace eegneo
