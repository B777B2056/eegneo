#include "eegtopo.h"
#include <cstdint>
#include "common/common.h"
#include "utils/config.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace eegneo
{
    void TopoPlot::PyErrorWrapper::init()
    {
        ::PyErr_Fetch(&this->type, &this->value, &this->traceback);
    }

    void TopoPlot::PyErrorWrapper::setErrorMsg()
    {
        this->errorMsg = ::PyUnicode_AsUTF8(::PyObject_Str(this->value));
    }

    TopoPlot::TopoPlot(double sampleFreqHz, std::size_t channelNum)
        : mSampleFreqHz_(sampleFreqHz)
        , mChannelNum_(channelNum)
        , mBufPyList_(nullptr)
        , mPyInstancePtr_(nullptr)
    {
        ::Py_SetPythonHome(L"" _Python3_ROOT_DIR);
        ::Py_Initialize();
        this->mPyError_.init();
        if (!::Py_IsInitialized())
        {
            this->mPyError_.setErrorMsg();
            return;
        }
        if (this->mBufPyList_ = ::PyList_New(channelNum); !this->mBufPyList_)
        {
            this->mPyError_.setErrorMsg();
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
        if (!this->mPyInstancePtr_) return;
        for (std::size_t i = 0; i < this->mChannelNum_; ++i)
        {
            ::PyList_SetItem(this->mBufPyList_, i, ::Py_BuildValue("d", data[i]));
        }
        ::PyObject_CallMethodOneArg(this->mPyInstancePtr_, ::Py_BuildValue("s", "appendData"), this->mBufPyList_);
    }

    void TopoPlot::plotAndSaveFigure()
    {        
        if (!this->mPyInstancePtr_) return;
        ::PyObject_CallMethodNoArgs(this->mPyInstancePtr_, ::Py_BuildValue("s", "plot"));
    }

    PyObject* TopoPlot::buildCtorArgs()
    {
        PyObject* pPyList = ::PyList_New(this->mChannelNum_);
        if (!pPyList)
        {
            this->mPyError_.setErrorMsg();
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
            this->mPyError_.setErrorMsg();
            return nullptr;
        }
        ::PyTuple_SetItem(pArgs, 0, ::Py_BuildValue("d", this->mSampleFreqHz_));
        ::PyTuple_SetItem(pArgs, 1, pPyList);
        ::PyTuple_SetItem(pArgs, 2, ::Py_BuildValue("s", _TOPO_PIC_PATH));
        return pArgs;
    }

#define PY_SYS_APPEND_CMD_HEAD "sys.path.append(\""
#define PY_SYS_APPEND_CMD_TAIL "\")\n"
#define PY_SYS_APPEND_CMD_TOPOPLOT PY_SYS_APPEND_CMD_HEAD _PYSCRIPT_MODULE_PATH PY_SYS_APPEND_CMD_TAIL

    void TopoPlot::constructPyInstance()
    {
        ::PyRun_SimpleString("import sys\n");
        ::PyRun_SimpleString(PY_SYS_APPEND_CMD_TOPOPLOT);
        PyObject* pModule = ::PyImport_ImportModule("topography");  
        if (!pModule)
        {
            this->mPyError_.setErrorMsg();
            ::PyErr_Print();
            return;
        }
        PyObject* pDict = ::PyModule_GetDict(pModule); 
        if (!pDict)
        {
            this->mPyError_.setErrorMsg();
            return;
        }
        PyObject* pClass = ::PyDict_GetItemString(pDict, "TopoPlot");
        if (!pClass)
        {
            this->mPyError_.setErrorMsg();
            return;
        }
        PyObject* pCtor = ::PyInstanceMethod_New(pClass);
        if (!pCtor)
        {
            this->mPyError_.setErrorMsg();
            return;
        }
        this->mPyInstancePtr_ = ::PyObject_CallObject(pCtor, this->buildCtorArgs());
        if (!this->mPyInstancePtr_)
        {
            this->mPyError_.setErrorMsg();
            return;
        }
    }

#undef PY_SYS_APPEND_CMD_HEAD
#undef PY_SYS_APPEND_CMD_TAIL
#undef PY_SYS_APPEND_CMD_TOPOPLOT

    void TopoPlot::destroyPyInstance()
    {

    }
}   // namespace eegneo
