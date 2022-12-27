#include "eegtopo.h"
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include "common/common.h"
#include "utils/config.h"

#define PY_SSIZE_T_CLEAN
#include "Python.h"

namespace eegneo
{
    constexpr const wchar_t* PYTHON_INTERRUPT_PATH = L"E:/anaconda3/envs/qtcpp_env";

    TopoPlot::TopoPlot(double sampleFreqHz, std::size_t channelNum)
        : mSampleFreqHz_(sampleFreqHz)
        , mChannelNum_(channelNum)
        , mBufPyList_(nullptr)
        , mPyInstancePtr_(nullptr)
    {
        ::Py_SetPythonHome(PYTHON_INTERRUPT_PATH);
        ::Py_Initialize();
        if (!::Py_IsInitialized())
        {
            // Error
            throw std::runtime_error{"Py_Initialize"};
            return;
        }
        if (this->mBufPyList_ = ::PyList_New(channelNum); !this->mBufPyList_)
        {
            // Error
            throw std::runtime_error{"PyList_New"};
            return;
        }
        this->constructPyInstance();
    }

    TopoPlot::~TopoPlot()
    {
        this->destroyPyInstance();
        ::Py_Finalize();
    }

    void TopoPlot::appendNewData(const double* data)
    {
        for (std::size_t i = 0; i < this->mChannelNum_; ++i)
        {
            ::PyList_SetItem(this->mBufPyList_, i, ::Py_BuildValue("d", data[i]));
        }
        ::PyObject_CallMethodOneArg(this->mPyInstancePtr_, ::Py_BuildValue("s", "appendData"), this->mBufPyList_);
    }

    void TopoPlot::plotAndSaveFigure()
    {        
        ::PyObject_CallMethodNoArgs(this->mPyInstancePtr_, ::Py_BuildValue("s", "plot"));
    }

    PyObject* TopoPlot::buildCtorArgs()
    {
        PyObject* pPyList = ::PyList_New(this->mChannelNum_);
        if (!pPyList)
        {
            // Error
            throw std::runtime_error{"PyList_New ARGS"};
            return nullptr;
        }

        auto& config = eegneo::utils::ConfigLoader::instance();
        auto names = config.get<std::vector<std::string>>("Acquisition", "Electrodes");
        for (std::size_t i = 0; i < this->mChannelNum_; ++i)
        {
            ::PyList_SetItem(pPyList, i, ::Py_BuildValue("s", names[i].c_str()));
        }

        PyObject* pArgs = ::PyTuple_New(3);
        if (!pArgs)
        {
            // Error
            throw std::runtime_error{"PyTuple_New ARGS"};
            return nullptr;
        }
        ::PyTuple_SetItem(pArgs, 0, ::Py_BuildValue("d", this->mSampleFreqHz_));
        ::PyTuple_SetItem(pArgs, 1, pPyList);
        ::PyTuple_SetItem(pArgs, 2, ::Py_BuildValue("s", TOPO_PIC_PATH));
        return pArgs;
    }

    void TopoPlot::constructPyInstance()
    {
        PyObject* pModule = ::PyImport_ImportModule("topography");  
        if (!pModule)
        {
            // Error
            throw std::runtime_error{"PyImport_ImportModule"};
            return;
        }
        PyObject* pDict = ::PyModule_GetDict(pModule); 
        if (!pDict)
        {
            // Error
            throw std::runtime_error{"PyModule_GetDict"};
            return;
        }
        PyObject* pClass = ::PyDict_GetItemString(pDict, "TopoPlot");
        if (!pClass)
        {
            // Error
            throw std::runtime_error{"PyDict_GetItemString"};
            return;
        }
        PyObject* pCtor = ::PyInstanceMethod_New(pClass);
        if (!pCtor)
        {
            // Error
            throw std::runtime_error{"PyInstanceMethod_New"};
            return;
        }
        this->mPyInstancePtr_ = ::PyObject_CallObject(pCtor, this->buildCtorArgs());
        if (!this->mPyInstancePtr_)
        {
            // Error
            throw std::runtime_error{"PyObject_CallObject"};
            return;
        }
    }

    void TopoPlot::destroyPyInstance()
    {

    }
}   // namespace eegneo
