/* -*- c-basic-offset: 4 -*- */

#include "ActionSetAbilities.h"
#include "ActionType.h"
#include "CombatSearchParameters.h"

using namespace BOSS;

ActionSetAbilities::ActionSetAbilities()
{
    m_actionsAndTargets.reserve(10);
}

void ActionSetAbilities::setNewSet(const ActionSetAbilities & newSet)
{
    *this = newSet;
}

bool ActionSetAbilities::contains(ActionType action) const
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

void ActionSetAbilities::add(ActionType action)
{
    // can add multiple ability actions because the ability can potentially be used
    // on multiple targets
    if (action.isAbility() || !contains(action))
    {
        m_actionsAndTargets.emplace_back(action, -1);
    }
}

void ActionSetAbilities::add(const ActionSetAbilities & set)
{
    for (const auto & val : set.m_actionsAndTargets)
    {
        add(val.first, val.second);
    }
}

void ActionSetAbilities::add(ActionType action, NumUnits abilityTargetID)
{
    if (action.isAbility() || !contains(action))
    {
        m_actionsAndTargets.emplace_back(action, abilityTargetID);
    }
}

// add action and target at a specific index
void ActionSetAbilities::add(ActionType action, NumUnits abilityTargetID, size_t index)
{
    if (action.isAbility() || !contains(action))
    {
        m_actionsAndTargets.emplace(m_actionsAndTargets.begin() + index, action, abilityTargetID);
    }
}

void ActionSetAbilities::sort(const GameState & state, const CombatSearchParameters & /* params */)
{
    //std::cout << "Before sorting:" << std::endl;
    //std::cout << toString() << std::endl;
    const short SupplyHeuristic = 4;

    ActionSetAbilities sortedSet;

    const int totalSupply = state.getCurrentSupply() + state.getSupplyInProgress();
    
    // if we have little supply free, give priority to supply providing units
    // ie. pylon/supply depot/overlord and nexus/command center/hatchery
    if (state.getMaxSupply() - totalSupply <= SupplyHeuristic)
    {
        for (size_t i(0); i < m_actionsAndTargets.size(); ++i)
        {
            auto & actionTargetPair = m_actionsAndTargets[i];
            auto & type = actionTargetPair.first;
            if (type.isSupplyProvider() || type.isDepot())
            {
                sortedSet.add(type, actionTargetPair.second);
                m_actionsAndTargets.erase(m_actionsAndTargets.begin() + i);
                i--;
            }
        }
    }

    // add combat units
    for (size_t i(0); i < m_actionsAndTargets.size(); ++i)
    {
        auto & actionTargetPair = m_actionsAndTargets[i];
        auto & type = actionTargetPair.first;
        if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
        {
            sortedSet.add(type, actionTargetPair.second);
            m_actionsAndTargets.erase(m_actionsAndTargets.begin() + i);
            i--;
        }
    }

    // add worker
    for (size_t i(0); i < m_actionsAndTargets.size(); ++i)
    {
        auto & actionTargetPair = m_actionsAndTargets[i];
        auto & type = actionTargetPair.first;
        if (type.isWorker())
        {
            sortedSet.add(type, actionTargetPair.second);
            m_actionsAndTargets.erase(m_actionsAndTargets.begin() + i);
            break;
        }
    }

    // add the rest
    sortedSet.add(*this);

    setNewSet(sortedSet);

    //std::cout << "After sorting:" << std::endl;
    //std::cout << sortedSet.toString() << std::endl;
}

void ActionSetAbilities::remove(ActionType action)
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
void ActionSetAbilities::remove(ActionType action, size_t index)
{
    BOSS_ASSERT(action == m_actionsAndTargets[index].first, "argument does not match action in vector");

    m_actionsAndTargets.erase(m_actionsAndTargets.begin() + index);
}

NumUnits ActionSetAbilities::getAbilityTarget(size_t index) const
{ 
    return m_actionsAndTargets[index].second; 
}

//  incorrect implementation
/*void ActionSetAbilities::remove(const ActionSetAbilities & set)
{
    for (const auto & val : set.m_actions)
    {
        add(val);
    }
}
*/

const std::string ActionSetAbilities::toString() const
{
    std::stringstream ss;

    for (const auto & val : m_actionsAndTargets)
    {
        ss << "Action: " << val.first.getName() << std::endl;
        ss << "Target ID: " << val.second << std::endl;
    }

    return ss.str();
}
