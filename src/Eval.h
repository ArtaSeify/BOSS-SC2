/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "BuildOrder.h"
#include "CombatSearchParameters.h"

namespace BOSS
{
    namespace Eval
    {
        static FracType GASWORTH = 1.0;
        static std::string UnitWeightsString;

        //double ArmyCompletedResourceSum(const GameState & state);
        FracType ArmyInProgressResourceSum(const GameState & state);
        FracType ArmyTotalResourceSum(const GameState & state);
        FracType UnitValue(const GameState & state, ActionType type);

        FracType UnitValueWithOpponent(const GameState & state, ActionType type, const CombatSearchParameters & params);

        void CalculateUnitValues(const GameState & state);
        std::vector<FracType> CalculateUnitWeightVector(const GameState& state, const std::vector<int> & enemyUnits);
        const std::vector<FracType> & GetUnitWeightVector();
        void SetUnitWeightVector(const std::vector<FracType> & weights);
        const std::vector<FracType>& GetUnitValuesVector();

        void setUnitWeightsString();
        std::string getUnitWeightsString();

        bool BuildOrderBetter(const BuildOrder & buildOrder, const BuildOrder & compareTo);
        bool StateBetter(const GameState & state, const GameState & compareTo);

        bool StateDominates(const GameState & state, const GameState & other);
    }
}
