/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "Timer.hpp"
#include "Eval.h"
#include "BuildOrderAbilities.h"
#include "CombatSearchParameters.h"
#include "CombatSearchResults.h"

namespace BOSS
{

    constexpr int BOSS_COMBATSEARCH_TIMEOUT = -1;
    constexpr int MAX_COMBAT_SEARCH_DEPTH = 100;


    class CombatSearch
    {
    protected:

        CombatSearchParameters      m_params;            // parameters that will be used in this search
        CombatSearchResults         m_results;            // the results of the search so far

        int                         m_upperBound;         // the current upper bound for search
        Timer                       m_searchTimer;

        BuildOrderAbilities         m_buildOrder;

        virtual void recurse(const GameState & s, int depth);
        virtual void generateLegalActions(const GameState & state, ActionSetAbilities & legalActions, const CombatSearchParameters & params);

        //virtual double              eval(const GameState & state) const;
        virtual bool isTerminalNode(const GameState & s,int depth);

        virtual void updateResults(const GameState & state);
        virtual bool timeLimitReached();

    public:

        virtual void search();
        virtual void finishSearch();
        virtual void printResults();
        virtual void writeResultsFile(const std::string & dir, const std::string & prefix);

        virtual const CombatSearchResults & getResults() const;

        virtual ~CombatSearch() { }
    };

}
