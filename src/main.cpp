///* -*- c-basic-offset: 4 -*- */
//
//#define _CRT_NO_VA_START_VALIDATION
//

#include "BOSS.h"
#include "Experiments.h"

#include <string>

using namespace BOSS;

int main(int argc, char * argv[])
{
    BOSS_ASSERT(argc >= 2, "Missing experiment file name?");

    // get path of this executable
    std::string executable_path = argv[0];
    std::string bin_path = executable_path.substr(0, executable_path.find_last_of("\\"));

    // Initialize all the BOSS internal data
    BOSS::Init(bin_path + "/SC2Data.json");

    BOSS::ExperimentsArta::RunExperiments(bin_path + "/" + argv[1] + ".txt");

    return 0;
}