/* -*- c-basic-offset: 4 -*- */

#pragma once

#include <atomic>

#include "BOSS.h"
#include "JSONTools.h"
#include "CombatSearchParameters.h"
#include "CombatSearchResults.h"
#include "BuildOrderAbilities.h"


namespace BOSS
{
    class CombatSearch;
    class IntegralExperimentOMP
    {
        std::string                 m_name;
        std::string                 m_outputDir;
        CombatSearchParameters      m_params;
        RaceID                      m_race;
        std::string                 m_searchType;

    public:

        IntegralExperimentOMP();
        IntegralExperimentOMP(const std::string& experimentName, const json& exp);

        void runExperimentThread(int thread, int numRuns, int startingIndex);
        void runTotalTimeExperiment(int run);
        void run(int numberOfRuns);
    };
}


