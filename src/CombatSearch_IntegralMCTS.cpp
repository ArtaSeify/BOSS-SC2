#include "CombatSearch_IntegralMCTS.h"

using namespace BOSS;

CombatSearch_IntegralMCTS::CombatSearch_IntegralMCTS(const CombatSearchParameters p)
    : m_exploration_parameter (EXPLORATION_PARAMETER)
{
    m_params = p;

    std::srand(std::time(0)); //use current time as seed for random generator
}

void CombatSearch_IntegralMCTS::recurse(const GameState & state, int depth)
{
    Node root(state);
    
    //while (!timeLimitReached())
    {
        Node & promisingNode = getPromisingNode(root);
        if (!isTerminalNode(promisingNode))
        {
            ActionSetAbilities legalActions;
            generateLegalActions(promisingNode.getState(), legalActions, m_params);
            promisingNode.createChildren(legalActions);
        
            // there might be no action possible, so createChildren creates 0 children
            Node & nodeToExplore = promisingNode;
            if (promisingNode.getChildNodes().size() > 0)
            {
                // get a random child node
                nodeToExplore = promisingNode.getRandomChild();
            }

            randomPlayout(nodeToExplore);
            backPropogation(nodeToExplore);
        }
    }
}

Node CombatSearch_IntegralMCTS::getPromisingNode(Node node)
{
    // create copies
    m_promisingNodeBuildOrder = m_buildOrder;
    m_promisingNodeIntegral = m_integral;

    while (node.getChildNodes().size() > 0)
    {
        // get the next child
        node = node.selectChild(m_exploration_parameter);
        
        updateBOIntegral(node);
    }

    return node;
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
    node.doAction(legalActions, std::rand() % legalActions.size());

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
    Node * current_node(&node);

    while (current_node != NULL)
    {
        std::cout << current_node->getChildNodes().size() << std::endl;
        current_node->updateNodeValue(m_promisingNodeIntegral.getIntegralValue());
        std::cout << "value of " << current_node->getAction().first.getName() << " changed to: " << current_node->getValue() << std::endl;
        current_node = current_node->getParent();
    }
}