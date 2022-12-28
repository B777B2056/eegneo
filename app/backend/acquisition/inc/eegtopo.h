#pragma once
#include <cstddef>
#include <string>

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

        const std::string& error() const { return this->mPyError_.errorMsg; }

    private:
        struct PyErrorWrapper
        {
            PyObject* type = nullptr;
            PyObject* value = nullptr;
            PyObject* traceback = nullptr;
            std::string errorMsg = "";

            void init();
            void setErrorMsg();
        };

    private:
        PyErrorWrapper mPyError_;
        double mSampleFreqHz_;
        std::size_t mChannelNum_;
        PyObject* mBufPyList_;
        PyObject* mPyInstancePtr_;

        PyObject* buildCtorArgs();
        void constructPyInstance();
        void destroyPyInstance();
    };
}   // namespace eegneo
