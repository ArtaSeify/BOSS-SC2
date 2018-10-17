#pragma once

#include "Common.h"
#include "GameState.h"
#include "BuildOrder.h"

namespace BOSS
{
namespace Eval
{
    double ArmyCompletedResourceSum(const GameState & state);
    double ArmyTotalResourceSum(const GameState & state);
    double ArmyResourceSumToIndex(const GameState & state, size_t finishedUnitsIndex);

    bool BuildOrderBetter(const BuildOrder & buildOrder, const BuildOrder & compareTo);

    bool StateDominates(const GameState & state, const GameState & other);
}
}
