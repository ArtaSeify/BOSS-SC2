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

    double ArmyCompletedResourceSum(const GameState & state)
    {
        double sum(0);

        auto & army_units = state.getFinishedArmyUnits();
        for (size_t unit_index : army_units)
        {
            sum += state.getUnit(unit_index).getType().mineralPrice();
            sum += 2 * state.getUnit(unit_index).getType().gasPrice();
        }

        return sum;
    }

    double ArmyTotalResourceSum(const GameState & state)
    {
        double sum(0);
	    
	    for (auto & type : ActionTypes::GetAllActionTypes())
	    {
	        if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
	        {
                sum += state.getNumTotal(type)*type.mineralPrice();
	            sum += 2*state.getNumTotal(type)*type.gasPrice();
	        }
	    }
	    
	    return sum;
    }

    double ArmyResourceSumToIndex(const GameState & state, size_t finishedUnitsIndex)
    {
        double sum(0);
        auto & finishedUnits = state.getFinishedUnits();
        size_t index = (finishedUnitsIndex == 0) ? 0 : finishedUnitsIndex;

        for (index; index < finishedUnitsIndex + 1; ++index)
        {
            ActionType type = state.getUnit(finishedUnits[index]).getType();
            if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
            {
                sum += type.mineralPrice();
                sum += 2 * type.gasPrice();
            }
        }

        return sum;
    }

    bool BuildOrderBetter(const BuildOrder & buildOrder, const BuildOrder & compareTo)
    {
        size_t numWorkers = 0;
        size_t numWorkersOther = 0;

        for (size_t a(0); a<buildOrder.size(); ++a)
        {
            if (buildOrder[a].isWorker())
            {
                numWorkers++;
            }
        }

        for (size_t a(0); a<compareTo.size(); ++a)
        {
            if (compareTo[a].isWorker())
            {
                numWorkersOther++;
            }
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

    bool StateBetter(const GameState & state, const GameState & compareTo)
    {
        size_t numWorkers = state.getNumTotalCompleted(ActionTypes::GetWorker(state.getRace()));
        size_t numWorkersOther = compareTo.getNumTotalCompleted(ActionTypes::GetWorker(compareTo.getRace()));

        if (numWorkers == numWorkersOther)
        {
            return (state.getMinerals()) > (compareTo.getMinerals());
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

