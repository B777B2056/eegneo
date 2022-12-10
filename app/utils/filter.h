#pragma once
#include "third/dsp/dsp_filter.hpp"
#include "third/dsp/dsp_window_functions.hpp"

namespace eegneo
{
    namespace utils
    {
        class Filter
        {
        public:
            Filter() : mSampleFreqHz_(0.0), mKaiser_(10.) {}
            Filter(double sampleFreqHz);
            void setSampleFreqHz(double sampleFreqHz) { this->mSampleFreqHz_ = sampleFreqHz; }

            constexpr static std::size_t numTaps() { return NumTaps; }

            void lowPass(double cutoffFreq, std::vector<double>& signal, std::vector<double>& result);
            void highPass(double cutoffFreq, std::vector<double>& signal, std::vector<double>& result);
            void bandPass(double lowCutoffFreq, double highCutoffFreq, std::vector<double>& signal, std::vector<double>& result);
            void notch(double notchFreq, std::vector<double>& signal, std::vector<double>& result);

        private:
            double mSampleFreqHz_;
            dsp::KaiserGenerator mKaiser_;
            dsp::FilterHolder<double> mHolder_;
            constexpr static std::size_t NumTaps = 51;
        };
    }   // namespace utils
}   // namespace eegneo
