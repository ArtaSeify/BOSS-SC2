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
        virtual void recurse(const GameState & state, int depth);

    protected:
        //CombatSearch_IntegralData   m_integral;
        CombatSearch_IntegralDataFinishedUnits  m_integral;
        FracType                                m_highestValueFound;
        std::ofstream                           m_fileStates;
        std::stringstream                       m_ssStates;
        std::vector<std::uint8_t>               m_jStates;
        std::stringstream                       m_ssHighestValue;

        int m_filesWritten;
        int m_statesWritten;

        std::string m_dir;
        std::string m_prefix;
        std::string m_name;

    protected:
        FracType recurseReturnValue(const GameState & state, int depth);

    public:
        static FracType highestValueThusFar;

        CombatSearch_Integral(const CombatSearchParameters p = CombatSearchParameters(),
            const std::string & dir = "", const std::string & prefix = "", const std::string & name = "");
        ~CombatSearch_Integral();
    
        virtual void printResults();
        virtual void writeResultsFile(const std::string & dir, const std::string & filename);
    };
}
