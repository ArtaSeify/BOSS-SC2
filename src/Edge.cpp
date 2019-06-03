#include "Edge.h"
#include "Node.h"

using namespace BOSS;

FracType Edge::CURRENT_HIGHEST_VALUE = 1.f;
int Edge::NODE_VISITS_BEFORE_EXPAND = 30;
bool Edge::USE_MAX_VALUE = true;

Edge::Edge()
    : m_timesVisited(0)
    , m_valueSimulations(0)
    , m_valueNetwork(0)
    , m_value(0)
    , m_averageValue(0)
    , m_maxValue(0)
    , m_policyValue(0)
    , m_action(ActionAbilityPair(ActionTypes::None, AbilityAction()))
    , m_child()
    , m_parent()
{

}

Edge::Edge(const ActionAbilityPair & action, std::shared_ptr<Node> parent)
    : m_timesVisited(0)
    , m_valueSimulations(0)
    , m_valueNetwork(0)
    , m_value(0)
    , m_averageValue(0)
    , m_maxValue(0)
    , m_policyValue(0)
    , m_action(action)
    , m_child()
    , m_parent(parent)
{

}

void Edge::cleanUp()
{
    //std::cout << "cleaning up edge: " << m_action.first.getName() << std::endl;
    if (m_child != nullptr)
    {
        //std::cout << "cleaning up child of this edge" << std::endl;
        m_child->cleanUp();
        //std::cout << "child use count: " << m_child.use_count() << std::endl;
        m_child.reset();
    }
    //std::cout << "parent use count: " << m_parent.use_count() << std::endl;
    m_parent.reset();
}

void Edge::reset()
{
    m_timesVisited = 0;
    m_valueSimulations = 0;
    m_valueNetwork = 0;
    m_value = 0;
    m_averageValue = 0;
    m_maxValue = 0;
    m_policyValue = 0;
    m_parent.reset();
}

void Edge::updateEdge(FracType simulationValue)
{
    //std::cout << m_action.first.getName() << " node updated" << std::endl;

    m_timesVisited++;

    m_maxValue = std::max(simulationValue, m_maxValue);
    m_averageValue = m_averageValue + ((1.f / m_timesVisited) * (simulationValue - m_averageValue));

    if (USE_MAX_VALUE)
    {
        if (m_valueSimulations < m_maxValue)
        {
            m_valueSimulations = m_maxValue;
            setNewEdgeValue();
        }
    }
    else
    {
        m_valueSimulations = m_averageValue;
        setNewEdgeValue();
    }
}

void Edge::setChild(std::shared_ptr<Node> node) 
{ 
    node.swap(m_child); 
}

void Edge::setNewEdgeValue()
{
    //m_value = (MIXING_PARAMETER * m_valueNetwork) + ((1 - MIXING_PARAMETER)*m_valueSimulations);
    m_value = m_valueSimulations;

    if (m_value > CURRENT_HIGHEST_VALUE)
    {
        CURRENT_HIGHEST_VALUE = m_value;
    }
}

void Edge::printValues() const
{
    std::cout << "Edge action: " << m_action.first.getName() << std::endl;
    std::cout << "Network Value: " << m_valueNetwork << std::endl;
    std::cout << "Simulations value: " << m_valueSimulations << " with " << m_timesVisited << " simulations " << std::endl;
    std::cout << "Edge Value: " << m_value << std::endl;
}

//double Edge::getSD() const
//{
//    return sqrt((m_valuesSquared - (m_timesVisited *  m_averageValue * m_averageValue))/(m_timesVisited - 1));
//}