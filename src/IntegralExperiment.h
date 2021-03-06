/* -*- c-basic-offset: 4 -*- */

#pragma once

#include <atomic>

#include "BOSS.h"
#include "JSONTools.h"
#include "CombatSearchParameters_MCTS.h"

namespace BOSS
{
    class CombatSearch;
    class IntegralExperiment
    {
        std::string                 m_name;
        std::string                 m_outputDir;
        CombatSearchParameters_MCTS m_params;
        RaceID                      m_race;
        std::string                 m_searchType;

    public:

        IntegralExperiment();
        IntegralExperiment(const std::string& experimentName, const json& exp);

        void runExperimentThread(int run);
        void runTotalTimeExperiment(int run);
        void run(int numberOfRuns);
    };
}


