/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch_Integral_DFS.h"
#include "FileTools.h"

using namespace BOSS;

CombatSearch_Integral_DFS::CombatSearch_Integral_DFS(const CombatSearchParameters p)
    : m_highestValueFound(0)
    , m_filesWritten(0)
    , m_statesWritten(0)
    , m_highestValueThusFar(0)
{
    m_params = p;

    m_ssHighestValue << "0,0\n";
    m_ssStates.precision(4);

    //BOSS_ASSERT(m_params.getInitialState().getRace() != Races::None, "Combat search initial state is invalid");
}

void CombatSearch_Integral_DFS::run(const GameState & state, int depth)
{
    m_highestValueFound = recurse(state, depth);
    m_results.buildOrder = m_integral.getBestBuildOrder();
}

FracType CombatSearch_Integral_DFS::recurse(const GameState & state, int depth)
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

    ActionSet legalActions;
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

        FracType actionValue = recurse(child, depth + 1);
        ActionValue av;
        av.action = action;
        av.evaluation = actionValue;
        actionValues.push_back(av);
        nodeIntegralValue = std::max(nodeIntegralValue, actionValue);

        m_buildOrder.pop_back();
        m_integral.popFinishedLastOrder(state, child);
    }

    if (nodeIntegralValue > m_highestValueThusFar)
    {
        m_highestValueThusFar = nodeIntegralValue;
        m_ssHighestValue << m_results.nodesExpanded << "," << m_highestValueThusFar << "\n";
    }

    if (isLeafNode)
    {
        m_results.leafNodesExpanded++;
    }

    //std::cout << "Value to this point: " << nodeIntegralToThisPoint << ". Total value: " << nodeIntegralValue << std::endl;
    //std::cout << nodeIntegralValue << std::endl;

    return nodeIntegralValue;
}

#include "BuildOrderPlotter.h"
void CombatSearch_Integral_DFS::writeResultsFile(const std::string & dir, const std::string & filename)
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