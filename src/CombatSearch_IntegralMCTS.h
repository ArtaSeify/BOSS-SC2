#pragma once
#include "CombatSearch_Integral.h"
#include "Node.h"

namespace BOSS
{
    class CombatSearch_IntegralMCTS : public CombatSearch_Integral
    {
        int m_exploration_parameter;

        void recurse(const GameState & state, int depth);
        
        Node & getPromisingNode(const Node & root) const;
    public:
        CombatSearch_IntegralMCTS(const CombatSearchParameters p = CombatSearchParameters());

    };
}

