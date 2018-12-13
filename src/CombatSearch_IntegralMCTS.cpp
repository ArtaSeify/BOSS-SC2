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
        }

        Node & nodeToExplore = promisingNode;
        if (promisingNode.getChildNodes().size() > 0)
        {
            nodeToExplore = promisingNode.getRandomChild();
        }
    }
}

Node & CombatSearch_IntegralMCTS::getPromisingNode(Node & root) const
{
    Node & child = root;
    
    while (child.getChildNodes().size() > 0)
    {
        child = child.selectChild(m_exploration_parameter);
    }

    return child;
}

bool CombatSearch_IntegralMCTS::isTerminalNode(const Node & node) const
{
    return node.getState().getCurrentFrame() >= m_params.getFrameTimeLimit();
}