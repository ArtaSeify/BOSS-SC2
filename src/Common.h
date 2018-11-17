/* -*- c-basic-offset: 4 -*- */

#pragma once

#include <cstdio>
#include <cmath>
#include <fstream>
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

#include "BOSSAssert.h"

namespace BOSS
{
    using uint1 = unsigned char;
    using uint2 = unsigned short;
    using uint4 = unsigned int;
    using uint8 = unsigned long long int;

    static_assert(sizeof(uint4) == 4 && sizeof(uint2) == 2, "unexpected sizes");
  
    using int1  = signed char;
    using int2  = signed short;
    using int4  = signed int;

    using ActionID = uint2;
    using RaceID = uint1;
    using TimeType = int4;
    using NumUnits = int2;
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
    };

    namespace Races
    {
        enum {Protoss, Terran, Zerg, NUM_RACES, None};

        RaceID GetRaceID(const std::string & race);
        std::string GetRaceName(RaceID race);
    }
}
