///* -*- c-basic-offset: 4 -*- */
//
//#define _CRT_NO_VA_START_VALIDATION
//

#include <Python.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "BOSS.h"
#include "GameState.h"
#include "Experiments.h"
#include "GPUQueue.h"

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

        std::ifstream file(parent_path + "/" + argv[1] + ".txt");
        json j;
        file >> j;
        std::string networkType;
        if (j["Experiments"].front()["UsePolicyNetwork"])
        {
            networkType = "policy";
        }
        if (j["Experiments"].front()["UsePolicyValueNetwork"])
        {
            networkType = "both";
        }

        BOSS_ASSERT(argc == expectedArguments+1, "must provide %i argument, but got %i", expectedArguments, argc);
        std::string command = "import sys\nsys.path.append(\"" + path_string + "\")\n";

        Py_Initialize();
        PyEval_InitThreads();
        
        PyRun_SimpleString(command.c_str());

        PyObject* PyModule = PyImport_ImportModule("predictor");
        PyObject* PyClass = PyObject_GetAttrString(PyModule, "Network");
        GPUQueue::getInstance().setPredictorFunction(PyObject_CallFunction(PyClass, "ssi", std::string(argv[2]).c_str(), networkType.c_str(), true));
        PythonState = PyEval_SaveThread();
        GPUQueue::getInstance().getPythonInterpretor();
    }

    // Initialize all the BOSS internal data
    BOSS::Init(parent_path + "/SC2Data.json");

    BOSS::BOSSConfig::Instance().ParseConfig(parent_path + "/" + argv[1] + ".txt");
    BOSS::ExperimentsArta::RunExperiments(parent_path + "/" + argv[1] + ".txt");

    if (argc > 2)
    {
        GPUQueue::getInstance().releasePythonInterpretor();
        PyEval_RestoreThread(PythonState);
        Py_DECREF(PyImport_ImportModule("threading"));
        Py_Finalize();
    }

    return 0;
}