/* -*- c-basic-offset: 4 -*- */

#include "Common.h"

using namespace BOSS;

// defining static variables
float   CONSTANTS::MPWPF;
float   CONSTANTS::GPWPF;
float   CONSTANTS::ERPF;
float   CONSTANTS::HRPF;
float   CONSTANTS::SRPF;
short   CONSTANTS::WorkersPerRefinery;

RaceID Races::GetRaceID(const std::string & race)
{
    if      (race == "Protoss") { return Races::Protoss; }
    else if (race == "Terran")  { return Races::Terran; }
    else if (race == "Zerg")    { return Races::Zerg; }
    else                        { return Races::None; }
}

std::string Races::GetRaceName(RaceID race)
{
    if      (race == Races::Protoss)    { return "Protoss"; }
    else if (race == Races::Terran)     { return "Terran"; }
    else if (race == Races::Zerg)       { return "Zerg"; }
    else                                { return "None"; }
}
