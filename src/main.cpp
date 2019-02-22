///* -*- c-basic-offset: 4 -*- */
//
//#define _CRT_NO_VA_START_VALIDATION
//
#include <Python.h>
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
    // get path of this executable
    fs::path full_path(fs::initial_path<fs::path>());
    full_path = fs::system_complete(fs::path(argv[0]));
    std::string parent_path = full_path.parent_path().string();
    std::replace(parent_path.begin(), parent_path.end(), '\\', '/');
    std::string path_string = parent_path + "/data";

    BOSS::CONSTANTS::ExecutablePath = parent_path;

    if (argc > 1)
    {
        BOSS_ASSERT(argc == 3, "number of arguments must be 2, but got %i", argc);
        std::string command = "import sys\nsys.path.append(\"" + path_string + "\")\n";

        Py_Initialize();
        
        PyRun_SimpleString(command.c_str());
        //if (!strcmp(argv[1], "python"))
        try
        {
            BOSS::CONSTANTS::Predictor = python::import("predictor").attr("Network")(std::string(argv[1]));
        }
        catch (const python::error_already_set&)
        {
            PyErr_Print();
            BOSS_ASSERT(false, "error in evaluating states in Python");
        }
    }

    // Initialize all the BOSS internal data
    BOSS::Init(parent_path + "/SC2Data.json");

    BOSS::BOSSConfig::Instance().ParseConfig(parent_path + "/Experiments.txt");
    BOSS::ExperimentsArta::RunExperiments(parent_path + "/Experiments.txt");
    
    return 0;
}