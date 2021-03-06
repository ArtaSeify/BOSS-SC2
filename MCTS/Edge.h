#pragma once

#include "Common.h"
#include "ActionSetAbilities.h"
#include "ActionType.h"
#include "AbilityAction.h"
#include <atomic>

namespace BOSS
{
    using ActionAbilityPair = std::pair<ActionType, AbilityAction>;
    using Action = ActionSetAbilities::ActionTargetPair;
    class Node;
    class Edge
    {
    public:
        static std::atomic<FracType> CURRENT_HIGHEST_VALUE;
        static std::atomic<int> NODE_VISITS_BEFORE_EXPAND;
        static std::atomic<bool> USE_MAX_VALUE;
        static std::atomic<int> MAX_EDGE_VALUE_EXPECTED;
        static std::atomic<FracType> MIXING_VALUE;

    private:
        int m_timesVisited;
        FracType m_valueSimulations;
        FracType m_valueNetwork;
        FracType m_value;
        FracType m_averageValue;
        FracType m_maxValue;
        FracType m_policyValue;
        ActionAbilityPair m_action;

        std::shared_ptr<Node> m_child;
        std::shared_ptr<Node> m_parent;

        void setNewEdgeValue();

    public:
        Edge();
        Edge(const ActionAbilityPair & action, std::shared_ptr<Node> parent);

        void cleanUp();
        void reset();

        void updateEdge(FracType simulationValue);
        void setNetworkValue(FracType newValue) { m_valueNetwork = newValue; setNewEdgeValue(); }
        FracType getNetworkValue() const { return m_valueNetwork; }
        
        const ActionAbilityPair & getAction() const { return m_action; }
        void setAction(const ActionAbilityPair & newAction) { m_action = newAction; }

        void setPolicyValue(FracType value) { m_policyValue = value; }
        FracType getPolicyValue() const { return m_policyValue; }
        
        std::shared_ptr<Node> getChild() { return m_child; }
        void setChild(std::shared_ptr<Node> node);
        std::shared_ptr<Node> getParent() { return m_parent; }

        int timesVisited() const { return m_timesVisited; }
        FracType getValue() const { return m_value; }
        FracType getMean() const { return m_averageValue; }
        FracType getMax() const { return m_maxValue; }

        void printValues() const;
        
        //double getSD() const;
    };
}

