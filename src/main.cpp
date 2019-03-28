///* -*- c-basic-offset: 4 -*- */
//
//#define _CRT_NO_VA_START_VALIDATION
//
#define BOOST_PYTHON_STATIC_LIB
#ifdef _DEBUG
    #undef _DEBUG
    #include <Python.h>
    #define _DEBUG
#else
    #include <Python.h>
#endif
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/python/import.hpp>
#include "BOSS.h"
#include "GameState.h"
#include "BOSSExperiments.h"
#include "Experiments.h"
#include <chrono>
#include <thread>
#include <string>

using namespace BOSS;
namespace python = boost::python;
namespace fs = boost::filesystem;

int main(int argc, char * argv[])
{
    BOSS_ASSERT(argc >= 2, "Missing experiment file name?");

    // get path of this executable
    fs::path full_path(fs::initial_path<fs::path>());
    full_path = fs::system_complete(fs::path(argv[0]));
    std::string parent_path = full_path.parent_path().string();
    std::replace(parent_path.begin(), parent_path.end(), '\\', '/');
    std::string path_string = parent_path + "/data";

    BOSS::CONSTANTS::ExecutablePath = parent_path;

    if (argc > 2)
    {
        int expectedArguments = 2;

        BOSS_ASSERT(argc == expectedArguments+1, "must provide %i argument, but got %i", expectedArguments, argc);
        std::string command = "import sys\nsys.path.append(\"" + path_string + "\")\n";

        Py_Initialize();
        
        PyRun_SimpleString(command.c_str());
        try
        {
            BOSS::CONSTANTS::Predictor = python::import("predictor").attr("Network")(std::string(argv[2]), "both");
        }
        catch (const python::error_already_set&)
        {
            PyErr_Print();
            BOSS_ASSERT(false, "error in evaluating states in Python");
        }
    }

    // Initialize all the BOSS internal data
    BOSS::Init(parent_path + "/SC2Data.json");

    BOSS::BOSSConfig::Instance().ParseConfig(parent_path + "/" + argv[1] + ".txt");
    BOSS::ExperimentsArta::RunExperiments(parent_path + "/" + argv[1] + ".txt");
    
    return 0;
}