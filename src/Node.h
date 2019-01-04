#pragma once

#include "GameState.h"
#include "ActionSetAbilities.h"
#include "CombatSearchParameters.h"

namespace BOSS
{
    class Node
    {
        Node * m_parent;
        GameState m_state;
        ActionSetAbilities::ActionTargetPair m_action;    // the action that created this node
        int m_timesVisited;
        FracType m_value;

        // holds the child nodes and the action value of transitioning
        // to each child node
        std::vector<Node*> m_children;

    public:
        Node();
        Node(const GameState & state);
        Node(const GameState & state, BOSS::Node * parent);

        // creates the child nodes and stores them inside of m_children
        void createChildren(ActionSetAbilities & actions);
        
        bool doAction(ActionSetAbilities & actions, int index);

        // selects a child based on UCB
        Node & selectChild(int exploration_param) const;

        // gets a child at random
        Node & getRandomChild() const;

        // sets the action that created this node
        void setAction(const ActionSetAbilities::ActionTargetPair & action) { m_action = action; }

        void updateNodeValue(FracType newActionValue);

        int timesVisited() const { return m_timesVisited; }
        FracType getValue() const { return m_value; }
        const std::vector<Node*> & getChildNodes() const { return m_children; }
        const GameState & getState() const { return m_state; }
        const ActionSetAbilities::ActionTargetPair & getAction() const { return m_action; }
        Node * getParent() const { return m_parent; }
    };
}
