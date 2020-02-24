/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "BuildOrderSearchGoal.h"
#include "BuildOrder.h"
#include "BuildOrderAbilities.h"
#include "ActionSet.h"

namespace BOSS
{
namespace Tools
{
    int         GetUpperBound(const GameState & state, const BuildOrderSearchGoal & goal);
    int         GetLowerBound(const GameState & state, const BuildOrderSearchGoal & goal);
    int         CalculatePrerequisitesLowerBound(const GameState & state, const ActionSetAbilities & needed, int timeSoFar, int depth = 0);
    void        InsertActionIntoBuildOrder(BuildOrder & result, const BuildOrder & buildOrder, const GameState & initialState, ActionType action);
    void        CalculatePrerequisitesRequiredToBuild(const GameState & state, const ActionSetAbilities & wanted, ActionSetAbilities & requiredToBuild);
    BuildOrder  GetOptimizedNaiveBuildOrderOld(const GameState & state, const BuildOrderSearchGoal & goal);
    BuildOrder  GetNaiveBuildOrderAddWorkersOld(const GameState & state, const BuildOrderSearchGoal & goal, int maxWorkers);
    int         GetBuildOrderCompletionTime(const GameState & state, const BuildOrder & buildOrder);
    void        DoBuildOrder(GameState & state, const BuildOrder & buildOrder);
    void        DoBuildOrder(GameState & state, BuildOrderAbilities & buildOrder);
}
}
