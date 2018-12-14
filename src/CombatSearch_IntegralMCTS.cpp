#include "CombatSearch_IntegralMCTS.h"

using namespace BOSS;

CombatSearch_IntegralMCTS::CombatSearch_IntegralMCTS(const CombatSearchParameters p)
    : m_exploration_parameter (5)
{
    m_params = p;
}

void CombatSearch_IntegralMCTS::recurse(const GameState & state, int depth)
{
    Node root(m_params, state, Node());
    
    while (!timeLimitReached())
    {
        Node & promisingNode = getPromisingNode(root);
        if (!isTerminalNode(promisingNode))
        {
            ActionSetAbilities legalActions;
            generateLegalActions(promisingNode.getState(), legalActions, m_params);
            promisingNode.createChildren(legalActions, m_params);
        
            // there might be no action possible, so createChildren creates 0 children
            Node & nodeToExplore = promisingNode;
            if (promisingNode.getChildNodes().size() > 0)
            {
                // get a random child node
                nodeToExplore = promisingNode.getRandomChild();

                updateBOIntegral(promisingNode);
            }

            randomPlayout(nodeToExplore);
        }

    }
}

Node CombatSearch_IntegralMCTS::getPromisingNode(const Node & root)
{
    // create copies
    m_promisingNodeBuildOrder = m_buildOrder;
    m_promisingNodeIntegral = m_integral;

    Node child(root);

    while (child.getChildNodes().size() > 0)
    {
        // get the next child
        child = child.selectChild(m_exploration_parameter);
        
        updateBOIntegral(child);
    }

    return child;
}

bool CombatSearch_IntegralMCTS::isTerminalNode(const Node & node) const
{
    return node.getState().getCurrentFrame() >= m_params.getFrameTimeLimit();
}

void CombatSearch_IntegralMCTS::randomPlayout(const Node & node)
{
    // create copy of node so we can simulate a rollout
    Node playoutNode(node);

    // do a rollout until we reach a terminal state
    while (!isTerminalNode(playoutNode))
    {
        doRandomAction(playoutNode);
    }
}

void CombatSearch_IntegralMCTS::doRandomAction(Node & node)
{
    // generate possible actions
    ActionSetAbilities legalActions;
    generateLegalActions(node.getState(), legalActions, m_params);

    // do an action at random
    node.doAction(legalActions, std::rand() % legalActions.size());
    updateBOIntegral(node);    
}

void CombatSearch_IntegralMCTS::updateBOIntegral(const Node & node)
{
    // if the node is terminal, we don't consider its action, instead we fastforward
    // to the end of the timelimit and update the integrals.
    if (isTerminalNode(node))
    {
        // create copy of the gamestate before it was terminal and fast forward to the end
        // of the timelimit
        GameState state(node.getParent()->getState());
        state.fastForward(m_params.getFrameTimeLimit());

        // update the integral. since no action is done, we don't need to update the build order
        m_promisingNodeIntegral.update(state, m_buildOrder, m_params, m_searchTimer);
    }

    // add the action and calculate the integral
    else
    {
        auto & action = node.getAction();
        m_promisingNodeBuildOrder.add(action.first, action.second);
        m_promisingNodeIntegral.update(node.getState(), m_promisingNodeBuildOrder, m_params, m_searchTimer);
    }
}

