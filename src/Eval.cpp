/* -*- c-basic-offset: 4 -*- */

#include "Eval.h"

using namespace BOSS;

FracType Eval::ArmyInProgressResourceSum(const GameState & state)
{
    FracType sum(0);

    for (auto unitIndex : state.getUnitsBeingBuilt())
    {
        auto & type = state.getUnit(unitIndex).getType();

        if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider() && type != ActionTypes::GetDetector(state.getRace()))
        {
            sum += type.mineralPrice();
            sum += FracType(1.5 * type.gasPrice());
        }
    }

    return sum / 100;
}

FracType Eval::ArmyTotalResourceSum(const GameState & state)
{
    FracType sum(0);
        
    for (int unitIndex = 0; unitIndex < state.getNumUnits(); ++unitIndex)
    {
        auto & type = state.getUnit(unitIndex).getType();

        if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider() && type != ActionTypes::GetDetector(state.getRace()))
        {
            sum += type.mineralPrice();
            sum += FracType(1.5 * type.gasPrice());
        }
    }

    return sum / 100;
}

FracType Eval::ArmyResourceUnit(const GameState & state, int unitIndex)
{
    FracType sum = 0;

    ActionType type = state.getUnit(unitIndex).getType();
    if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider() && type != ActionTypes::GetDetector(state.getRace()))
    {
        sum += type.mineralPrice();
        sum += FracType(1.5 * type.gasPrice());
    }

    return sum / 100;
}

bool Eval::BuildOrderBetter(const BuildOrderAbilities & buildOrder, const BuildOrderAbilities & compareTo)
{
    int numWorkers = 0;
    int numWorkersOther = 0;

    for (const auto &x : buildOrder) {
        numWorkers += x.first.isWorker();
    }
        
    for (const auto &x : compareTo) {
        numWorkersOther += x.first.isWorker();
    }

    if (numWorkers == numWorkersOther)
    {
        return buildOrder.size() < compareTo.size();
    }
    else
    {
        return numWorkers > numWorkersOther;
    }
}

// condition 1: more workers
// condition 2: more army units in production
// condition 3: less units
bool Eval::StateBetter(const GameState & state, const GameState & compareTo)
{
    if (compareTo.getNumMineralWorkers() == 0)
    {
        return true;
    }

    int numWorkers = state.getNumTotalWorkers();
    int numWorkersOther = compareTo.getNumTotalWorkers();

    if (numWorkers == numWorkersOther)
    {
        FracType armyInProgress = ArmyInProgressResourceSum(state);
        FracType armyInProgressOther = ArmyInProgressResourceSum(compareTo);

        if (armyInProgress == armyInProgressOther)
        {
            return state.getNumUnits() < compareTo.getNumUnits();
        }
            
        else
        {
            return armyInProgress > armyInProgressOther;
        }
            
    }
    else
    {
        return numWorkers > numWorkersOther;
    }
}

bool Eval::StateDominates(const GameState & state, const GameState & other)
{
    // we can't define domination for different races
    if (state.getRace() != other.getRace())
    {
        return false;
    }

    // if we have less resources than the other state we are not dominating it
    if ((state.getMinerals() < other.getMinerals()) || (state.getGas() < other.getGas()))
    {
        return false;
    }

    // if we have less of any unit than the other state we are not dominating it
    for (auto & action : ActionTypes::GetAllActionTypes())
    {
        if (state.getNumTotal(action) < other.getNumTotal(action))
        {
            return false;
        }

        if (state.getNumCompleted(action) < other.getNumCompleted(action))
        {
            return false;
        }
    }

    return true;
}

