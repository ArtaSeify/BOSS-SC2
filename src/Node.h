#pragma once

#include "GameState.h"
#include "ActionSetAbilities.h"
#include "CombatSearchParameters.h"

namespace BOSS
{
    class Node
    {
        using Edge = std::pair<Node*, FracType>;

        Node * m_parent;
        GameState m_state;
        int m_timesVisited;

        // holds the child nodes and the action value of transitioning
        // to each child node
        std::vector<Edge> m_children;

    public:
        Node();
        Node(const CombatSearchParameters & params, const GameState & state, BOSS::Node & parent);

        void createChildren(ActionSetAbilities & actions, const CombatSearchParameters & params);
        Node & selectChild(int exploration_param) const;

        int timesVisited() const { return m_timesVisited; }
        const std::vector<Edge> & getChildNodes() const { return m_children; }
    };
}
