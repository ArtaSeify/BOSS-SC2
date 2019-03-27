#include "CombatSearch_IntegralMCTS.h"
#include "FileTools.h"
#include <random>

using namespace BOSS;

CombatSearch_IntegralMCTS::CombatSearch_IntegralMCTS(const CombatSearchParameters p, const std::string & dir, 
                                                        const std::string & prefix, const std::string & name)
    : m_exploration_parameter (p.getExplorationValue())
    , m_bestIntegralFound(CombatSearch_IntegralDataFinishedUnits())
    , m_bestBuildOrderFound(BuildOrderAbilities())
    , m_numSimulations(0)
{
    m_params = p;
    Edge::USE_MAX_VALUE = m_params.getUseMaxValue();

    m_simulationsPerStep = m_params.getSimulationsPerStep();
    m_writeEveryKSimulations = 1;
    m_dir = dir;
    m_name = prefix;
    m_resultsStream << "0,0,0,0,0,0, ,0\n";

    std::random_device rd; // obtain a random number from hardware
    m_rnggen.seed(rd());

    if (m_params.getSaveStates())
    {
        std::string dataDir = CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + ".csv";
        m_fileStates.open(dataDir, std::ofstream::out | std::ofstream::app);
    }

    if (m_params.useNetworkPrediction())
    {
        Edge::MIXING_PARAMETER = 0.5f;
    }
    else
    {
        Edge::MIXING_PARAMETER = 0.0f;
    }
}

CombatSearch_IntegralMCTS::~CombatSearch_IntegralMCTS()
{
    if (m_params.getSaveStates())
    {
        m_fileStates << m_dataStream.rdbuf();
        m_dataStream.str(std::string());
        m_fileStates.close();
    }
}

void CombatSearch_IntegralMCTS::recurse(const GameState & state, int depth)
{
    //test2(state);
    m_numSimulations = 0;
    int simulationsWritten = 0;

    std::shared_ptr<Node> root = std::make_shared<Node>(state);
    std::shared_ptr<Node> currentRoot = root;

    while (!timeLimitReached() && m_numSimulations < m_params.getNumberOfSimulations() && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        // change the root of the tree. Remove all the nodes and edges that are now irrelevant
        if (m_params.getChangingRoot() && m_numSimulations > 0 && (m_simulationsPerStep == 1 || m_numSimulations%m_simulationsPerStep == 0))
        {
            std::shared_ptr<Edge> childEdge = currentRoot->getHighestValueChild(m_params);

            // TODO: DONT CRASH, CREATE NODE INSTEAD
            BOSS_ASSERT(childEdge->getChild() != nullptr, "currentRoot has become null");
            
            // we have made a choice, so we need to update the integral and build order permanently
            updateBOIntegral(*(childEdge->getChild()), childEdge->getAction(), currentRoot->getState(), true);

            currentRoot->removeEdges(childEdge);
            currentRoot = childEdge->getChild();
            updateNodeVisits(false, isTerminalNode(*currentRoot));

            // search is over
            if (isTerminalNode(*currentRoot) || m_numSimulations >= m_params.getNumberOfSimulations())
            {
                break;
            }
        }
        /*if ((m_numSimulations % 1000) == 0)
        {
            std::cout << "have run : " << m_numSimulations << " simulations thus far." << std::endl;
        }*/
        auto nodePair = getPromisingNode(currentRoot);
        // we have reached node limit 
        if (m_results.nodeVisits >= m_params.getNumberOfNodes())
        {
            break;
        }
        std::shared_ptr<Node> promisingNode = nodePair.first;
        
        // a node that isn't part of the graph yet. We just simulate from this point
        if (nodePair.second)
        {
            randomPlayout(*promisingNode);
        }

        // the node is part of the graph, so we create its edges and pick one at random
        else
        {
            if (!isTerminalNode(*promisingNode))
            {
                ActionSetAbilities legalActions;
                generateLegalActions(promisingNode->getState(), legalActions, m_params);
                promisingNode->createChildrenEdges(legalActions, m_params);

                // there might be no action possible, so createChildrenEdges creates 0 edges
                if (isTerminalNode(*promisingNode))
                {
                    m_results.leafNodesExpanded++;
                    m_results.leafNodesVisited++;
                }
                else
                {
                    // get a child based on highest network value
                    if (m_params.useNetworkPrediction())
                    {
                        const GameState & prevNodeState = promisingNode->getState();
                        std::shared_ptr<Edge> action = promisingNode->getHighestValueChild(m_params);
                        promisingNode = promisingNode->notExpandedChild(action, m_params);
                        updateBOIntegral(*promisingNode, action->getAction(), prevNodeState, false);
                    }
                    // pick a child at random
                    else
                    {
                        const GameState & prevNodeState = promisingNode->getState();
                        std::shared_ptr<Edge> action = promisingNode->getRandomEdge();
                        promisingNode = promisingNode->notExpandedChild(action, m_params);
                        updateBOIntegral(*promisingNode, action->getAction(), prevNodeState, false);
                    }
                    
                    updateNodeVisits(Edge::NODE_VISITS_BEFORE_EXPAND == 1, isTerminalNode(*promisingNode));
                }
                randomPlayout(*promisingNode);
            }
        }

        if (m_results.nodeVisits < m_params.getNumberOfNodes())
        {
            backPropogation(promisingNode);

            m_numSimulations++;

            if (m_needToWriteBestValue)
            {
                writeResultsToFile(currentRoot);
            }
        }
    }

    m_integral = m_bestIntegralFound;
    m_buildOrder = m_bestBuildOrderFound;

    BuildOrderAbilities finishedUnitsBuildOrder = createFinishedUnitsBuildOrder(m_bestIntegralFound);

    m_results.highestEval = m_integral.getCurrentStackValue();
    m_results.buildOrder = m_bestBuildOrderFound;
    m_results.finishedUnitsBuildOrder = finishedUnitsBuildOrder;

    // write state data
    if (m_params.getSaveStates())
    {
        std::shared_ptr<Node> currentNode = root;

        while (currentNode != currentRoot)
        {
            currentNode->getState().writeToSS(m_dataStream, m_params);
            m_dataStream << "," << m_integral.getCurrentStackValue() << "\n";

            currentNode = currentNode->getHighestValueChild(m_params)->getChild();
        }
        currentNode->getState().writeToSS(m_dataStream, m_params);
        m_dataStream << "," << m_integral.getCurrentStackValue() << "\n";
    }

    root->cleanUp();
}

void CombatSearch_IntegralMCTS::test(const GameState & state)
{
    Node root(state);
    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);
    root.createChildrenEdges(legalActions, m_params);
    root.printChildren();
    std::cout << std::endl;
    root.doAction(root.getChild(ActionTypes::GetActionType("Probe")), m_params);
    root.printChildren();
    std::cout << std::endl;

    Node & secondLevel = root.getChild(0);
    legalActions.clear();
    generateLegalActions(secondLevel.getState(), legalActions, m_params);
    secondLevel.createChildrenEdges(legalActions, m_params);
    secondLevel.printChildren();
    std::cout << std::endl;
    secondLevel.doAction(secondLevel.getChild(ActionTypes::GetActionType("Pylon")), m_params);
    secondLevel.printChildren();
    std::cout << std::endl;

    Node & thirdLevel = secondLevel.getChild(0);
    legalActions.clear();
    generateLegalActions(thirdLevel.getState(), legalActions, m_params);
    thirdLevel.createChildrenEdges(legalActions, m_params);
    thirdLevel.printChildren();
}

void CombatSearch_IntegralMCTS::test2(const GameState & state)
{
    std::shared_ptr<Node> root = std::make_shared<Node>(state);

    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);
    root->createChildrenEdges(legalActions, m_params);
    root->printChildren();

    std::cout << "root use count: " << root.use_count() << std::endl;

    //std::shared_ptr<Node> child1 = std::make_shared<Node>(state);
    //child1->doAction(root->getChild(ActionTypes::GetActionType("Probe")), m_params);

    //root->cleanUp();
}

std::pair<std::shared_ptr<Node>, bool> CombatSearch_IntegralMCTS::getPromisingNode(std::shared_ptr<Node> node) 
{
    // create copies of integral and the build order
    m_promisingNodeBuildOrder = m_buildOrder;
    m_promisingNodeIntegral = m_integral;

    std::shared_ptr<Node> returnNode = node;
    std::shared_ptr<Edge> edge;

    while(returnNode->getNumEdges() > 0 && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        const GameState & prevStateNode = returnNode->getState();

        // select the edge with the highest UCT value
        edge = returnNode->selectChildEdge(m_exploration_parameter, m_params);

        // the node doesn't exist in memory
        if (edge->getChild() == nullptr)
        {
            returnNode = returnNode->notExpandedChild(edge, m_params);
            updateNodeVisits(edge->getChild() != nullptr, isTerminalNode(*returnNode));
            updateBOIntegral(*returnNode, edge->getAction(), prevStateNode, false);
            return std::pair<std::shared_ptr<Node>, bool>(returnNode, true);
        }

        // the node exists and is pointed to by the edge
        // get the next child
        returnNode = edge->getChild();
        updateNodeVisits(false, isTerminalNode(*returnNode));

        // update build order and integral
        updateBOIntegral(*returnNode, edge->getAction(), prevStateNode, false);
    }

    return std::pair<std::shared_ptr<Node>, bool>(returnNode, false);
}

bool CombatSearch_IntegralMCTS::isTerminalNode(const Node & node) const
{
    return node.isTerminal();
}

void CombatSearch_IntegralMCTS::randomPlayout(Node node)
{
    // do a rollout until we reach a terminal state
    while (!isTerminalNode(node) && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        GameState prevGameState(node.getState());
        doRandomAction(node, prevGameState);
    }
}

void CombatSearch_IntegralMCTS::doRandomAction(Node & node, const GameState & prevGameState)
{
    // generate possible actions
    ActionSetAbilities legalActions;
    generateLegalActions(node.getState(), legalActions, m_params);

    // the placeholder Chronoboost is expanded into a Chronoboost for each target.
    // this gives each chrono boostable target a fair chance at getting picked
    //getChronoBoostTargets(node, legalActions);

    // do an action at random
    if (legalActions.size() > 0)
    {
        std::uniform_int_distribution<> distribution(0, legalActions.size() - 1);
        int index = distribution(m_rnggen);
        Action action = legalActions[index];

        while (!node.doAction(action, m_params))
        {
            legalActions.remove(action.first, index);
            if (legalActions.size() == 0)
            {
                updateIntegralTerminal(node, prevGameState);
                node.setTerminal();
                m_results.leafNodesVisited++;
                return;
            }
            else if (legalActions.size() == 1)
            {
                index = 0;
                action = legalActions[index];
            }
            else
            {
                std::uniform_int_distribution<> distribution(0, legalActions.size() - 1);
                index = distribution(m_rnggen);
                action = legalActions[index];
            }
        }
        updateBOIntegral(node, ActionAbilityPair(action.first, node.getState().getLastAbility()), prevGameState, false);
    }
    else
    {
        updateIntegralTerminal(node, prevGameState);
        node.setTerminal();
    }

    updateNodeVisits(false, isTerminalNode(node));
}

//void CombatSearch_IntegralMCTS::getChronoBoostTargets(const Node & node, ActionSetAbilities & legalActions)
//{
//    int chronoBoostIndex = -1;
//    for (auto it = legalActions.begin(); it != legalActions.end(); ++it)
//    {
//        if (it->first.isAbility())
//        {
//            chronoBoostIndex = int(it - legalActions.begin());
//            node.getState().getSpecialAbilityTargets(legalActions, chronoBoostIndex);
//            break;
//        }
//
//    }
//    // chronoboost is an invalid action
//    if (chronoBoostIndex != -1 && legalActions[chronoBoostIndex].second == -1)
//    {
//        legalActions.remove(legalActions[chronoBoostIndex].first, chronoBoostIndex);
//    }
//}

void CombatSearch_IntegralMCTS::updateIntegralTerminal(const Node & node, const GameState & prevGameState)
{
    GameState stateCopy(prevGameState);
    stateCopy.fastForward(m_params.getFrameTimeLimit());
    m_promisingNodeIntegral.update(stateCopy, m_promisingNodeBuildOrder, m_params, m_searchTimer, false);
}

void CombatSearch_IntegralMCTS::updateBOIntegral(const Node & node, const ActionAbilityPair & action, const GameState & prevGameState, bool permanantUpdate)
{
    // if the node is terminal, it means it doesn't have any children. we add the action
    // that took us to this node, fast forward to the frame limit, and update the integral
    if (isTerminalNode(node))
    {
        GameState stateCopy(prevGameState);
        // fast forward to the end of the timelimit
        stateCopy.fastForward(m_params.getFrameTimeLimit());

        // update the integral. since no action is done, we don't need to update the build order
        if (permanantUpdate)
        {
            m_buildOrder.add(action);
            m_integral.update(stateCopy, m_buildOrder, m_params, m_searchTimer, false);
        }
        else
        {
            m_promisingNodeBuildOrder.add(action);
            m_promisingNodeIntegral.update(stateCopy, m_promisingNodeBuildOrder, m_params, m_searchTimer, false);
        }
    }

    // add the action and calculate the integral
    else
    {
        if (permanantUpdate)
        {
            m_buildOrder.add(action);
            m_integral.update(node.getState(), m_buildOrder, m_params, m_searchTimer, false);
        }
        else
        {
            m_promisingNodeBuildOrder.add(action);
            m_promisingNodeIntegral.update(node.getState(), m_promisingNodeBuildOrder, m_params, m_searchTimer, false);
        }
    }
}

void CombatSearch_IntegralMCTS::backPropogation(std::shared_ptr<Node> node)
{
    std::shared_ptr<Node> current_node = node;
    std::shared_ptr<Edge> parent_edge = current_node->getParentEdge();

    if (m_promisingNodeIntegral.getCurrentStackValue() > m_bestIntegralFound.getCurrentStackValue() || 
        ((m_promisingNodeIntegral.getCurrentStackValue() == m_bestIntegralFound.getCurrentStackValue()) && Eval::StateBetter(m_promisingNodeIntegral.getState(), m_bestIntegralFound.getState())))
    {
        m_bestIntegralFound = m_promisingNodeIntegral;
        m_bestBuildOrderFound = m_promisingNodeBuildOrder;
        m_needToWriteBestValue = true;
    }

    //std::cout << "\nvalue of search: " << m_promisingNodeIntegral.getIntegralValue() << std::endl;

    while (parent_edge != nullptr)
    {
        parent_edge->updateEdge(m_promisingNodeIntegral.getCurrentStackValue());

        //std::cout << "value of " << current_node->getAction().first.getName() << " changed to: " << current_node->getValue() << std::endl;
        //std::cout << std::endl;

        current_node = parent_edge->getParent();
        parent_edge = current_node->getParentEdge();
    }
}

void CombatSearch_IntegralMCTS::pickBestBuildOrder(std::shared_ptr<Node> root,  bool useVisitCount)
{
    // create copies of integral and the build order
    m_promisingNodeBuildOrder = m_buildOrder;
    m_promisingNodeIntegral = m_integral;

    std::shared_ptr<Node> bestNode = root;
    std::shared_ptr<Edge> bestEdge;

    // pick the child node with the highest action value until we reach a leaf node
    while (bestNode->getNumEdges() > 0)
    {
        if (useVisitCount)
        {
            bestEdge = bestNode->getHighestVisitedChild();
        }
        else
        {
            bestEdge = bestNode->getHighestValueChild(m_params);
        }
        if (bestEdge->getChild() == nullptr)
        {
            break;
        }
        bestNode = bestEdge->getChild();

        m_promisingNodeBuildOrder.add(bestEdge->getAction());
        m_promisingNodeIntegral.update(bestNode->getState(), m_promisingNodeBuildOrder, m_params, m_searchTimer, false);
    }

    // there are no more actions, but we still need to fast forward to the time
    // limit to properly calculate the integral
    GameState finalState(bestNode->getState());
    finalState.fastForward(m_params.getFrameTimeLimit());
    m_promisingNodeIntegral.update(finalState, m_promisingNodeBuildOrder, m_params, m_searchTimer, false);
}

BuildOrderAbilities CombatSearch_IntegralMCTS::createFinishedUnitsBuildOrder(const CombatSearch_IntegralDataFinishedUnits & integral) const
{
    // create a build order based only on the units that finished
    const GameState bestState(integral.getState());
    int numInitialUnits = m_params.getInitialState().getNumUnits();
    int numTotalUnits = bestState.getNumUnits();
    auto & chronoboosts = bestState.getChronoBoostTargets();
    int chronoboostIndex = 0;
    TimeType nextCBTime = 0;
    if (chronoboosts.size() > 0)
    {
        nextCBTime = chronoboosts[chronoboostIndex].frameCast;
    }

    BuildOrderAbilities finishedUnitsBuildOrder;
    for (int index = 0; index < numTotalUnits - numInitialUnits; ++index)
    {
        auto & unit = bestState.getUnit(index + numInitialUnits);
        if (unit.getFinishFrame() == -1)
        {
            continue;
        }
        if (chronoboostIndex < chronoboosts.size())
        {
            while (nextCBTime < unit.getStartFrame())
            {
                const AbilityAction & cb = chronoboosts[chronoboostIndex];
                if (bestState.getUnit(cb.targetProductionID).getFinishFrame() != -1)
                {
                    finishedUnitsBuildOrder.add(cb.type, cb);
                }
                chronoboostIndex++;
                if (chronoboostIndex == chronoboosts.size())
                {
                    break;
                }
                nextCBTime = chronoboosts[chronoboostIndex].frameCast;
            }
        }

        finishedUnitsBuildOrder.add(unit.getType());
    }
    while (chronoboostIndex < chronoboosts.size())
    {
        const AbilityAction & cb = chronoboosts[chronoboostIndex];
        if (bestState.getUnit(cb.targetProductionID).getFinishFrame() != -1)
        {
            finishedUnitsBuildOrder.add(cb.type, cb);
        }
        chronoboostIndex++;
    }

    return finishedUnitsBuildOrder;
}

void CombatSearch_IntegralMCTS::updateNodeVisits(bool nodeExpanded, bool isTerminal)
{
    m_results.nodeVisits++;
    if (nodeExpanded)
    {
        m_results.nodesExpanded++;
    }  
    if (isTerminal)
    {
        m_results.leafNodesVisited++;
    }
}

void CombatSearch_IntegralMCTS::writeResultsToFile(std::shared_ptr<Node> root)
{
    //pickBestBuildOrder(root, false);
    //FracType highestValueRoute = m_promisingNodeIntegral.getCurrentStackValue();

    m_resultsStream << m_results.nodesExpanded << "," << m_results.nodeVisits << ","
        << m_results.leafNodesExpanded << "," << m_results.leafNodesVisited << ","
        << m_results.searchTimer.getElapsedTimeInMilliSec() / 1000 << "," << m_numSimulations << ","
        << m_bestBuildOrderFound.getNameString() << "," << m_bestIntegralFound.getCurrentStackValue() << "\n";
    
    m_needToWriteBestValue = false;
}

void CombatSearch_IntegralMCTS::printResults()
{
    m_bestIntegralFound.print(m_bestBuildOrderFound);
    std::cout << "\nRan " << m_numSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000*m_numSimulations / m_results.timeElapsed) << " simulations/sec\n";
    std::cout << "Nodes expanded: " << m_results.nodesExpanded << ". Total nodes visited: " << m_results.nodeVisits << ", at a rate of " << (1000 * m_results.nodeVisits / m_results.timeElapsed) << " nodes/sec\n";
}

#include "BuildOrderPlotter.h"
void CombatSearch_IntegralMCTS::writeResultsFile(const std::string & dir, const std::string & filename)
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