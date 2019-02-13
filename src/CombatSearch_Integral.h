/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "Timer.hpp"
#include "Eval.h"
#include "BuildOrder.h"
#include "CombatSearch.h"
#include "CombatSearchParameters.h"
#include "CombatSearchResults.h"
#include "CombatSearch_IntegralData.h"
#include "CombatSearch_IntegralDataFinishedUnits.h"

namespace BOSS
{

    class CombatSearch_Integral : public CombatSearch
    {
        virtual void recurse(const GameState & s, int depth);

    protected:
        //CombatSearch_IntegralData   m_integral;
        CombatSearch_IntegralDataFinishedUnits  m_integral;
        std::ofstream                           m_file;
        std::stringstream                       m_ss;

    public:
    
        CombatSearch_Integral(const CombatSearchParameters p = CombatSearchParameters(), int run = 0);
        ~CombatSearch_Integral();
    
        FracType CombatSearch_Integral::recurseReturnValue(const GameState & state, int depth);

        virtual void printResults();
        virtual void setBestBuildOrder();
        virtual void writeResultsFile(const std::string & dir, const std::string & filename);
    };
}
