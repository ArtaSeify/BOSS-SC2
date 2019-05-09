#pragma once

#include "Common.h"
#include "GameState.h"
#include "ActionSetAbilities.h"
#include "CombatSearchParameters.h"
#include "Edge.h"
#include <random>

namespace BOSS
{
    class Node : public std::enable_shared_from_this<Node>
    {
        std::shared_ptr<Edge> m_parentEdge;             // the edge leading to the parent
        GameState m_state;                              // the current game state
        std::vector<std::shared_ptr<Edge>> m_edges;     // each edge leads to a child node
        bool isTerminalNode;
        
    public:
        Node(const GameState & state);
        Node(const GameState & state, std::shared_ptr<Edge> parentEdge);

        // cleans up the entire search tree
        void cleanUp();

        // creates edges pointing to the states we can get to from this state
        void createChildrenEdges(ActionSetAbilities & legalActions, const CombatSearchParameters & params);

        // removes all edges except the one passed as a parameter
        void removeEdges(std::shared_ptr<Edge> edge);

        // does action at index of actions. Returns true if the action is sucessful
        // and the state is valid (doesn't go past the time limit). Otherwise returns false
        bool doAction(std::shared_ptr<Edge> edge, const CombatSearchParameters & params, bool makeNode = false);
        bool doAction(const Action & action, const CombatSearchParameters & params);
        
        void printChildren() const;
        
        // selects a child based on UCB
        std::shared_ptr<Edge> selectChildEdge(FracType exploration_param, std::mt19937& rnggen, const CombatSearchParameters & params) const;

        // creates a node that hasn't been expanded in the tree yet 
        std::shared_ptr<Node> notExpandedChild(std::shared_ptr<Edge> edge, const CombatSearchParameters & params, bool makeNode = false) const;
        
        // returns the child with the highest action value
        std::shared_ptr<Edge> getHighestValueChild(const CombatSearchParameters & params) const;

        // returns the child with the highest visit count
        std::shared_ptr<Edge> getHighestVisitedChild() const;

        // gets a child at random
        std::shared_ptr<Edge> getRandomEdge();

        // returns the child node that is the result of action
        std::shared_ptr<Edge> getEdge(int index) { return m_edges[index]; }
        std::shared_ptr<Edge> getChild(const ActionAbilityPair& action);
        Node & getChild(int index) 
        { 
            BOSS_ASSERT(index < m_edges.size(), "index out of range for m_edges"); 
            return *m_edges[index]->getChild(); 
        }

        std::shared_ptr<Edge> getParentEdge() { return m_parentEdge; }
        void setParentEdge(std::shared_ptr<Edge> edge) { edge.swap(m_parentEdge); }
        const GameState & getState() const { return m_state; }
        std::shared_ptr<Node> getParent() { return m_parentEdge->getParent(); }
        int getNumEdges() const { return int(m_edges.size()); }
        bool isTerminal() const { return isTerminalNode; }
        void setTerminal() { isTerminalNode = true; }

        void clearChildren() { m_edges.clear(); }
    };
}
