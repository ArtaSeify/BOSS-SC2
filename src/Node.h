#pragma once

#include "Common.h"
#include "GameState.h"
#include "ActionSetAbilities.h"
#include "CombatSearchParameters.h"
#include "Edge.h"
#include <random>
#include <mutex>

namespace BOSS
{
    class Node : public std::enable_shared_from_this<Node>
    {
        std::shared_ptr<Edge>               m_parentEdge;       // the edge leading to the parent
        GameState                           m_state;            // the current game state
        std::vector<std::shared_ptr<Edge>>  m_edges;            // each edge leads to a child node
        bool                                m_isTerminalNode;   // is this a terminal node (no edges)
        mutable std::mutex                  m_mutex;            // multiple threads can't read and write 
        
    public:
        Node();
        Node(const GameState & state);
        Node(const GameState & state, Edge & parentEdge);
        Node(const Node& node);

        Node& operator=(const Node& node);

        // cleans up the entire search tree
        void cleanUp(int threads);

        // creates edges pointing to the states we can get to from this state
        void createChildrenEdges(ActionSetAbilities & legalActions, const CombatSearchParameters & params, FracType currentValue);

        // removes all edges except the one passed as a parameter
        void removeEdges(const Edge & edge);
        void removeEdges();

        // does action at index of actions. Returns true if the action is sucessful
        // and the state is valid (doesn't go past the time limit). Otherwise returns false
        bool doAction(Edge & edge, const CombatSearchParameters & params, bool makeNode = false);
        void doAction(const Action & action, const CombatSearchParameters & params);
        
        void printChildren() const;
        
        // selects a child based on UCB
        Edge & selectChildEdge(FracType exploration_param, std::mt19937& rnggen, const CombatSearchParameters & params);

        // creates a node that hasn't been expanded in the tree yet 
        std::shared_ptr<Node> notExpandedChild(Edge & edge, const CombatSearchParameters & params, bool makeNode = false) const;
        
        // returns the child with the highest action value
        Edge & getHighestValueChild(const CombatSearchParameters & params) const;

        // returns the child with the highest visit count
        Edge & getHighestVisitedChild() const;

        // returns the child with the highest network policy value
        Edge & getHighestPolicyValueChild() const;

        // returns a child with probability proportional to visit count
        Edge & getChildProportionalToVisitCount(std::mt19937& rnggen, const CombatSearchParameters& params) const;

        // gets a child at random
        Edge & getRandomEdge();

        // returns the child node that is the result of action
        Edge & getEdge(int index) { return *m_edges[index]; }
        Edge & getChild(const ActionAbilityPair& action);
        Node & getChild(int index) 
        { 
            BOSS_ASSERT(index < m_edges.size(), "index out of range for m_edges"); 
            return *m_edges[index]->getChild(); 
        }

        std::vector<int> getChronoboostTargets() const;

        std::shared_ptr<Edge> getParentEdge() const { return m_parentEdge; }
        void setParentEdge(const Edge & edge) { m_parentEdge = std::make_shared<Edge>(edge); }
        const GameState & getState() const { return m_state; }
        std::shared_ptr<Node> getParent() const { return m_parentEdge->getParent(); }
        int getNumEdges() const { return int(m_edges.size()); }
        bool isTerminal() const { return m_isTerminalNode; }

        void clearChildren() { m_edges.clear(); }

    private:
        std::vector<std::shared_ptr<Edge>> getAllEdges() const { std::scoped_lock sl(m_mutex); return m_edges; }
    };
}
