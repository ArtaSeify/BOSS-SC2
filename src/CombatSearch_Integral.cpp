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

    //BOSS_ASSERT(m_params.getInitialState().getRace() != Races::None, "Combat search initial state is invalid");
}

CombatSearch_Integral::~CombatSearch_Integral()
{
    FileTools::MakeDirectory(CONSTANTS::ExecutablePath + "/SavedStates");
    //std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + "_" + std::to_string(m_filesWritten) + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
    std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
    fileStates << m_ssStates.rdbuf();
    m_ssStates.str(std::string());

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

            state.writeToSS(m_ssStates, m_params);

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