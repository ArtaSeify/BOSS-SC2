/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"
#include "GameState.h"

namespace BOSS
{
    class BuildOrderSearchGoal
    {
        std::vector<size_t> m_goalUnits;                 // vector of goal number of units indexed by ActionType ID
        std::vector<size_t> m_goalUnitsMax;              // vector of goal max number of units indexed by ActionType ID
        size_t              m_supplyRequiredVal;         // amount of supply required for all goal units in _goalUnits

        void calculateSupplyRequired();

    public:

        BuildOrderSearchGoal();

        bool                operator == (const BuildOrderSearchGoal & g);
        bool                hasGoal() const;
        bool                isAchievedBy(const GameState & state);

        int                 supplyRequired() const;
        int                 getGoal(ActionType a) const;
        int                 getGoalMax(ActionType a) const;

        void                setGoal(ActionType a, const size_t num);
        void                setGoalMax(ActionType a, const size_t num);
        std::string         toString() const;
    };
}
