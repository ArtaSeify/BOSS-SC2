/* -*- c-basic-offset: 4 -*- */

#pragma once

#include <atomic>

#include "BOSS.h"
#include "JSONTools.h"
#include "CombatSearchParameters.h"
#include "BuildOrderAbilities.h"


namespace BOSS
{
    class CombatSearch;
    class IntegralExperiment
    {
        std::string                 m_name;
        std::string                 m_outputDir;
        CombatSearchParameters      m_params;
        RaceID                      m_race;
        std::string                 m_searchType;

    public:

        IntegralExperiment();
        IntegralExperiment(const std::string & experimentName, const json & exp);

        void runExperimentThread(int thread, int numRuns, int startingIndex);
        void runExperimentTotalTimeThread(std::unique_ptr<CombatSearch> & combatSearch, const std::string & outputDir, std::atomic<bool> & finish);
        void runExperimentsTotalTimeThread(int thread, int numRuns, int startingIndex);
        void run(int numberOfRuns);
    };
}


