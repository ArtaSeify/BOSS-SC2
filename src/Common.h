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
  
    using TimeType = int;

    using ActionID = uint2;
    using RaceID = uint1;

    // constants declared in data file
    class CONSTANTS
    {
    public:
        static float MPWPF;               // minerals per worker per frame
        static float GPWPF;               // gas per worker per frame
        static float ERPF;                // energy regen per frame
        static float HRPF;                // health regen per frame
        static float SRPF;                // shield regen per frame
        static int   WorkersPerRefinery;  // number of workers per refinery
    };

    namespace Races
    {
        enum {Protoss, Terran, Zerg, NUM_RACES, None};

        RaceID GetRaceID(const std::string & race);
        std::string GetRaceName(RaceID race);
    }
}
