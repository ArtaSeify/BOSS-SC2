/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "BuildOrderAbilities.h"

namespace BOSS
{
    namespace Eval
    {
        //double ArmyCompletedResourceSum(const GameState & state);
        float ArmyTotalResourceSum(const GameState & state);
        float ArmyResourceSumToIndex(const GameState & state, int finishedUnitsIndex);

        bool BuildOrderBetter(const BuildOrderAbilities & buildOrder, const BuildOrderAbilities & compareTo);
        bool StateBetter(const GameState & state, const GameState & compareTo);

        bool StateDominates(const GameState & state, const GameState & other);
    }
}
