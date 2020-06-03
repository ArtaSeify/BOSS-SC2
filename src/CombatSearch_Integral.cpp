#include "CombatSearch_Integral.h"

using namespace BOSS;

void CombatSearch_Integral::printResults()
{
    m_integral.print();
    std::cout << "\nSearched " << m_results.nodesExpanded << " nodes in " << m_results.timeElapsed << "ms @ " << (1000.0 * m_results.nodesExpanded / m_results.timeElapsed) << " nodes/sec\n\n";
}

BuildOrder CombatSearch_Integral::createFinishedUnitsBuildOrder(const BuildOrder& buildOrder) const
{
    BuildOrder finishedBuildOrder;
    GameState state(m_params.getInitialState());
    NumUnits numInitialUnits = state.getNumUnits();

    std::map<int, int> unitBuildOrderMap;
    int offset = 0;
    int buildOrderActions = 0;
    for (auto& action : buildOrder)
    {
        ActionType type = action.first;
        if (type.isAbility())
        {
            state.doAbility(type, action.second.targetID);
        }
        else
        {
            state.doAction(type);
        }

        if (type.isAbility())
        {
            unitBuildOrderMap[buildOrderActions] = -1;
        }

        while (!type.isAbility())
        {
            const int index = numInitialUnits + offset;
            const Unit unit = static_cast<const GameState>(state).getUnit(index);

            /*std::cout << "index: " << index << std::endl;
            std::cout << "unit type: " << unit.getType().getID() << std::endl;
            std::cout << "build order type: " << type.getID() << std::endl;*/
            if (unit.getType() == type)
            {
                unitBuildOrderMap[buildOrderActions] = index;
                ++offset;
                break;
            }
            //std::cout << "skipping!" << std::endl;
            ++offset;
        }
        ++buildOrderActions;
    }
    state.fastForward(m_params.getFrameLimit());

    for (auto it = unitBuildOrderMap.begin(); it != unitBuildOrderMap.end(); ++it)
    {
        int buildOrderIndex = it->first;
        int stateUnitIndex = it->second;

        if (stateUnitIndex == -1)
        {
            if (static_cast<const GameState>(state).getUnit(buildOrder[buildOrderIndex].second.targetProductionID).getTimeUntilBuilt() > 0)
            {
                break;
            }
        }

        else if (static_cast<const GameState>(state).getUnit(stateUnitIndex).getTimeUntilBuilt() > 0)
        {
            break;
        }

        finishedBuildOrder.add(buildOrder[buildOrderIndex]);
    }

    return finishedBuildOrder;
}

BuildOrder CombatSearch_Integral::createUsefulBuildOrder(const BuildOrder& buildOrder) const
{
    // map from index of unit in state to index in build order that built it
    std::map<int, int> unitBuildOrderMap;

    GameState state(m_params.getInitialState());
    int numInitialUnits = state.getNumUnits();
    int offset = 0;
    int buildOrderActions = 0;
    for (auto& action : buildOrder)
    {
        ActionType type = action.first;
        if (type.isAbility())
        {
            state.doAbility(type, action.second.targetID);
        }
        else
        {
            state.doAction(type);
        }

        if (type.isAbility())
        {
            unitBuildOrderMap[buildOrderActions] = -1;
        }

        while (!type.isAbility())
        {
            const int index = numInitialUnits + offset;
            const Unit unit = static_cast<const GameState>(state).getUnit(index);

            /*std::cout << "index: " << index << std::endl;
            std::cout << "unit type: " << unit.getType().getID() << std::endl;
            std::cout << "build order type: " << type.getID() << std::endl;*/
            if (unit.getType() == type)
            {
                unitBuildOrderMap[buildOrderActions] = index;
                ++offset;
                break;
            }
            //std::cout << "skipping!" << std::endl;
            ++offset;
        }
        ++buildOrderActions;
    }

    BuildOrder usefulBuildOrder;
    auto unitTypes = m_params.getInitialState().getUnitTypes();
    for (auto it = unitBuildOrderMap.begin(); it != unitBuildOrderMap.end(); ++it)
    {
        //std::cout << "start of loop" << std::endl;
        int buildOrderIndex = it->first;
        int stateIndex = it->second;

        // chronoboost
        if (stateIndex == -1)
        {
            usefulBuildOrder.add(buildOrder[buildOrderIndex]);
            continue;
        }

        const Unit unit = static_cast<const GameState>(state).getUnit(stateIndex);
        ActionType unitType = unit.getType();

        // warpgates are linked to Gateways, so we just ignore them and only consider gateways
        BOSS_ASSERT(unitType != ActionTypes::GetActionType("WarpGate"), "Found WarpGate index in mapping.");

        // These actions are always useful
        if (unitType.isWorker() || unitType.isDepot() || unitType.isRefinery() || unitType.isSupplyProvider() || Eval::UnitValue(state, unitType) > 0)
        {
            usefulBuildOrder.add(buildOrder[buildOrderIndex]);
            continue;
        }

        // this unit did not produce anything and is not a prerequisite for any created unit,
        // so it's a "useless" action.
        // if it's a prerequisite but we have more than 1, we can remove the extra ones
        if (unit.getType().isBuilding())
        {
            // if the unit built something it is useful
            bool unitUseful = false;
            for (auto it2 = unitBuildOrderMap.begin(); it2 != unitBuildOrderMap.end(); ++it2)
            {
                if (it2->second == -1)
                {
                    continue;
                }
                const Unit unitCheck = static_cast<const GameState>(state).getUnit(it2->second);
                if (unitCheck.getID() != unit.getID() && unitCheck.getBuilderID() == unit.getID())
                {
                    usefulBuildOrder.add(buildOrder[buildOrderIndex]);
                    unitUseful = true;
                    break;
                }
            }

            if (unitUseful)
            {
                continue;
            }

            // morphed building (Gateway)
            if (unit.getMorphID() != -1)
            {
                const Unit u = static_cast<const GameState>(state).getUnit(unit.getMorphID());
                bool unitUseful = false;

                for (auto it2 = unitBuildOrderMap.begin(); it2 != unitBuildOrderMap.end(); ++it2)
                {
                    if (it2->second == -1)
                    {
                        continue;
                    }
                    const Unit unitCheck = static_cast<const GameState>(state).getUnit(it2->second);
                    if (unitCheck.getID() != u.getID() && unitCheck.getBuilderID() == u.getID())
                    {
                        usefulBuildOrder.add(buildOrder[buildOrderIndex]);
                        unitUseful = true;
                        break;
                    }
                }

                if (unitUseful)
                {
                    continue;
                }
            }

            // even if it's a prereq, if we have more than 1 it's still useless
            if (unitTypes[unitType.getRaceActionID()].size() == 1)
            {
                //std::cout << "breaking because it's a prereq building that we have more than 1 off!" << std::endl;
                break;
                //std::cout << "removing: " << unitType.getName() << " because it's a building that we have more than 1 off" << std::endl;
            }
            // if we only have one, we need to see if it's a prereq for anything
            else
            {
                bool isUseful = false;

                for (int i = unit.getID() + 1; i < state.getNumUnits(); ++i)
                {
                    const Unit u = static_cast<const GameState>(state).getUnit(i);
                    for (auto& req : u.getType().required())
                    {
                        // this unit is a prerequisite for something we have built, so it's "useful"
                        if (req == unitType)
                        {
                            isUseful = true;
                            break;
                        }
                    }

                    if (isUseful)
                    {
                        usefulBuildOrder.add(buildOrder[buildOrderIndex]);
                        unitTypes[unitType.getRaceActionID()].push_back(0);
                        break;
                    }
                }

                if (!isUseful)
                {
                    //std::cout << "breaking because building is useless" << std::endl;
                    break;
                    //std::cout << "removing: " << unitType.getName() << " because isUseful is false" << std::endl;
                }
            }
        }
        else
        {
            BOSS_ASSERT(false, "unit not considered in usefulBuildOrder %s", unitType.getName().c_str());
        }
    }

    GameState t(m_params.getInitialState());
    for (auto& action : usefulBuildOrder)
    {
        if (action.first.isAbility())
        {
            t.doAbility(action.first, action.second.targetID);
        }
        else
        {
            t.doAction(action.first);
        }
    }

    return usefulBuildOrder;
}