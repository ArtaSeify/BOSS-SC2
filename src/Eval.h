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
    FracType ArmyTotalResourceSum(const GameState & state);
    FracType ArmyResourceUnit(const GameState & state, int unitIndex);

    bool BuildOrderBetter(const BuildOrderAbilities & buildOrder, const BuildOrderAbilities & compareTo);
    bool StateBetter(const GameState & state, const GameState & compareTo);

    bool StateDominates(const GameState & state, const GameState & other);
}
}
