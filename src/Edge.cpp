#include "Edge.h"

using namespace BOSS;

Edge::Edge()
    : m_timesVisited(0)
    , m_value(0)
    , m_action(std::pair<ActionType, NumUnits>(ActionTypes::None, 0))
    , m_parent()
    , m_child()
{

}

Edge::Edge(const Action & action, Node * parent, Node * child)
    : m_timesVisited(0)
    , m_value(0)
    , m_action(action)
    , m_parent(parent)
    , m_child(child)
{

}