#include "CombatSearch_IntegralMCTS.h"
#include <random>

using namespace BOSS;

CombatSearch_IntegralMCTS::CombatSearch_IntegralMCTS(const CombatSearchParameters p, const std::string & dir, const std::string & prefix)
    : m_exploration_parameter (p.getExplorationValue())
{
    m_params = p;

    m_writeEveryKSimulations = 10;
    m_save_dir = dir;
    m_file_prefix = prefix;

    std::random_device rd; // obtain a random number from hardware
    m_rnggen.seed(rd());
}

void CombatSearch_IntegralMCTS::recurse(const GameState & state, int depth)
{
    //test(state);
    m_numSimulations = 0;

    std::shared_ptr<Node> root = std::make_shared<Node>(state);

    while (!timeLimitReached() && m_numSimulations < m_params.getNumberOfSimulations())
    //for (int i = 0; i < 150000; ++i)
    {
        if ((m_numSimulations % 1000) == 0)
        {
            std::cout << "have run : " << m_numSimulations << " simulations thus far." << std::endl;
        }
        auto & nodePair = getPromisingNode(root);
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
                if (promisingNode->getNumEdges() > 0)
                {
                    // get a random child node
                    promisingNode = promisingNode->notExpandedChild(promisingNode->getRandomEdge(), m_params);
                }
                randomPlayout(*promisingNode);
            }
        }
        backPropogation(promisingNode);

        m_numSimulations++;

        if (m_numSimulations%m_writeEveryKSimulations == 0)
        {
            writeResultsToFile(root);
        }
    }
    pickBestBuildOrder(root, true);

    m_buildOrder = m_promisingNodeBuildOrder;
    m_integral = m_promisingNodeIntegral;
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

std::pair<std::shared_ptr<Node>, bool> CombatSearch_IntegralMCTS::getPromisingNode(std::shared_ptr<Node> node) 
{
    // create copies of integral and the build order
    m_promisingNodeBuildOrder = m_buildOrder;
    m_promisingNodeIntegral = m_integral;

    std::shared_ptr<Node> returnNode = node;
    std::shared_ptr<Edge> edge;

    while(returnNode->getNumEdges() > 0)
    {
        const GameState & prevStateNode = returnNode->getState();

        // select the edge with the highest UCT value
        edge = returnNode->selectChildEdge(m_exploration_parameter, m_params);

        // the node doesn't exist, so we return this edge
        if (edge->getChild() == nullptr)
        {
            returnNode = returnNode->notExpandedChild(edge, m_params);
            updateBOIntegral(*returnNode, edge->getAction(), prevStateNode);
            return std::pair<std::shared_ptr<Node>, bool>(returnNode, true);
        }

        // the node exists and is pointed to by the edge
        // get the next child
        returnNode = edge->getChild();

        // update build order and integral
        updateBOIntegral(*returnNode, edge->getAction(), prevStateNode);
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
    while (!isTerminalNode(node))
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

    // do an action at random
    std::uniform_int_distribution<> distribution(0, legalActions.size()-1);
    const Action & action = legalActions[distribution(m_rnggen)];
    node.doAction(action, m_params);

    updateBOIntegral(node, action, prevGameState);    
}

void CombatSearch_IntegralMCTS::updateBOIntegral(const Node & node, const Action & action, const GameState & prevGameState)
{
    // if the node is terminal, we don't consider its action, instead we fastforward
    // to the end of the timelimit and update the integrals.
    if (isTerminalNode(node))
    {
        GameState stateCopy(prevGameState);
        // fast forward to the end of the timelimit
        stateCopy.fastForward(m_params.getFrameTimeLimit());

        // update the integral. since no action is done, we don't need to update the build order
        m_promisingNodeIntegral.update(stateCopy, m_promisingNodeBuildOrder, m_params, m_searchTimer);
    }

    // add the action and calculate the integral
    else
    {
        m_promisingNodeBuildOrder.add(action.first, action.second);
        m_promisingNodeIntegral.update(node.getState(), m_promisingNodeBuildOrder, m_params, m_searchTimer);
    }
}

void CombatSearch_IntegralMCTS::backPropogation(std::shared_ptr<Node> node)
{
    std::shared_ptr<Node> current_node = node;
    std::shared_ptr<Edge> parent_edge = current_node->getParentEdge();

    //std::cout << "\nvalue of search: " << m_promisingNodeIntegral.getIntegralValue() << std::endl;

    while (parent_edge != nullptr)
    {
        parent_edge->updateEdge(m_promisingNodeIntegral.getBestStackValue());

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
        bestNode = bestEdge->getChild();
        if (bestNode == nullptr)
        {
            bestNode = bestNode->notExpandedChild(bestEdge, m_params);
        }
        const Action & action = bestEdge->getAction();

        if (action.first.isAbility())
        {
            m_promisingNodeBuildOrder.add(action.first, bestNode->getState().getLastAbility());
        }
        else
        {
            m_promisingNodeBuildOrder.add(action.first);
        }
        m_promisingNodeIntegral.update(bestNode->getState(), m_promisingNodeBuildOrder, m_params, m_searchTimer);
    }

    // there are no more actions, but we still need to fast forward to the time
    // limit to properly calculate the integral
    GameState finalState(bestNode->getState());
    finalState.fastForward(m_params.getFrameTimeLimit());
    m_promisingNodeIntegral.update(finalState, m_promisingNodeBuildOrder, m_params, m_searchTimer);
}

void CombatSearch_IntegralMCTS::writeResultsToFile(std::shared_ptr<Node> root)
{
    std::ofstream file(m_save_dir + "/" + m_file_prefix + "_Results.csv", std::ofstream::out | std::ofstream::app);
    std::stringstream ss;

    // picking the most visited route
    pickBestBuildOrder(root, true);
    ss << m_promisingNodeIntegral.getCurrentStackValue() << ",";

    // picking the route with the highest value
    pickBestBuildOrder(root, false);
    ss << m_promisingNodeIntegral.getCurrentStackValue() << "\n";

    file << ss.str();
    file.close();
}

void CombatSearch_IntegralMCTS::printResults()
{
    m_integral.print();
    std::cout << "\nRan " << m_numSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000*m_numSimulations / m_results.timeElapsed) << " simulations/sec\n\n";
}

