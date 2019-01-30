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
    , m_parent(std::shared_ptr<Node>(parent))
{

}

void Edge::updateEdge(FracType newActionValue)
{
    //std::cout << m_action.first.getName() << " node updated" << std::endl;

    m_timesVisited++;
    m_value = m_value + ((1.f / m_timesVisited) * (newActionValue - m_value));

    if (m_value > CURRENT_HIGHEST_VALUE)
    {
        CURRENT_HIGHEST_VALUE = m_value;
    }
}

void Edge::setChild(std::shared_ptr<Node> node) 
{ 
    node.swap(m_child); 
}