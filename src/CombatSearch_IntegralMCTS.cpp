#include "CombatSearch_IntegralMCTS.h"

using namespace BOSS;

CombatSearch_IntegralMCTS::CombatSearch_IntegralMCTS(const CombatSearchParameters p)
    : m_exploration_parameter (5)
{
    m_params = p;
}

void CombatSearch_IntegralMCTS::recurse(const GameState & state, int depth)
{
    if (isTerminalNode(state, depth))
    {
        return;
    }

    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);

    Node root(m_params, state, Node());
    
    while (!timeLimitReached())
    {
        Node & promisingNode = getPromisingNode(root);
    }
}

Node & CombatSearch_IntegralMCTS::getPromisingNode(const Node & root) const
{
    Node & child = root.selectChild(m_exploration_parameter);
    
    while (child.getChildNodes().size() > 0)
    {
        child = child.selectChild(m_exploration_parameter);
    }

    return child;
}