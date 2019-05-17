#pragma once

#include "Common.h"
#include "ActionSetAbilities.h"
#include "ActionType.h"
#include "AbilityAction.h"

namespace BOSS
{
    using ActionAbilityPair = std::pair<ActionType, AbilityAction>;
    using Action = ActionSetAbilities::ActionTargetPair;
    class Node;
    class Edge
    {
    public:
        static FracType CURRENT_HIGHEST_VALUE;
        static int NODE_VISITS_BEFORE_EXPAND;
        static FracType MIXING_PARAMETER;
        static bool USE_MAX_VALUE;

    private:
        int m_timesVisited;
        FracType m_valueSimulations;
        FracType m_valueNetwork;
        FracType m_value;
        FracType m_averageValue;
        FracType m_maxValue;
        double m_valuesSquared;
        ActionAbilityPair m_action;

        std::shared_ptr<Node> m_child;
        std::shared_ptr<Node> m_parent;

        void setNewEdgeValue();

    public:
        Edge();
        Edge(const ActionAbilityPair & action, std::shared_ptr<Node> parent);

        void cleanUp();

        void updateEdge(FracType simulationValue);
        void setNetworkValue(FracType newValue) { m_valueNetwork = newValue; setNewEdgeValue(); }
        
        const ActionAbilityPair & getAction() const { return m_action; }
        void setAction(const ActionAbilityPair & newAction) { m_action = newAction; }
        
        std::shared_ptr<Node> getChild() { return m_child; }
        void setChild(std::shared_ptr<Node> node);
        std::shared_ptr<Node> getParent() { return m_parent; }

        int timesVisited() const { return m_timesVisited; }
        FracType getValue() const { return m_value; }
        FracType getMean() const { return m_averageValue; }
        FracType getMax() const { return m_maxValue; }

        void printValues() const;
        
        double getSD() const;
    };
}

