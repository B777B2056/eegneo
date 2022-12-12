#include "fft.h"

namespace eegneo
{
    static inline bool IsPow2(std::uint64_t x)
    {
        return 0 == (x & (x - 1));
    }

    static inline std::uint64_t UpToPow2(std::uint64_t x) 
    {
        x -= 1;
        x |= (x >> 1); x |= (x >> 2);  x |= (x >> 4);
        x |= (x >> 8); x |= (x >> 16); x |= (x >> 32);
        return x + 1;
    }

    namespace utils
    {
        FFTCalculator::FFTCalculator(std::uint64_t sampleFreqHz)
        {
            this->init(sampleFreqHz);
        }

        std::uint64_t FFTCalculator::fftsize(std::uint64_t sampleFreqHz)
        {
            if (!IsPow2(sampleFreqHz))
            {
                return audiofft::AudioFFT::ComplexSize(UpToPow2(sampleFreqHz));
            }
            else
            {
                return audiofft::AudioFFT::ComplexSize(sampleFreqHz);
            }
        }

        void FFTCalculator::init(std::uint64_t sampleFreqHz)
        {
            mFFTSize_= sampleFreqHz;
            if (!IsPow2(mFFTSize_))
            {
                UpToPow2(mFFTSize_);
            }
            mRe_.resize(audiofft::AudioFFT::ComplexSize(mFFTSize_)); 
            mIm_.resize(audiofft::AudioFFT::ComplexSize(mFFTSize_)); 
            fft.init(mFFTSize_);
        }

        void FFTCalculator::appendSignalData(double data)
        {
            if (mSignal_.size() >= mFFTSize_)
            {
                mSignal_.erase(mSignal_.begin());
            }
            mSignal_.push_back(static_cast<float>(data));
        }

        void FFTCalculator::doFFT()
        {
            if (mSignal_.size() < mFFTSize_)   return;
            fft.fft(mSignal_.data(), mRe_.data(), mIm_.data());
        }  
    }   // namespace utils 
}   // namespace eegneo
