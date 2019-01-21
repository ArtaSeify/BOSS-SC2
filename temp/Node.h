#pragma once

#include "GameState.h"
#include "ActionSetAbilities.h"
#include "CombatSearchParameters.h"

namespace BOSS
{
    class Node
    {
    public:
        static FracType CURRENT_HIGHEST_VALUE;

    private:
        Node * m_parent;
        GameState m_state;
        ActionSetAbilities::ActionTargetPair m_action;    // the action that created this node
        int m_timesVisited;
        FracType m_value;

        // holds the child nodes
        std::vector<Node> m_children;

    public:
        Node();
        Node(const GameState & state);
        Node(const GameState & state, BOSS::Node * parent);

        // creates the child nodes and stores them inside of m_children
        void createChildren(ActionSetAbilities & actions, const CombatSearchParameters & params);
        
        bool doAction(ActionSetAbilities & actions, int index, const CombatSearchParameters & params);

        // selects a child based on UCB
        Node & selectChild(int exploration_param);

        // returns the child with the highest action value
        Node & getChildHighestValue();

        // gets a child at random
        Node & getRandomChild();

        // updates the value and the visit count of the node
        void updateNodeValue(FracType newActionValue);

        // returns the child node that is the result of action
        Node & getChildNode(ActionType action);


        // sets the action that created this node
        void setAction(const ActionSetAbilities::ActionTargetPair & action) { m_action = action; }

        int timesVisited() const { return m_timesVisited; }
        FracType getValue() const { return m_value; }
        const std::vector<Node> & getChildNodes() const { return m_children; }
        const GameState & getState() const { return m_state; }
        const ActionSetAbilities::ActionTargetPair & getAction() const { return m_action; }
        Node * getParent() const { return m_parent; }
    };
}
