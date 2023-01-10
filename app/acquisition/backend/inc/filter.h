#pragma once
#include "third/iir/include/Iir.h"

namespace eegneo
{
    class Filter
    {
    public:
        Filter();
        Filter(double sampleFreqHz);

        void setSampleFreqHz(double sampleFreqHz) { this->mSampleFreqHz_ = sampleFreqHz; }

        void setupLowPassFilter(double cutoffFreq);
        void setupHighPassFilter(double cutoffFreq);
        void setupBandPassFilter(double lowCutoffFreq, double highCutoffFreq);
        void setupNotchFilter(double cutoffFreq);

        double lowPass(double data);
        double highPass(double data);
        double bandPass(double data);
        double notch(double data);

    private:
        double mSampleFreqHz_;
        constexpr static std::size_t order = 1;
        Iir::Butterworth::LowPass<order> mLowPassFilter_;
        Iir::Butterworth::HighPass<order> mHighPassFilter_;
        Iir::Butterworth::BandPass<order> mBandPassFilter_;
        Iir::Butterworth::BandStop<order> mNotchFilter_;
    };
}   // namespace eegneo
