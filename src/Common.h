/* -*- c-basic-offset: 4 -*- */

#pragma once
#define BOOST_PYTHON_STATIC_LIB
#include <boost/python/object/function.hpp>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <limits>
#include <map>
#include <set>
#include <algorithm>
#include "json/json.hpp"

using json = nlohmann::json;
namespace python = boost::python;

#include "BOSSAssert.h"

namespace BOSS
{
    using uint1 = unsigned char;
    using uint2 = unsigned short;
    using uint4 = unsigned int;
    using uint8 = unsigned long long int;

    using sint1 = signed char;
    using sint2 = signed short;
    using sint4 = signed int;
    using sint8 = signed long long int;

    static_assert(sizeof(uint1) == 1, "unexpected size");
    static_assert(sizeof(uint2) == 2, "unexpected size");
    static_assert(sizeof(uint4) == 4, "unexpected size");
    static_assert(sizeof(uint8) == 8, "unexpected size");
    static_assert(sizeof(sint1) == 1, "unexpected size");
    static_assert(sizeof(sint2) == 2, "unexpected size");
    static_assert(sizeof(sint4) == 4, "unexpected size");
    static_assert(sizeof(sint8) == 8, "unexpected size");

    static_assert(sizeof(int) >= 4, "unexpected size");
    
    using ActionID = uint2;
    using RaceID   = uint1;
    using TimeType = sint2;
    using NumUnits = sint2;
    using FracType = float;

    // constants declared in data file
    class CONSTANTS
    {
    public:
        static FracType MPWPF;               // minerals per worker per frame
        static FracType GPWPF;               // gas per worker per frame
        static FracType ERPF;                // energy regen per frame
        static FracType HRPF;                // health regen per frame
        static FracType SRPF;                // shield regen per frame
        static NumUnits WorkersPerRefinery;  // number of workers per refinery
        static NumUnits WorkersPerDepot;     // number of workers gathering minerals per depot
        //static boost::python::object Predictor; // reference to module that contains the network used for evaluations
        static PyObject* Predictor; // reference to module that contains the network used for evaluations
        static PyThreadState* PythonState;
        static std::string ExecutablePath;      // path to this executable
    };

    namespace Races
    {
        enum {Protoss, Terran, Zerg, NUM_RACES, None};

        RaceID GetRaceID(const std::string & race);
        std::string GetRaceName(RaceID race);
    }
}
