#pragma once

#include "Common.h"
#include "GameState.h"
#include "ActionSetAbilities.h"
#include "CombatSearchParameters.h"
#include "Edge.h"

#include "gsl/gsl_rng.h"
#include "gsl/gsl_randist.h"
#include <random>
#include <mutex>

namespace BOSS
{
    class Node : public std::enable_shared_from_this<Node>
    {
        using Children = std::vector<std::shared_ptr<Edge>>;

        GameState                   m_state;
        Children                    m_children;
        std::shared_ptr<Edge>       m_parent;
        mutable std::mutex          m_lock;

        bool                        m_leaf;
        std::atomic_int             m_timesVisited;
        mutable bool                m_policySet;        // whether the policy for the edges has been set

        
        static const std::array<double,70> alphas;              // dirichlet noise alpha value
        
    public:
        Node(const std::shared_ptr<Edge> & parent);
        Node(const std::shared_ptr<Edge> & parent, const GameState & state);

        Node& operator=(const Node& node);

        // cleans up the entire search tree
        void cleanUp(int threads);

        // removes all edges except the one passed as a parameter
        void removeEdges(const Edge & edge);
        void removeEdges();

        void changeRootCheck(const CombatSearchParameters& params, int currentRootSimulations) const;

        // does action at index of actions. Returns true if the action is sucessful
        // and the state is valid (doesn't go past the time limit). Otherwise returns false
        bool doAction(Edge & edge, const CombatSearchParameters & params, bool makeNode = false);
        void doAction(const Action & action, const CombatSearchParameters & params);

        // creates edges for this node
        void createChildrenEdges(const CombatSearchParameters& params, FracType currentValue, bool rootNode = false, gsl_rng * gsl_r = nullptr);

        // creates edges for thie node. This function is called when we create the edges and do network
        // evaluation after an expanded node is visited a second time, not the first time
        //void createChildrenEdgesSecondVisit(const CombatSearchParameters& params, FracType currentValue);

        // adds Dirichlet noise to the policy
        void addNoise(gsl_rng* gsl_r);
        
        void printChildren() const;
        
        // selects a child based on UCB
        std::shared_ptr<Edge> getPromisingChild(const CombatSearchParameters & params, FracType currentValue);

        // make network prediction about state of node and store result in edges
        void networkPrediction(const CombatSearchParameters& params, FracType currentValue) const;

        // returns the child with the highest action value
        std::shared_ptr<Edge> getHighestValueChild(const CombatSearchParameters & params) const;

        // returns the child with the highest visit count
        std::shared_ptr<Edge> getHighestVisitedChild() const;

        // returns the child with the highest network policy value
        std::shared_ptr<Edge> getHighestPolicyValueChild() const;

        // returns a child with probability proportional to visit count
        std::shared_ptr<Edge> getChildProportionalToVisitCount(const CombatSearchParameters& params, std::mt19937 & rnggen) const;

        // gets a child at random
        std::shared_ptr<Edge> getRandomChild(RNG& rng);

        // returns the child node that is the result of action
        std::shared_ptr<Edge> getChild(const ActionAbilityPair& action);

        //std::vector<int> getChronoboostTargets() const;

        const std::shared_ptr<Edge>& getParent() const { return m_parent; }
        const GameState & getState() const { return m_state; }
        bool isTerminal() const { std::scoped_lock sl(m_lock); return m_leaf; }
        int numChildren() const { std::scoped_lock sl(m_lock); return (int)m_children.size(); }

    private:
        void generateLegalActions(const GameState& state, ActionSetAbilities& legalActions, const CombatSearchParameters& params);
    };
}
