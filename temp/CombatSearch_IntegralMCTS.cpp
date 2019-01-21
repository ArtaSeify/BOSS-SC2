#include "CombatSearch_IntegralMCTS.h"

using namespace BOSS;

CombatSearch_IntegralMCTS::CombatSearch_IntegralMCTS(const CombatSearchParameters p)
    : m_exploration_parameter (EXPLORATION_PARAMETER)
{
    m_params = p;

    std::srand(uint4(std::time(0))); //use current time as seed for random generator
}

void CombatSearch_IntegralMCTS::recurse(const GameState & state, int depth)
{
    int nodes_visited = 0;
    Node root(state);
    
    while (!timeLimitReached())
    //for (int i = 0; i < 100000; ++i)
    {
        Node * promisingNode = &getPromisingNode(root);
        if (!isTerminalNode(*promisingNode))
        {
            ActionSetAbilities legalActions;
            generateLegalActions(promisingNode->getState(), legalActions, m_params);
            promisingNode->createChildren(legalActions, m_params);
        
            // there might be no action possible, so createChildren creates 0 children
            if (promisingNode->getChildNodes().size() > 0)
            {
                // get a random child node
                promisingNode = &promisingNode->getRandomChild();
            }

            if (promisingNode->timesVisited() == 0)
            {
                ++nodes_visited;
            }

            randomPlayout(*promisingNode);
        }
        backPropogation(*promisingNode);
    }
    pickBestBuildOrder(root);

    std::cout << "number of nodes visited: " << nodes_visited << std::endl;

    root.~Node();

    /*Node * bestBO = &root;
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Probe"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Pylon"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Probe"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Gateway"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Probe"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Gateway"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Probe"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Gateway"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Probe"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Zealot"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Probe"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("ChronoBoost"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Pylon"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Zealot"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Probe"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("ChronoBoost"));
    std::cout << bestBO->timesVisited() << std::endl;
    bestBO = &bestBO->getChildNode(ActionTypes::GetActionType("Zealot"));
    std::cout << bestBO->timesVisited() << std::endl;*/
}

Node & CombatSearch_IntegralMCTS::getPromisingNode(Node & node)
{
    // create copies of integral and the build order
    m_promisingNodeBuildOrder = m_buildOrder;
    m_promisingNodeIntegral = m_integral;

    Node * returnNode = &node;
    while (returnNode->getChildNodes().size() > 0)
    {
        // get the next child
        returnNode = &returnNode->selectChild(m_exploration_parameter);
        
        // update build order and integral
        updateBOIntegral(*returnNode, GameState(returnNode->getState()));
    }
    return *returnNode;
}

bool CombatSearch_IntegralMCTS::isTerminalNode(const Node & node) const
{
    return node.getState().getCurrentFrame() >= m_params.getFrameTimeLimit();
}

void CombatSearch_IntegralMCTS::randomPlayout(Node node)
{
    // do a rollout until we reach a terminal state
    while (!isTerminalNode(node))
    {
        doRandomAction(node);
    }
}

void CombatSearch_IntegralMCTS::doRandomAction(Node & node)
{
    // generate possible actions
    ActionSetAbilities legalActions;
    generateLegalActions(node.getState(), legalActions, m_params);

    // create copy of state in case we reach the time limit
    GameState stateCopy(node.getState());

    // do an action at random
    node.doAction(legalActions, std::rand() % legalActions.size(), m_params);

    updateBOIntegral(node, stateCopy);    
}

void CombatSearch_IntegralMCTS::updateBOIntegral(const Node & node, GameState & stateCopy)
{
    // if the node is terminal, we don't consider its action, instead we fastforward
    // to the end of the timelimit and update the integrals.
    if (isTerminalNode(node))
    {
        // fast forward to the end of the timelimit
        stateCopy.fastForward(m_params.getFrameTimeLimit());

        // update the integral. since no action is done, we don't need to update the build order
        m_promisingNodeIntegral.update(stateCopy, m_promisingNodeBuildOrder, m_params, m_searchTimer);
    }

    // add the action and calculate the integral
    else
    {
        auto & action = node.getAction();
        m_promisingNodeBuildOrder.add(action.first, action.second);
        m_promisingNodeIntegral.update(node.getState(), m_promisingNodeBuildOrder, m_params, m_searchTimer);
    }
}

void CombatSearch_IntegralMCTS::backPropogation(Node & node)
{
    Node * current_node = &node;

    //std::cout << "\nvalue of search: " << m_promisingNodeIntegral.getIntegralValue() << std::endl;

    while (current_node != NULL)
    {
        current_node->updateNodeValue(m_promisingNodeIntegral.getBestStackValue());

        //std::cout << "value of " << current_node->getAction().first.getName() << " changed to: " << current_node->getValue() << std::endl;
        //std::cout << std::endl;

        current_node = current_node->getParent();
    }
}

void CombatSearch_IntegralMCTS::pickBestBuildOrder(Node & root)
{
    Node * bestNodes = &root;

    // pick the child node with the highest action value until we reach a leaf node
    while (bestNodes->getChildNodes().size() > 0)
    {
        bestNodes = &bestNodes->getChildHighestValue();
        
        m_buildOrder.add(bestNodes->getAction().first, bestNodes->getState().getLastAbility());
        m_integral.update(bestNodes->getState(), m_buildOrder, m_params, m_searchTimer);
    }

    // there are no more actions, but we still need to fast forward to the time
    // limit to properly calculate the integral
    GameState finalState(bestNodes->getState());
    finalState.fastForward(m_params.getFrameTimeLimit());
    m_integral.update(finalState, m_buildOrder, m_params, m_searchTimer);
}