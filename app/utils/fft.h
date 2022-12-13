#pragma once
#include <cstdint>
#include <vector>
#include "third/fft/fft.h"

namespace eegneo
{
    namespace utils
    {
        class FFTCalculator
        {
        public:
            FFTCalculator();
            static std::uint64_t fftsize() { return fftsize_; }

            void appendSignalData(double data);
            void doFFT();

            const std::vector<float>& real() const { return mRe_; }
            const std::vector<float>& im() const { return mIm_; }

        private:
            audiofft::AudioFFT fft;
            constexpr static std::uint64_t fftsize_ = 32;
            std::vector<float> mSignal_;
            std::vector<float> mRe_;
            std::vector<float> mIm_;
        };
    }   // namespace utils
}   // namespace eegneo
