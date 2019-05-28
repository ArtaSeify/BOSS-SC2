///* -*- c-basic-offset: 4 -*- */
//
//#define _CRT_NO_VA_START_VALIDATION
//
#ifdef _DEBUG
    #undef _DEBUG
    #include <Python.h>
    #define _DEBUG
#else
    #include <Python.h>
#endif
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include "BOSS.h"
#include "GameState.h"
#include "BOSSExperiments.h"
#include "Experiments.h"
#include <chrono>
#include <thread>
#include <string>

using namespace BOSS;
namespace fs = boost::filesystem;

int main(int argc, char * argv[])
{
    BOSS_ASSERT(argc >= 2, "Missing experiment file name?");

    // get path of this executable
    fs::path full_path(fs::initial_path<fs::path>());
    full_path = fs::system_complete(fs::path(argv[0]));
    std::string parent_path = full_path.parent_path().string();
    std::replace(parent_path.begin(), parent_path.end(), '\\', '/');
    std::string path_string = parent_path + "/ML";

    BOSS::CONSTANTS::ExecutablePath = parent_path;
    PyThreadState* PythonState;

    if (argc > 2)
    {
        int expectedArguments = 2;

        BOSS_ASSERT(argc == expectedArguments+1, "must provide %i argument, but got %i", expectedArguments, argc);
        std::string command = "import sys\nsys.path.append(\"" + path_string + "\")\n";

        Py_Initialize();
        PyEval_InitThreads();
        
        PyRun_SimpleString(command.c_str());

        PyObject* PyModule = PyImport_ImportModule("predictor");
        PyObject* PyClass = PyObject_GetAttrString(PyModule, "Network");
        PyObject* PyObj = PyEval_CallObject(PyClass, Py_BuildValue("(s, s, i)", std::string(argv[2]).c_str(), "policy", true));
        BOSS::CONSTANTS::Predictor = PyObject_GetAttrString(PyObj, "predict");
        PyObject* policyValues = PyEval_CallObject(CONSTANTS::Predictor, Py_BuildValue("(s)", ""));

        PythonState = PyEval_SaveThread();
    }

    // Initialize all the BOSS internal data
    BOSS::Init(parent_path + "/SC2Data.json");

    BOSS::BOSSConfig::Instance().ParseConfig(parent_path + "/" + argv[1] + ".txt");
    BOSS::ExperimentsArta::RunExperiments(parent_path + "/" + argv[1] + ".txt");

    if (argc > 2)
    {
        PyEval_RestoreThread(PythonState);
        Py_DECREF(PyImport_ImportModule("threading"));
        Py_Finalize();
    }

    return 0;
}