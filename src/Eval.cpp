/* -*- c-basic-offset: 4 -*- */

#include "Eval.h"

using namespace BOSS;

std::vector<FracType> UnitWeights;

FracType Eval::ArmyInProgressResourceSum(const GameState & state)
{
    FracType sum(0);

    for (auto unitIndex : state.getUnitsBeingBuilt())
    {
        auto type = state.getUnit(unitIndex).getType();

        if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
        {
            sum += type.mineralPrice();
            sum += FracType(GASWORTH * type.gasPrice());
        }
    }

    return sum / 100;
}

FracType Eval::ArmyTotalResourceSum(const GameState & state)
{
    FracType sum(0);
        
    for (int unitIndex = 0; unitIndex < state.getNumUnits(); ++unitIndex)
    {
        auto type = state.getUnit(unitIndex).getType();

        if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
        {
            sum += type.mineralPrice();
            sum += FracType(GASWORTH * type.gasPrice());
        }
    }

    return sum / 100;
}

FracType Eval::UnitValue(const GameState & state, ActionType type)
{
    FracType sum = 0;

    if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
    {
        sum += type.mineralPrice();
        sum += FracType(GASWORTH * type.gasPrice());
    }

    return sum / 100;
}

std::vector<FracType> Eval::CalculateUnitWeightVector(const GameState & state, const std::vector<int> & enemyUnits)
{
    std::vector<FracType> weights = std::vector<FracType>(ActionTypes::GetRaceActionCount(state.getRace()), 0);
    std::vector<int> enemyCounters = std::vector<int>(ActionTypes::GetRaceActionCount(state.getRace()), 0);
    std::vector<int> enemyIsCountered = std::vector<int>(ActionTypes::GetRaceActionCount(state.getRace()), 0);

    for (int index = 0; index < enemyUnits.size(); ++index)
    {
        int numEnemyUnit = enemyUnits[index];

        if (numEnemyUnit == 0)
        {
            continue;
        }

        for (const auto& strongAgainst : ActionTypes::GetActionType(index).strongAgainst(state.getRace()))
        {
            BOSS_ASSERT(strongAgainst.getRace() == state.getRace(), "Wrong race for unit!");
            if (strongAgainst.getID() == ActionTypes::GetWorker(state.getRace()).getID())
            {
                continue;
            }

            enemyCounters[strongAgainst.getRaceActionID()]++;
        }

        for (const auto& weakAgainst : ActionTypes::GetActionType(index).weakAgainst(state.getRace()))
        {
            BOSS_ASSERT(weakAgainst.getRace() == state.getRace(), "Wrong race for unit!");
            enemyIsCountered[weakAgainst.getRaceActionID()]++;
        }
    }

    for (int index = 0; index < enemyCounters.size(); ++index)
    {
        //weights[index] = FracType((1.0 + enemyIsCountered[index]) / (1.0 + enemyCounters[index]));
        weights[index] = FracType(enemyIsCountered[index] - enemyCounters[index]);
    }

    return weights;
}

const std::vector<FracType> & Eval::GetUnitWeightVector()
{
    return UnitWeights;
}

void Eval::SetUnitWeightVector(const std::vector<FracType> & weights)
{
    UnitWeights = weights;
    for (int index = 0; index < UnitWeights.size(); ++index)
    {
        std::cout << ActionTypes::GetActionType(index + 1).getName() << ": " << weights[index] << std::endl;
    }
}

FracType Eval::UnitValueWithOpponent(const GameState & state, ActionType type, const CombatSearchParameters & params)
{
    FracType sum = 0;

    if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
    {
        sum += type.mineralPrice();
        sum += FracType(GASWORTH * type.gasPrice());

        //sum *= Eval::UnitWeight(state, type, params);
        sum += UnitWeights[type.getRaceActionID()];
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

