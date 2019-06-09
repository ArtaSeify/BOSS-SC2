/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch_Integral.h"
#include "FileTools.h"

using namespace BOSS;

FracType CombatSearch_Integral::highestValueThusFar = 0;

CombatSearch_Integral::CombatSearch_Integral(const CombatSearchParameters p,
    const std::string & dir, const std::string & prefix, const std::string & name)
    : m_highestValueFound(0)
    , m_filesWritten(0)
    , m_statesWritten(0)
{
    m_params = p;

    m_dir = dir;
    m_prefix = prefix;
    m_name = name;
    m_ssHighestValue << "0,0\n";
    m_ssStates.precision(4);

    //BOSS_ASSERT(m_params.getInitialState().getRace() != Races::None, "Combat search initial state is invalid");
}

CombatSearch_Integral::~CombatSearch_Integral()
{
    if (m_params.getSaveStates())
    {
        FileTools::MakeDirectory(CONSTANTS::ExecutablePath + "/SavedStates");
        //std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + "_" + std::to_string(m_filesWritten) + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
        std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
        fileStates << m_ssStates.rdbuf();
        m_ssStates.str(std::string());
    }

    std::ofstream fileHighestValue(m_dir + "/" + m_prefix + "_HighestValueOrdering.txt", std::ofstream::out | std::ofstream::trunc);
    fileHighestValue << m_ssHighestValue.rdbuf();
    fileHighestValue.close();
    m_ssHighestValue.str(std::string());
}

void CombatSearch_Integral::recurse(const GameState & state, int depth)
{
    m_highestValueFound = recurseReturnValue(state, depth);
    m_results.buildOrder = m_integral.getBestBuildOrder();
}

FracType CombatSearch_Integral::recurseReturnValue(const GameState & state, int depth)
{
    if (timeLimitReached())
    {
        throw BOSS_COMBATSEARCH_TIMEOUT;
    }

    updateResults(state);

    FracType nodeIntegralToThisPoint = m_integral.getCurrentStackValue();
    FracType nodeIntegralValue = nodeIntegralToThisPoint;
    std::vector<ActionValue> actionValues;
    bool isLeafNode = true;

    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);

    for (int a(0); a < legalActions.size(); ++a)
    {
        const int index = a;
        GameState child(state);

        auto action = legalActions[index];

        if (action.first.isAbility())
        {
            child.doAbility(action.first, action.second);
            m_buildOrder.add(action.first, child.getLastAbility());
        }
        else
        {
            child.doAction(action.first);
            m_buildOrder.add(action.first);
        }

        //std::cout << "action added: " << action.getName() << std::endl;
        //std::cout << "target of action added: " << actionTarget << std::endl;
        //std::cout << "frame of action added: " << child.getCurrentFrame() << std::endl;

        m_integral.update(child, m_buildOrder, m_params, m_searchTimer, true);
        isLeafNode = false;

        FracType actionValue = recurseReturnValue(child, depth + 1);
        ActionValue av;
        av.action = action;
        av.evaluation = actionValue;
        actionValues.push_back(av);
        nodeIntegralValue = std::max(nodeIntegralValue, actionValue);

        m_buildOrder.pop_back();
        m_integral.popFinishedLastOrder(state, child);
    }

    if (m_params.getSaveStates())
    {
        if (nodeIntegralValue - nodeIntegralToThisPoint > 0)
        {
            ActionValue bestAction;
            bestAction.evaluation = -1;
            for (auto & av : actionValues)
            {
                if (av.evaluation >= bestAction.evaluation)
                {
                    bestAction = av;
                }
            }

            std::vector<ActionValue> tiedActionValues;
            for (auto & av : actionValues)
            {
                if (av.evaluation == bestAction.evaluation)
                {
                    tiedActionValues.push_back(av);
                }
            }

            state.writeToSS(m_ssStates, m_params, std::vector<int>());

            m_ssStates << ",";
            for (int index = 0; index < tiedActionValues.size(); ++index)
            {
                m_ssStates << tiedActionValues[index].action.first.getID() << "," << tiedActionValues[index].evaluation - nodeIntegralToThisPoint << ",";
            }
            m_ssStates << nodeIntegralValue - nodeIntegralToThisPoint << "\n";

            //std::cout << m_ssStates.str() << std::endl;
            //json stateValuePair;
            //stateValuePair["State"] = state.writeToJson(m_params);
            //stateValuePair["Value"] = nodeIntegralValue - nodeIntegralToThisPoint;
            //std::vector<std::uint8_t> v_msgpack = json::to_msgpack(stateValuePair);
            //m_jStates.insert(m_jStates.end(), v_msgpack.begin(), v_msgpack.end());
            m_statesWritten++;

            if (m_statesWritten % 1000000 == 0)
            {
                FileTools::MakeDirectory(CONSTANTS::ExecutablePath + "/SavedStates");
                //std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + "_" + std::to_string(m_filesWritten) + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
                std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
                fileStates << m_ssStates.rdbuf();
                m_ssStates.str(std::string());
                m_ssStates.clear();
                //fileStates.write(reinterpret_cast<const char*>(m_jStates.data()), m_jStates.size());
                //m_jStates.clear();
                m_filesWritten++;
            }
        }
    }

    if (nodeIntegralValue > highestValueThusFar)
    {
        highestValueThusFar = nodeIntegralValue;
        m_ssHighestValue << m_results.nodesExpanded << "," << highestValueThusFar << "\n";
    }

    if (isLeafNode)
    {
        m_results.leafNodesExpanded++;
    }

    //std::cout << "Value to this point: " << nodeIntegralToThisPoint << ". Total value: " << nodeIntegralValue << std::endl;
    //std::cout << nodeIntegralValue << std::endl;

    return nodeIntegralValue;
}

BuildOrderAbilities CombatSearch_Integral::createFinishedUnitsBuildOrder(const BuildOrderAbilities & buildOrder) const
{
    BuildOrderAbilities finishedBuildOrder;
    GameState state(m_params.getInitialState());
    NumUnits numInitialUnits = state.getNumUnits();

    std::map<int, int> unitBuildOrderMap;
    int offset = 0;
    int buildOrderActions = 0;
    for (auto & action : buildOrder)
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
    state.fastForward(m_params.getFrameTimeLimit());

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

BuildOrderAbilities CombatSearch_Integral::createUsefulBuildOrder(const BuildOrderAbilities & buildOrder) const
{
    // map from index of unit in state to index in build order that built it
    std::map<int, int> unitBuildOrderMap;

    GameState state(m_params.getInitialState());
    int numInitialUnits = state.getNumUnits();
    int offset = 0;
    int buildOrderActions = 0;
    for (auto & action : buildOrder)
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

    BuildOrderAbilities usefulBuildOrder;
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
                    for (auto & req : u.getType().required())
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
    for (auto & action : usefulBuildOrder)
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

void CombatSearch_Integral::printResults()
{
    m_integral.print();
    std::cout << "\nSearched " << m_results.nodesExpanded << " nodes in " << m_results.timeElapsed << "ms @ " << (1000.0*m_results.nodesExpanded / m_results.timeElapsed) << " nodes/sec\n\n";
}

#include "BuildOrderPlotter.h"
void CombatSearch_Integral::writeResultsFile(const std::string & dir, const std::string & filename)
{
    BuildOrderPlotter plot;
    plot.setOutputDir(dir);
    plot.addPlot(filename, m_params.getInitialState(), m_integral.getBestBuildOrder());
    plot.doPlots();

    m_integral.writeToFile(dir, filename);

    std::ofstream file(dir + "/" + filename + "_BuildOrder.txt", std::ofstream::out | std::ofstream::app);
    file << "\nSearched " << m_results.nodesExpanded << " nodes in " << m_results.timeElapsed << "ms @ " << (1000.0*m_results.nodesExpanded / m_results.timeElapsed) << " nodes/sec";
    file.close();

    std::ofstream searchData(dir + "/" + filename + "_SearchData.txt", std::ofstream::out | std::ofstream::app);
    searchData << "Max value found: " << m_highestValueFound << "\n";
    searchData << "Best build order: " << m_integral.getBestBuildOrder().getNameString() << std::endl;
    searchData << "Nodes expanded: " << m_results.nodesExpanded << "\n";
    searchData << "Nodes traversed: " << m_results.nodesExpanded << "\n";
    searchData << "Leaf nodes expanded: " << m_results.leafNodesExpanded << "\n";
    searchData << "Leaf nodes traversed: " << m_results.leafNodesExpanded << "\n";
    searchData << "Search time in ms: " << m_results.timeElapsed;
    searchData.close();
}