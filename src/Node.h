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
        ActionSetAbilities::ActionTargetPair m_action;    // the action that created this node
        int m_timesVisited;

        // holds the child nodes and the action value of transitioning
        // to each child node
        std::vector<Edge> m_children;

    public:
        Node();
        Node(const CombatSearchParameters & params, const GameState & state, BOSS::Node & parent);

        // creates the child nodes and stores them inside of m_children
        void createChildren(ActionSetAbilities & actions, const CombatSearchParameters & params);
        
        bool doAction(ActionSetAbilities & actions, int index);

        // selects a child based on UCB
        Node & selectChild(int exploration_param) const;

        // gets a child at random
        Node & getRandomChild() const;

        // sets the action that created this node
        void setAction(const ActionSetAbilities::ActionTargetPair & action) { m_action = action; }

        int timesVisited() const { return m_timesVisited; }
        const std::vector<Edge> & getChildNodes() const { return m_children; }
        const GameState & getState() const { return m_state; }
        const ActionSetAbilities::ActionTargetPair & getAction() const { return m_action; }
        Node * getParent() const { return m_parent; }
    };
}
