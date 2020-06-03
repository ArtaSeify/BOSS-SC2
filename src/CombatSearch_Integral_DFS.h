/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "Timer.hpp"
#include "Eval.h"
#include "CombatSearch.h"
#include "CombatSearchParameters.h"
#include "CombatSearchResults.h"
#include "CombatSearch_IntegralData_FinishedUnits.h"

namespace BOSS
{

    class CombatSearch_Integral_DFS : public CombatSearch
    {
    protected:
        CombatSearch_IntegralData_FinishedUnits m_integral;
        FracType                                m_highestValueFound;
        FracType                                m_highestValueThusFar;

        std::ofstream                           m_fileStates;
        std::stringstream                       m_ssStates;
        std::vector<std::uint8_t>               m_jStates;
        std::stringstream                       m_ssHighestValue;

        int m_filesWritten;
        int m_statesWritten;

        std::string m_dir;
        std::string m_prefix;
        std::string m_name;

    private:
        virtual void run(const GameState& state, int depth);

    protected:
        FracType recurse(const GameState & state, int depth);

    public:
        CombatSearch_Integral_DFS(const CombatSearchParameters p = CombatSearchParameters());

        virtual void writeResultsFile(const std::string & dir, const std::string & filename);
    };
}
