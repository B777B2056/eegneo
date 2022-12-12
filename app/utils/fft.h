#pragma once
#include <cstdint>
#include <vector>

namespace eegneo
{
    namespace utils
    {
        class FFTCalculator
        {
        public:
            FFTCalculator() : mFFTSize_(0) {} 
            FFTCalculator(std::uint64_t sampleFreqHz);

            static std::uint64_t fftsize(std::uint64_t sampleFreqHz);

            void init(std::uint64_t sampleFreqHz);
            void appendSignalData(double data);
            void doFFT();

            const std::vector<float>& real() const { return mRe_; }
            const std::vector<float>& im() const { return mIm_; }

        private:
            std::uint64_t mFFTSize_;
            std::vector<float> mSignal_;
            std::vector<float> mRe_;
            std::vector<float> mIm_;
        };
    }   // namespace utils
}   // namespace eegneo
