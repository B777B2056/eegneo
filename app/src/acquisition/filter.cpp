#include "acquisition/filter.h"

namespace eegneo
{
    namespace utils
    {
        Filter::Filter(double sampleFreqHz)
            : mSampleFreqHz_(sampleFreqHz)
            , mKaiser_(10.)
        {

        }

        void Filter::lowPass(double cutoffFreq, std::vector<double>& signal, std::vector<double>& result)
        {
            auto filterCoeffs = dsp::FirLowPassFilter(NumTaps, cutoffFreq, mSampleFreqHz_, mKaiser_);
            mHolder_.Initialise(signal.size(), filterCoeffs, true);
            mHolder_(signal.begin(), signal.end(), result.begin(), true);
        }

        void Filter::highPass(double cutoffFreq, std::vector<double>& signal, std::vector<double>& result)
        {
            auto filterCoeffs = dsp::FirHighPassFilter(NumTaps, cutoffFreq, mSampleFreqHz_, mKaiser_);
            mHolder_.Initialise(signal.size(), filterCoeffs, true);
            mHolder_(signal.begin(), signal.end(), result.begin(), true);
        }

        void Filter::bandPass(double lowCutoffFreq, double highCutoffFreq, std::vector<double>& signal, std::vector<double>& result)
        {
            double bandWidth = highCutoffFreq - lowCutoffFreq;
            auto filterCoeffs = dsp::FirBandPassFilter(NumTaps, lowCutoffFreq + (bandWidth / 2.0), bandWidth, mSampleFreqHz_, mKaiser_);
            mHolder_.Initialise(signal.size(), filterCoeffs, true);
            mHolder_(signal.begin(), signal.end(), result.begin(), true);
        }

        void Filter::notch(double notchFreq, std::vector<double>& signal, std::vector<double>& result)
        {
            auto filterCoeffs = dsp::FirNotchFilter(NumTaps, notchFreq, 4.0, mSampleFreqHz_, mKaiser_);
            mHolder_.Initialise(signal.size(), filterCoeffs, true);
            mHolder_(signal.begin(), signal.end(), result.begin(), true);
        }
    }   // namespace utils
}   // namespace eegneo
