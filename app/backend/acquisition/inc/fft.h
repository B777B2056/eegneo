#pragma once
#include <cstdint>
#include <vector>
#include "third/fft/fft.h"

namespace eegneo
{
    class FFTCalculator
    {
    public:
        FFTCalculator();

        void appendSignalData(double data);
        void doFFT();

        const std::vector<float>& real() const { return mRe_; }
        const std::vector<float>& im() const { return mIm_; }

    private:
        audiofft::AudioFFT fft;
        const std::uint64_t fftsize_;
        std::vector<float> mSignal_;
        std::vector<float> mRe_;
        std::vector<float> mIm_;
    };
}   // namespace eegneo
