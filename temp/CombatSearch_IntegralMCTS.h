#pragma once
#include "CombatSearch_Integral.h"
#include "Node.h"

namespace BOSS
{
    static const int EXPLORATION_PARAMETER = 10;

    class CombatSearch_IntegralMCTS : public CombatSearch_Integral
    {
        int m_exploration_parameter;

        CombatSearch_IntegralDataFinishedUnits m_promisingNodeIntegral;
        BuildOrderAbilities m_promisingNodeBuildOrder;

        void recurse(const GameState & state, int depth);
        
        Node & getPromisingNode(Node & root);
        bool isTerminalNode(const Node & node) const;
        void randomPlayout(Node node);
        
        // does a random action
        void doRandomAction(Node & node);
    
        // updates both m_promisingNodeIntegral and m_promisingNodeBuildOrder
        void updateBOIntegral(const Node & node, GameState & stateCopy);

        // updates values of the nodes explored in this iteration
        void backPropogation(Node & node);

        // sets the class variables to the best build order found during search
        void pickBestBuildOrder(Node & root);

    public:
        CombatSearch_IntegralMCTS(const CombatSearchParameters p = CombatSearchParameters());

    };
}

