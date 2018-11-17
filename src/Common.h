/* -*- c-basic-offset: 4 -*- */

#pragma once

#include <stdio.h>
#include <math.h>
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
    typedef unsigned char   uint1;
    typedef unsigned short  uint2;
    typedef unsigned int    uint4;

    using TimeType = int;
  
    // ~~~~~!!! CAN CHANGE THIS
    typedef uint2  ActionID;
    typedef uint1  RaceID;

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
