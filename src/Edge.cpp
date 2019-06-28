#include "Node.h"
#include "Edge.h"

using namespace BOSS;

FracType Edge::CURRENT_HIGHEST_VALUE = 1.f;
int Edge::NODE_VISITS_BEFORE_EXPAND = 1;
bool Edge::USE_MAX_VALUE = true;
int Edge::MAX_EDGE_VALUE_EXPECTED = 1;
FracType Edge::MIXING_VALUE = 0.0;
int Edge::VIRTUAL_LOSS_VALUE = 1;
int Edge::VIRTUAL_LOSS_COUNT = 1;

Edge::Edge()
    : m_timesVisited(0)
    , m_virtualLoss(0)
    , m_valueSimulations(0)
    , m_valueNetwork(0)
    , m_value(0)
    , m_averageValue(0)
    , m_maxValue(0)
    , m_policyValue(0)
    , m_action(ActionAbilityPair(ActionTypes::None, AbilityAction()))
    , m_child()
    , m_parent()
    , m_mutex()
{

}

Edge::Edge(const ActionAbilityPair & action, std::shared_ptr<Node> parent)
    : m_timesVisited(0)
    , m_virtualLoss(0)
    , m_valueSimulations(0)
    , m_valueNetwork(0)
    , m_value(0)
    , m_averageValue(0)
    , m_maxValue(0)
    , m_policyValue(0)
    , m_action(action)
    , m_child()
    , m_parent(parent)
    , m_mutex()
{

}

Edge::Edge(const Edge& edge)
{
    std::scoped_lock sl(m_mutex);

    m_timesVisited = edge.m_timesVisited;
    m_virtualLoss = edge.m_virtualLoss;
    m_valueSimulations = edge.m_valueSimulations;
    m_valueNetwork = edge.m_valueNetwork;
    m_value = edge.m_value;
    m_averageValue = edge.m_averageValue;
    m_maxValue = edge.m_maxValue;
    m_policyValue = edge.m_policyValue;
    m_action = edge.m_action;
    m_child = edge.m_child;
    m_parent = edge.m_parent;
}

void Edge::cleanUp()
{
    std::scoped_lock sl(m_mutex);

    //std::cout << "cleaning up edge: " << m_action.first.getName() << std::endl;
    if (m_child != nullptr)
    {
        //std::cout << "cleaning up child of this edge" << std::endl;
        m_child->cleanUp(0);
        //std::cout << "child use count: " << m_child.use_count() << std::endl;
        m_child.reset();
    }
    //std::cout << "parent use count: " << m_parent.use_count() << std::endl;
    m_parent.reset();
}

void Edge::reset()
{
    std::scoped_lock sl(m_mutex);

    m_timesVisited = 0;
    m_valueSimulations = 0;
    m_valueNetwork = 0;
    m_value = 0;
    m_averageValue = 0;
    m_maxValue = 0;
    m_policyValue = 0;
    m_parent.reset();
}

void Edge::updateEdge(FracType simulationValue, FracType networkValue)
{
    std::scoped_lock sl(m_mutex);

    decrementVirtualLoss();

    m_maxValue = std::max(simulationValue, m_maxValue);
    
    if (USE_MAX_VALUE)
    {
        if (m_valueSimulations < m_maxValue || m_valueNetwork < networkValue)
        {
            m_valueSimulations = m_maxValue;
            m_valueNetwork = networkValue;
            setNewEdgeValue();
        }
    }
    else
    {
        m_averageValue = m_averageValue + ((1.f / m_timesVisited) * (simulationValue - m_averageValue));
        m_valueSimulations = m_averageValue;
        setNewEdgeValue();
    }
}

void Edge::setChild(std::shared_ptr<Node> node) 
{ 
    std::scoped_lock sl(m_mutex);
    node.swap(m_child); 
}

void Edge::setNewEdgeValue()
{
    std::scoped_lock sl(m_mutex);

    //m_value = (MIXING_PARAMETER * m_valueNetwork) + ((1 - MIXING_PARAMETER)*m_valueSimulations);
    m_value = (MIXING_VALUE * m_valueNetwork) + ((1 - MIXING_VALUE) * m_valueSimulations);

    /*if (m_value > CURRENT_HIGHEST_VALUE)
    {
        CURRENT_HIGHEST_VALUE = m_value;
    }*/
}

void Edge::printValues() const
{
    std::scoped_lock sl(m_mutex);

    std::cout << "Edge action: " << m_action.first.getName() << std::endl;
    std::cout << "Network Value: " << m_valueNetwork << std::endl;
    std::cout << "Simulations value: " << m_valueSimulations << " with " << m_timesVisited << " simulations " << std::endl;
    std::cout << "Edge Value: " << m_value << std::endl;
    std::cout << "Virtual Loss: " << m_virtualLoss << std::endl;
    std::cout << std::endl;
}

//double Edge::getSD() const
//{
//    return sqrt((m_valuesSquared - (m_timesVisited *  m_averageValue * m_averageValue))/(m_timesVisited - 1));
//}