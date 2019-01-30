#pragma once

#include "Common.h"
#include "ActionSetAbilities.h"
#include "ActionType.h"

namespace BOSS
{
    using Action = ActionSetAbilities::ActionTargetPair;
    class Node;
    class Edge
    {
    public:
        static FracType CURRENT_HIGHEST_VALUE;
        static int NODE_VISITS_BEFORE_EXPAND;
        
    public:
        int m_timesVisited;
        FracType m_value;
        Action m_action;

        std::shared_ptr<Node> m_child;
        std::shared_ptr<Node> m_parent;

    public:
        Edge();
        Edge(const Action & action, std::shared_ptr<Node> parent);

        void updateEdge(FracType newActionValue);
        
        const Action & getAction() const { return m_action; }
        void setAction(const Action & newAction) { m_action = newAction; }
        
        std::shared_ptr<Node> getChild() { return m_child; }
        void setChild(std::shared_ptr<Node> node);
        std::shared_ptr<Node> getParent() { return m_parent; }

        int timesVisited() const { return m_timesVisited; }
        FracType getValue() const { return m_value; }
    };
}

