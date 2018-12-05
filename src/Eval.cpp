/* -*- c-basic-offset: 4 -*- */

#include "Eval.h"

namespace BOSS
{
namespace Eval
{
    /*double ArmyCompletedResourceSum(const GameState & state)
    {
        double sum(0);
        
        for (auto & type : ActionTypes::GetAllActionTypes())
        {
            if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
            {
                sum += state.getNumCompleted(type)*type.mineralPrice();
                sum += 2*state.getNumCompleted(type)*type.gasPrice();
            }
        }
        
        return sum;
    }*/

    /*double ArmyCompletedResourceSum(const GameState & state)
    {
        double sum(0);

        auto & army_units = state.getFinishedArmyUnits();
        for (size_t i(0); i < army_units.size(); ++i)
        {
            size_t unit_index = army_units[i];
            sum += state.getUnit(unit_index).getType().mineralPrice();
            sum += 2 * state.getUnit(unit_index).getType().gasPrice();
        }

        return sum;
    }*/

    FracType ArmyTotalResourceSum(const GameState & state)
    {
        FracType sum(0);
        
        for (int unitIndex = 0; unitIndex < state.getNumUnits(); ++unitIndex)
        {
            auto & type = state.getUnit(unitIndex).getType();

            if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
            {
                sum += type.mineralPrice();
                sum += 2 * type.gasPrice();
            }
        }

        return sum;
    }

    FracType ArmyResourceUnit(const GameState & state, int unitIndex)
    {
        FracType sum = 0;

        ActionType type = state.getUnit(unitIndex).getType();
        if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
        {
            sum += type.mineralPrice();
            sum += 2 * type.gasPrice();
        }

        return sum;
    }

    bool BuildOrderBetter(const BuildOrderAbilities & buildOrder, const BuildOrderAbilities & compareTo)
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
    bool StateBetter(const GameState & state, const GameState & compareTo)
    {
        if (compareTo.getNumMineralWorkers() == 0)
        {
            return true;
        }

        int numWorkers = state.getNumTotalWorkers();
        int numWorkersOther = compareTo.getNumTotalWorkers();

        if (numWorkers == numWorkersOther)
        {
            FracType armyVal = ArmyTotalResourceSum(state);
            FracType armyValOther = ArmyTotalResourceSum(compareTo);

            if (armyVal == armyValOther)
            {
                return state.getNumUnits() < compareTo.getNumUnits();
            }
            
            else
            {
                return armyVal > armyValOther;
            }
            
        }
        else
        {
            return numWorkers > numWorkersOther;
        }
    }

    bool StateDominates(const GameState & state, const GameState & other)
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
}
}

