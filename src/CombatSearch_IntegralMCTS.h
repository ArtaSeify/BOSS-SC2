#pragma once
#include "CombatSearch_Integral.h"
#include "Node.h"

namespace BOSS
{
    static const int EXPLORATION_PARAMETER = 5;
    class CombatSearch_IntegralMCTS : public CombatSearch_Integral
    {
        int m_exploration_parameter;

        CombatSearch_IntegralDataFinishedUnits m_promisingNodeIntegral;
        BuildOrderAbilities m_promisingNodeBuildOrder;

        void recurse(const GameState & state, int depth);
        
        Node getPromisingNode(Node root);
        bool isTerminalNode(const Node & node) const;
        void randomPlayout(Node node);
        
        // does a random action
        void doRandomAction(Node & node);
    
        // updates both m_promisingNodeIntegral and m_promisingNodeBuildOrder
        void updateBOIntegral(const Node & node, GameState & stateCopy = GameState());

        // updates values of the nodes explored in this iteration
        void backPropogation(Node & node);

    public:
        CombatSearch_IntegralMCTS(const CombatSearchParameters p = CombatSearchParameters());

    };
}

