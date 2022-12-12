#include "filter.h"

namespace eegneo
{
    namespace utils
    {
        Filter::Filter()
            : Filter(0.0)
        {

        }

        Filter::Filter(double sampleFreqHz)
            : mSampleFreqHz_(sampleFreqHz), mKaiser_(10.)
            , mOriginalSignal_(0), mFiltResult_(NumTaps)
        {

        }

        void Filter::appendSignalData(double originalData)
        {
            if (mOriginalSignal_.size() >= NumTaps)
            {
                mOriginalSignal_.erase(mOriginalSignal_.begin());
            }
            mOriginalSignal_.push_back(originalData);
        }

        double Filter::lowPass(double cutoffFreq)
        {
            if (mOriginalSignal_.size() < NumTaps) return -1.0;
            auto filterCoeffs = dsp::FirLowPassFilter(NumTaps, cutoffFreq, mSampleFreqHz_, mKaiser_);
            mHolder_.Initialise(NumTaps, filterCoeffs, true);
            mHolder_(mOriginalSignal_.begin(), mOriginalSignal_.end(), mFiltResult_.begin(), true);
            return mFiltResult_.back();
        }

        double Filter::highPass(double cutoffFreq)
        {
            if (mOriginalSignal_.size() < NumTaps) return -1.0;
            auto filterCoeffs = dsp::FirHighPassFilter(NumTaps, cutoffFreq, mSampleFreqHz_, mKaiser_);
            mHolder_.Initialise(NumTaps, filterCoeffs, true);
            mHolder_(mOriginalSignal_.begin(), mOriginalSignal_.end(), mFiltResult_.begin(), true);
            return mFiltResult_.back();
        }

        double Filter::bandPass(double lowCutoffFreq, double highCutoffFreq)
        {
            if (mOriginalSignal_.size() < NumTaps) return -1.0;
            double bandWidth = highCutoffFreq - lowCutoffFreq;
            auto filterCoeffs = dsp::FirBandPassFilter(NumTaps, lowCutoffFreq + (bandWidth / 2.0), bandWidth, mSampleFreqHz_, mKaiser_);
            mHolder_.Initialise(NumTaps, filterCoeffs, true);
            mHolder_(mOriginalSignal_.begin(), mOriginalSignal_.end(), mFiltResult_.begin(), true);
            return mFiltResult_.back();
        }

        double Filter::notch(double notchFreq)
        {
            if (mOriginalSignal_.size() < NumTaps) return -1.0;
            auto filterCoeffs = dsp::FirNotchFilter(NumTaps, notchFreq, 4.0, mSampleFreqHz_, mKaiser_);
            mHolder_.Initialise(NumTaps, filterCoeffs, true);
            mHolder_(mOriginalSignal_.begin(), mOriginalSignal_.end(), mFiltResult_.begin(), true);
            return mFiltResult_.back();
        }
    }   // namespace utils
}   // namespace eegneo
