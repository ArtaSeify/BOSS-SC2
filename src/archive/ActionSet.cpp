/* -*- c-basic-offset: 4 -*- */

#include "ActionSet.h"
#include "ActionType.h"

using namespace BOSS;

ActionSet::ActionSet()
{
}

int ActionSet::size() const
{
    return int(m_actions.size());
}

bool ActionSet::isEmpty() const
{
    return m_actions.empty();
}

bool ActionSet::contains(ActionType action) const
{
    return std::find(m_actions.begin(), m_actions.end(), action) != m_actions.end();
}

void ActionSet::add(ActionType action)
{
    if (!contains(action))
    {
        m_actions.push_back(action);
    }
}

void ActionSet::add(const ActionSet & set)
{
    for (const auto & val : set.m_actions)
    {
        add(val);
    }
}

void ActionSet::remove(ActionType action)
{
    m_actions.erase(std::remove(m_actions.begin(), m_actions.end(), action), m_actions.end());
}

void ActionSet::remove(const ActionSet & set)
{
    for (const auto & val : set.m_actions)
    {
        add(val);
    }
}

const std::string ActionSet::toString() const
{
    std::stringstream ss;
    
    for (const auto & val : m_actions)
    {
        ss << "PreReq: " << val.getName() << std::endl;
    }

    return ss.str();
}

void ActionSet::clear()
{
    m_actions.clear();
}

ActionType & ActionSet::operator[] (int index)
{
    return m_actions[index];
}

ActionType ActionSet::operator[] (int index) const
{
    return m_actions[index];
}
