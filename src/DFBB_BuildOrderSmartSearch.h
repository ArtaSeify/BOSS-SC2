/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "DFBB_BuildOrderStackSearch.h"
#include "Timer.hpp"

namespace BOSS
{
    class DFBB_BuildOrderSmartSearch
    {
        //!!! PROBLEM UNUSED RaceID                              m_race;

        DFBB_BuildOrderSearchParameters m_params;
        BuildOrderSearchGoal m_goal;

        std::vector<ActionType> m_relevantActions;

        GameState m_initialState;

        int m_searchTimeLimit;

        Timer m_searchTimer;

        DFBB_BuildOrderStackSearch m_stackSearch;
        DFBB_BuildOrderSearchResults m_results;

        void doSearch();
        void calculateSearchSettings();
        void setPrerequisiteGoalMax();
        void recurseOverStrictDependencies(ActionType action);
        void setRelevantActions();
        void setRepetitions();

        int calculateSupplyProvidersRequired();
        int calculateRefineriesRequired();

        RaceID getRace() const;

    public:

        DFBB_BuildOrderSmartSearch();

        void addGoal(ActionType a, int count);
        void setGoal(const BuildOrderSearchGoal & goal);
        void setState(const GameState & state);
        void print();
        void setTimeLimit(int n);

        void search();

        const DFBB_BuildOrderSearchResults & getResults() const;
        const DFBB_BuildOrderSearchParameters & getParameters();
    };

}
