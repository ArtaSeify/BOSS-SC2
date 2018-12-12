#pragma once
#include "CombatSearch_Integral.h"

namespace BOSS
{
    class CombatSearch_IntegralMCTS : public CombatSearch_Integral
    {
        void recurse(const GameState & state, int depth);
        void pickAction(ActionSetAbilities legalActions);

    public:
        CombatSearch_IntegralMCTS(const CombatSearchParameters p = CombatSearchParameters());

    };
}

