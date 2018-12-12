#include "CombatSearch_IntegralMCTS.h"

using namespace BOSS;

CombatSearch_IntegralMCTS::CombatSearch_IntegralMCTS(const CombatSearchParameters p = CombatSearchParameters())
{
    m_params = p;
}

void CombatSearch_IntegralMCTS::recurse(const GameState & state, int depth)
{
    if (timeLimitReached())
    {
        throw BOSS_COMBATSEARCH_TIMEOUT;
    }

    updateResults(state);

    if (isTerminalNode(state, depth))
    {
        return;
    }

    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);

    pickAction(legalActions);
}

void CombatSearch_IntegralMCTS::pickAction(ActionSetAbilities legalActions)
{

}