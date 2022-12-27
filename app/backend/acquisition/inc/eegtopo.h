#pragma once
#include <cstddef>

typedef struct _object PyObject;

namespace eegneo
{
    class TopoPlot
    {
    public:
        TopoPlot(double sampleFreqHz, std::size_t channelNum);
        ~TopoPlot();

        void appendNewData(const double* data);
        void plotAndSaveFigure();

    private:
        double mSampleFreqHz_;
        std::size_t mChannelNum_;
        PyObject* mBufPyList_;
        PyObject* mPyInstancePtr_;

        PyObject* buildCtorArgs();
        void constructPyInstance();
        void destroyPyInstance();
    };
}   // namespace eegneo
