#include "NMCS.h"

using namespace BOSS;

NMCS::NMCS(const CombatSearchParameters p)
{
    m_params = p;
}

NMCS::NMCS(const CombatSearchParameters p, const std::string & dir ,
    const std::string & prefix, const std::string & name)
{
    m_params = p;

    m_dir = dir;
    m_prefix = prefix;
    m_name = name;
}

void NMCS::recurse(const GameState & state, int depth)
{
    std::shared_ptr<Node> root = std::make_shared<Node>(state);

    auto searchResult = executeSearch(*root, m_params.getLevel(), depth);

    BuildOrderAbilities finishedUnitsBuildOrder = createFinishedUnitsBuildOrder(searchResult.first);

    m_bestIntegralFound = searchResult.first;
    m_bestBuildOrderFound = searchResult.second;
    m_results.highestEval = searchResult.first.getCurrentStackValue();
    m_results.buildOrder = searchResult.second;
    m_results.finishedUnitsBuildOrder = finishedUnitsBuildOrder;
}

std::pair<CombatSearch_IntegralDataFinishedUnits, BuildOrderAbilities> NMCS::executeSearch(Node node, int level, int depth)
{
    if (level < 0)
    {
        std::cerr << "level less than 0???" << std::endl;
    }
    if (level == 0)
    {
        return randomPlayout(node);
    }

    CombatSearch_IntegralDataFinishedUnits globalBestIntegral;
    BuildOrderAbilities globalBestBuildOrder;

    while (!timeLimitReached() && !isTerminalNode(node))
    {
        CombatSearch_IntegralDataFinishedUnits currentBestIntegral;
        BuildOrderAbilities currentBestBuildOrder;
        Action currentBestAction;

        ActionSetAbilities legalActions;
        generateLegalActions(node.getState(), legalActions, m_params);
        //getChronoBoostTargets(node, legalActions);

        if (legalActions.size() == 0)
        {
            node.setTerminal();
            GameState stateCopy(node.getState());
            stateCopy.fastForward(m_params.getFrameTimeLimit());
            m_integral.update(stateCopy, globalBestBuildOrder, m_params, m_searchTimer, false);

            if (m_integral.getCurrentStackValue() > globalBestIntegral.getCurrentStackValue()
                || (m_integral.getCurrentStackValue() == globalBestIntegral.getCurrentStackValue() && Eval::StateBetter(m_integral.getState(), globalBestIntegral.getState())))
            {
                globalBestIntegral = m_integral;
            }
            return std::make_pair(globalBestIntegral, globalBestBuildOrder);
        }

        for (int actionIndex = 0; actionIndex < legalActions.size(); ++actionIndex)
        {
            if (timeLimitReached())
            {
                break;
            }

            Node nextNode(node);
            Action action = legalActions[actionIndex];

            CombatSearch_IntegralDataFinishedUnits prevIntegral = m_integral;
            BuildOrderAbilities prevBO = m_buildOrder;

            nextNode.doAction(action, m_params);
            updateBOIntegral(nextNode, ActionAbilityPair(action.first, nextNode.getState().getLastAbility()), node.getState(), true);
            updateNodeVisits(true, nextNode.isTerminal());

            auto integralBOPair = executeSearch(nextNode, level - 1, depth + 1);

            m_integral = prevIntegral;
            m_buildOrder = prevBO;                  

            if (integralBOPair.first.getCurrentStackValue() > currentBestIntegral.getCurrentStackValue()
                || (integralBOPair.first.getCurrentStackValue() == currentBestIntegral.getCurrentStackValue() && Eval::StateBetter(integralBOPair.first.getState(), currentBestIntegral.getState())))
            {
                currentBestIntegral = integralBOPair.first;
                currentBestBuildOrder = integralBOPair.second;
                currentBestAction = action;
            }
        }

        GameState prevState(node.getState());
        if (currentBestIntegral.getCurrentStackValue() >= globalBestIntegral.getCurrentStackValue())
        {
            //visitedEdges.push_back(currentBestEdge);
            globalBestIntegral = currentBestIntegral;
            globalBestBuildOrder = currentBestBuildOrder;
            
            node.doAction(currentBestAction, m_params);
            updateBOIntegral(node, ActionAbilityPair(currentBestAction.first, node.getState().getLastAbility()), prevState, true);

            //std::cout << "took best found action: " << currentBestAction.first.getName() << ", " << currentBestAction.second << std::endl;
        }
        else
        {
            ActionAbilityPair actionToTake = std::make_pair(globalBestBuildOrder[depth].first, globalBestBuildOrder[depth].second);
            node.doAction(std::make_pair(actionToTake.first, actionToTake.second.targetID), m_params);
            updateBOIntegral(node, actionToTake, prevState, true);

            //std::cout << "took previous found action: " << actionToTake.first.getName() << ", " << actionToTake.second.targetID << std::endl;
        }
        updateNodeVisits(false, node.isTerminal());
        depth++;
    }

    return std::make_pair(globalBestIntegral, globalBestBuildOrder);
}

std::pair<CombatSearch_IntegralDataFinishedUnits, BuildOrderAbilities> NMCS::randomPlayout(Node node)
{
    CombatSearch_IntegralDataFinishedUnits bestIntegralFound;
    BuildOrderAbilities bestBuildOrderFound;

    for (int playout = 0; playout < m_params.getNumPlayouts(); ++playout)
    {
        m_promisingNodeIntegral = m_integral;
        m_promisingNodeBuildOrder = m_buildOrder;

        CombatSearch_IntegralMCTS::randomPlayout(node);
        m_numSimulations++;

        if (m_promisingNodeIntegral.getCurrentStackValue() >= bestIntegralFound.getCurrentStackValue())
        {
            bestIntegralFound = m_promisingNodeIntegral;
            bestBuildOrderFound = m_promisingNodeBuildOrder;
        }

        if (timeLimitReached())
        {
            break;
        }
    }

    return std::make_pair(bestIntegralFound, bestBuildOrderFound);
}

//std::pair<CombatSearch_IntegralDataFinishedUnits, BuildOrderAbilities> NMCS::updateBOIntegral(const Node & node, const ActionAbilityPair & action, const GameState & prevGameState, bool permanantUpdate)
//{
//    CombatSearch_IntegralDataFinishedUnits prevIntegral = m_promisingNodeIntegral;
//    BuildOrderAbilities prevBO = m_promisingNodeBuildOrder;
//
//    CombatSearch_IntegralMCTS::updateBOIntegral(node, action, prevGameState, permanantUpdate);
//
//    std::pair<CombatSearch_IntegralDataFinishedUnits, BuildOrderAbilities> returnValue = std::make_pair(m_promisingNodeIntegral, m_promisingNodeBuildOrder);
//
//    m_promisingNodeIntegral = prevIntegral;
//    m_promisingNodeBuildOrder = prevBO;
//
//    return returnValue;
//}

bool NMCS::timeLimitReached()
{
    return m_searchTimer.getElapsedTimeInMilliSec() > m_params.getSearchTimeLimit() - 50;
}

void NMCS::printResults()
{
    m_bestIntegralFound.print(m_bestBuildOrderFound);
    std::cout << "\nRan " << m_numSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000 * m_numSimulations / m_results.timeElapsed) << " simulations/sec\n";
    std::cout << "Nodes expanded: " << m_results.nodesExpanded << ". Total nodes visited: " << m_results.nodeVisits << ", at a rate of " << (1000 * m_results.nodeVisits / m_results.timeElapsed) << " nodes/sec\n";
}

#include "BuildOrderPlotter.h"
void NMCS::writeResultsFile(const std::string & dir, const std::string & filename)
{
    BuildOrderPlotter plot;
    plot.setOutputDir(dir);
    plot.addPlot(filename, m_params.getInitialState(), m_bestBuildOrderFound);
    plot.doPlots();

    m_bestIntegralFound.writeToFile(dir, filename);

    std::ofstream boFile(dir + "/" + filename + "_BuildOrder.txt", std::ofstream::out | std::ofstream::app);
    boFile << "\nRan " << m_numSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000 * m_numSimulations / m_results.timeElapsed) << " simulations/sec" << std::endl;
    boFile << "Nodes expanded: " << m_results.nodesExpanded << ". Total nodes visited: " << m_results.nodeVisits << ", at a rate of " << (1000 * m_results.nodeVisits / m_results.timeElapsed) << " nodes/sec\n";

    // Write search data to file 
    std::ofstream file(m_dir + "/" + m_name + "_Results.csv", std::ofstream::out | std::ofstream::app);
    file << m_resultsStream.rdbuf();
    file.close();
    m_resultsStream.str(std::string());


    std::ofstream searchData(m_dir + "/" + m_name + "_SearchData.txt", std::ofstream::out | std::ofstream::app);
    searchData << "Max value found: " << m_bestIntegralFound.getCurrentStackValue() << "\n";
    searchData << "Best build order (all): " << m_bestBuildOrderFound.getNameString() << std::endl;
    searchData << "Best build order (finished): " << m_results.finishedUnitsBuildOrder.getNameString(0, -1, true) << std::endl;
    searchData << "Nodes expanded: " << m_results.nodesExpanded << "\n";
    searchData << "Nodes traversed: " << m_results.nodeVisits << "\n";
    searchData << "Leaf nodes expanded: " << m_results.leafNodesExpanded << "\n";
    searchData << "Leaf nodes traversed: " << m_results.leafNodesVisited << "\n";
    searchData << "Search time in ms: " << m_results.timeElapsed << "\n";
    searchData << "Simulations: " << m_numSimulations;
    searchData.close();
}