/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "BOSS.h"
#include "JSONTools.h"
#include "CombatSearchParameters.h"
#include "BuildOrderAbilities.h"

namespace BOSS
{
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

        void runExperimentThread(int thread, int runPerThread);
        void run(int numberOfRuns);
    };
}


