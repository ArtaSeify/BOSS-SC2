/* -*- c-basic-offset: 4 -*- */

#include "ActionSet.h"
#include "ActionType.h"
#include "CombatSearchParameters.h"
#include "AbilityAction.h"

using namespace BOSS;

ActionSet::ActionSet()
{
    m_actionsAndTargets.reserve(10);
}

void ActionSet::setNewSet(const ActionSet & newSet)
{
    *this = newSet;
}

bool ActionSet::contains(ActionType action) const
{
    for (const auto & actionTargetPair : m_actionsAndTargets)
    {
        if (actionTargetPair.first == action)
        {
            return true;
        }
    }
    return false;
    //return std::find(m_actionsAndTargets.begin()->first, m_actionsAndTargets.end()->first, action) != m_actionsAndTargets.end()->first;
}

void ActionSet::add(ActionType action)
{
    // can add multiple ability actions because the ability can potentially be used
    // on multiple targets
    if (action.isAbility() || !contains(action))
    {
        m_actionsAndTargets.emplace_back(action, -1);
    }
}

void ActionSet::add(const ActionSet & set)
{
    for (const auto & val : set.m_actionsAndTargets)
    {
        add(val.first, val.second);
    }
}

void ActionSet::add(ActionType action, int abilityTargetID)
{
    if (action.isAbility() || !contains(action))
    {
        m_actionsAndTargets.emplace_back(action, abilityTargetID);
    }
}

// add action and target at a specific index
void ActionSet::add(ActionType action, int abilityTargetID, int index)
{
    if (action.isAbility() || !contains(action))
    {
        m_actionsAndTargets.emplace(m_actionsAndTargets.begin() + index, action, abilityTargetID);
    }
}

void ActionSet::sort(const GameState & state, const CombatSearchParameters & /* params */)
{
    // IMPLEMENT YOUR SORT ALGORITHM HERE.
    BOSS_ASSERT(false, "Sort function is not implemented!");
}

void ActionSet::remove(ActionType action)
{
    //m_actions.erase(std::remove(m_actions.begin(), m_actions.end(), action), m_actions.end());
    for (size_t index(0); index < m_actionsAndTargets.size(); index++)
    {
        if (m_actionsAndTargets[index].first == action)
        {
            m_actionsAndTargets.erase(m_actionsAndTargets.begin() + index);
            index--;
        }
    }
}

// remove the index, given that the action at that index is equal to the action parameter
void ActionSet::remove(ActionType action, int index)
{
    BOSS_ASSERT(action == m_actionsAndTargets[index].first, "argument does not match action in vector");

    m_actionsAndTargets.erase(m_actionsAndTargets.begin() + index);
}

int ActionSet::getAbilityTarget(int index) const
{ 
    return m_actionsAndTargets[index].second; 
}

//  incorrect implementation
void ActionSet::remove(const ActionSet & set)
{
    for (auto & action : set)
    {
        remove(action.first);
    }
}

const ActionSet::ActionTargetPair & ActionSet::getRandomElement() const
{
    return m_actionsAndTargets[std::rand() % size()];
}

const std::string ActionSet::toString() const
{
    std::stringstream ss;

    for (const auto & val : m_actionsAndTargets)
    {
        ss << "Action: " << val.first.getName() << ". Target ID : " << val.second << std::endl;
    }

    return ss.str();
}
