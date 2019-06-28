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
    using Action = ActionSetAbilities::ActionTargetPair;
    class Node;
    class Edge : public std::enable_shared_from_this<Edge>
    {
    public:
        static FracType CURRENT_HIGHEST_VALUE;
        static int NODE_VISITS_BEFORE_EXPAND;
        static bool USE_MAX_VALUE;
        static int MAX_EDGE_VALUE_EXPECTED;
        static FracType MIXING_VALUE;
        static int VIRTUAL_LOSS_VALUE;
        static int VIRTUAL_LOSS_COUNT;

    private:
        int m_timesVisited;
        int m_virtualLoss;
        FracType m_valueSimulations;
        FracType m_valueNetwork;
        FracType m_value;
        FracType m_averageValue;
        FracType m_maxValue;
        FracType m_policyValue;
        ActionAbilityPair m_action;

        mutable std::recursive_mutex m_mutex;

        std::shared_ptr<Node> m_child;
        std::shared_ptr<Node> m_parent;

        void setNewEdgeValue();

    public:
        Edge();
        Edge(const ActionAbilityPair & action, std::shared_ptr<Node> parent);
        Edge(const Edge& edge);

        void cleanUp();
        void reset();

        void updateEdge(FracType simulationValue, FracType networkValue);

        void setNetworkValue(FracType newValue) 
        { 
            std::scoped_lock sl(m_mutex);
            m_valueNetwork = std::min(1.f, newValue); 
            setNewEdgeValue();
        }
        FracType getNetworkValue() const { std::scoped_lock sl(m_mutex); return m_valueNetwork; }
        
        const ActionAbilityPair & getAction() const { std::scoped_lock sl(m_mutex); return m_action; }

        void setPolicyValue(FracType value) { std::scoped_lock sl(m_mutex); m_policyValue = value; }
        FracType getPolicyValue() const { std::scoped_lock sl(m_mutex); return m_policyValue; }
        
        std::shared_ptr<Node> getChild() { std::scoped_lock sl(m_mutex); return m_child; }
        void setChild(std::shared_ptr<Node> node);
        std::shared_ptr<Node> getParent() { std::scoped_lock sl(m_mutex); return m_parent; }

        int totalTimesVisited() const { std::scoped_lock sl(m_mutex); return m_timesVisited + m_virtualLoss; }
        int realTimesVisited() const { std::scoped_lock sl(m_mutex); return m_timesVisited; }
        //void incrementVisitCount() { std::scoped_lock sl(m_mutex); ++m_timesVisited; }

        int virtualLoss() const { std::scoped_lock sl(m_mutex); return m_virtualLoss; }
        void incrementVirtualLoss()  { std::scoped_lock sl(m_mutex); m_virtualLoss += VIRTUAL_LOSS_COUNT; }
        void decrementVirtualLoss() { std::scoped_lock sl(m_mutex); m_virtualLoss -= VIRTUAL_LOSS_COUNT; }
        int getVirtualLossValue() const { std::scoped_lock sl(m_mutex); return m_virtualLoss * Edge::VIRTUAL_LOSS_VALUE; }

        void visited() { std::scoped_lock sl(m_mutex); ++m_timesVisited; incrementVirtualLoss(); }
        void decVisits()
        {
            std::scoped_lock sl(m_mutex);
            m_timesVisited -= 1;
            decrementVirtualLoss();
        }

        FracType getValue() const { std::scoped_lock sl(m_mutex); return m_value; }
        FracType getMean() const { std::scoped_lock sl(m_mutex); return m_averageValue; }
        FracType getMax() const { std::scoped_lock sl(m_mutex); return m_maxValue; }

        void printValues() const;
    };
}

