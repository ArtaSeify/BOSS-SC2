#pragma once

#include "Common.h"
#include "GameState.h"
#include "ActionSetAbilities.h"
#include "CombatSearchParameters.h"
#include "Edge.h"

namespace BOSS
{
    class Node
    {
    public:
        static FracType CURRENT_HIGHEST_VALUE;

    private:
        std::unique_ptr<Edge> m_parentEdge;            // the edge leading to the parent
        GameState m_state;              // the current game state
        std::vector<std::unique_ptr<Edge>> m_edges;     // each edge leads to a child node
        
    public:
        Node(const GameState & state);

        // creates edges pointing to the states we can get to 
        // from this state
        void createChildrenEdges(ActionSetAbilities & legalActions, const CombatSearchParameters & params);

        // does action at index of actions. Returns true if the action is sucessful
        // and the state is valid (doesn't go past the time limit). Otherwise returns false
        bool doAction(std::unique_ptr<Edge> & edge, const CombatSearchParameters & params);
        
        void printChildren() const;
        
        // selects a child based on UCB
        std::unique_ptr<Node> selectChild(int exploration_param, const CombatSearchParameters & params);
        
        // returns the child with the highest action value
        std::unique_ptr<Node> getHighestValueChild(const CombatSearchParameters & params);

        // gets a child at random
        std::unique_ptr<Node> getRandomChild();

        // updates the value and the visit count of the node
        void updateNodeValue(FracType newActionValue);

        // returns the child node that is the result of action
        std::unique_ptr<Node> getChildNode(ActionType action);

        void setParentEdge(Edge * edge) { m_parentEdge = std::unique_ptr<Edge>(edge); }
        const GameState & getState() const { return m_state; }
        Node & getParent() { return *m_parentEdge->getParent(); }
        Node & getChild(int index) { return *m_edges[index]->getChild(); }
        int getNumEdgesOut() const { return int(m_edges.size()); }
    };
}
