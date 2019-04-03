/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "BuildOrderAbilities.h"
#include "CombatSearchParameters.h"

namespace BOSS
{
    namespace Eval
    {
        static FracType GASWORTH = 1.5;

        //double ArmyCompletedResourceSum(const GameState & state);
        FracType ArmyInProgressResourceSum(const GameState & state);
        FracType ArmyTotalResourceSum(const GameState & state);
        FracType UnitValue(const GameState & state, ActionType type);

        FracType UnitValueWithOpponent(const GameState & state, ActionType type, const CombatSearchParameters & params);
        FracType UnitWeight(const GameState & state, ActionType type, const CombatSearchParameters & params);

        bool BuildOrderBetter(const BuildOrderAbilities & buildOrder, const BuildOrderAbilities & compareTo);
        bool StateBetter(const GameState & state, const GameState & compareTo);

        bool StateDominates(const GameState & state, const GameState & other);
    }
}
