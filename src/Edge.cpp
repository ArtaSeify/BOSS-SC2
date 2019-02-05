#include "Edge.h"
#include "Node.h"

using namespace BOSS;

FracType Edge::CURRENT_HIGHEST_VALUE = 1.f;
int Edge::NODE_VISITS_BEFORE_EXPAND = 1;

Edge::Edge()
    : m_timesVisited(0)
    , m_value(0)
    , m_action(std::pair<ActionType, NumUnits>(ActionTypes::None, 0))
    , m_child()
    , m_parent()
{

}

Edge::Edge(const Action & action, std::shared_ptr<Node> parent)
    : m_timesVisited(0)
    , m_value(0)
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

void Edge::updateEdge(FracType newActionValue)
{
    //std::cout << m_action.first.getName() << " node updated" << std::endl;

    m_timesVisited++;
    //m_value = m_value + ((1.f / m_timesVisited) * (newActionValue - m_value));
    m_value = std::max(m_value, newActionValue);

    if (m_value > CURRENT_HIGHEST_VALUE)
    {
        CURRENT_HIGHEST_VALUE = m_value;
    }
}

void Edge::setChild(std::shared_ptr<Node> node) 
{ 
    node.swap(m_child); 
}