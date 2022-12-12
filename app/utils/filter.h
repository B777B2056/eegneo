#pragma once
#include "third/dsp/dsp.hpp"

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
            void appendSignalData(double originalData);

            double lowPass(double cutoffFreq);
            double highPass(double cutoffFreq);
            double bandPass(double lowCutoffFreq, double highCutoffFreq);
            double notch(double notchFreq);

        private:
            double mSampleFreqHz_;
            dsp::KaiserGenerator mKaiser_;
            dsp::FilterHolder<double> mHolder_;
            constexpr static std::size_t NumTaps = 51;

            std::vector<double> mOriginalSignal_;
            std::vector<double> mFiltResult_;
        };
    }   // namespace utils
}   // namespace eegneo
