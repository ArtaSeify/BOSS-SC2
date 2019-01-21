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
        int m_timesVisited;
        FracType m_value;
        Action m_action;

        std::shared_ptr<Node> m_parent;
        std::unique_ptr<Node> m_child;

    public:
        Edge();
        Edge(const Action & action, Node * parent, Node * child);
        
        const Action & getAction() const { return m_action; }
        void setAction(const Action & newAction) { m_action = newAction; }

        std::unique_ptr<Node> & getChild() { return m_child; }
        std::shared_ptr<Node> & getParent() { return m_parent; }

        int timesVisited() const { return m_timesVisited; }
        void incrementVisited() { m_timesVisited++; }

        FracType getValue() const { return m_value; }
    };
}

