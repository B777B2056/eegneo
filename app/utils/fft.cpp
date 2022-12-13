#include "fft.h"

namespace eegneo
{
    namespace utils
    {
        FFTCalculator::FFTCalculator()
            : mRe_(audiofft::AudioFFT::ComplexSize(fftsize_), 0.0)
            , mIm_(audiofft::AudioFFT::ComplexSize(fftsize_), 0.0)
        {
            fft.init(fftsize_);
        }

        void FFTCalculator::appendSignalData(double data)
        {
            if (mSignal_.size() >= fftsize_)
            {
                mSignal_.erase(mSignal_.begin());
            }
            mSignal_.push_back(static_cast<float>(data));
        }

        void FFTCalculator::doFFT()
        {
            if (mSignal_.size() < fftsize_)   return;
            fft.fft(mSignal_.data(), mRe_.data(), mIm_.data());
        }  
    }   // namespace utils 
}   // namespace eegneo
