#pragma once
#include "third/iir/include/Iir.h"

namespace eegneo
{
    namespace utils
    {
        class Filter
        {
        public:
            Filter();
            Filter(double sampleFreqHz);

            void setSampleFreqHz(double sampleFreqHz) { this->mSampleFreqHz_ = sampleFreqHz; }

            double lowPass(double data, double cutoffFreq);
            double highPass(double data, double cutoffFreq);
            double bandPass(double data, double lowCutoffFreq, double highCutoffFreq);
            double notch(double data, double notchFreq);

        private:
            double mSampleFreqHz_;
            constexpr static std::size_t order = 4;
            Iir::Butterworth::LowPass<order> mLowPassFilter_;
            Iir::Butterworth::HighPass<order> mHighPassFilter_;
            Iir::Butterworth::BandPass<order> mBandPassFilter_;
            Iir::Butterworth::BandStop<order> mNotchFilter_;
        };
    }   // namespace utils
}   // namespace eegneo
