#pragma once

#include "Common.h"
#include "ActionSetAbilities.h"
#include "ActionType.h"
#include "AbilityAction.h"
#include <atomic>
#include <mutex>

namespace BOSS
{
    using ActionAbilityPair = std::pair<ActionType, AbilityAction>;
    using Action = std::pair<ActionType, NumUnits>;
    class Node;
    class Edge : public std::enable_shared_from_this<Edge>
    {
    public:
        static int NODE_VISITS_BEFORE_EXPAND;
        static bool USE_MAX_VALUE;
        static FracType HIGHEST_VALUE_FOUND;
        static FracType MIXING_VALUE;
        static int VIRTUAL_LOSS;

    private:
        int m_visits;
        int m_virtualLoss;
        FracType m_valueSimulations;
        FracType m_valueNetwork;
        FracType m_value;
        FracType m_average;
        FracType m_max;
        FracType m_policy;
        ActionAbilityPair m_action;

        std::shared_ptr<Node> m_child;
        std::shared_ptr<Node> m_parent;

        mutable std::mutex m_lock;

        void setNewEdgeValue();

    public:
        Edge();
        Edge(const ActionAbilityPair & action, std::shared_ptr<Node> parent);
        Edge(const Edge& edge);

        void cleanUp();
        void reset();

        void updateEdge(FracType simulationValue, FracType networkValue);
        FracType calculatePUCT(FracType exploration, FracType totalVisitsSqrt, int normalization) const;

        void setNetworkValue(FracType newValue) 
        { 
            m_valueNetwork = newValue;
            setNewEdgeValue();
        }
        FracType getNetworkValue() const { std::scoped_lock sl(m_lock); return m_valueNetwork; }
        
        const ActionAbilityPair & getAction() const { std::scoped_lock sl(m_lock); return m_action; }

        void setPolicy(FracType newPolicy) { assert(newPolicy >= 0); m_policy = newPolicy; }
        FracType getPolicyValue() const { return m_policy; }
        
        const std::shared_ptr<Node> & getChild() { std::scoped_lock sl(m_lock); return m_child; }
        const std::shared_ptr<Node> & getParent() { std::scoped_lock sl(m_lock); return m_parent; }

        bool setChild(const std::shared_ptr<Node>& child);

        int realTimesVisited() const { std::scoped_lock sl(m_lock); return m_visits; }
        int virtualLoss() const { return m_virtualLoss; }

        void visited();

        FracType getValue() const { return m_value; }
        FracType getMean() const { return m_average; }
        FracType getMax() const { return m_max; }

        void printValues() const;
    };
}

