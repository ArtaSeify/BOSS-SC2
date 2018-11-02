#include "ActionSetAbilities.h"
#include "ActionType.h"

using namespace BOSS;

ActionSetAbilities::ActionSetAbilities()
{
    m_actionsAndTargets.reserve(10);
}

bool ActionSetAbilities::contains(const ActionType & action) const
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

void ActionSetAbilities::add(const ActionType & action)
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

void ActionSetAbilities::add(const ActionType & action, size_t abilityTargetID)
{
    add(action);
    m_actionsAndTargets.back().second = abilityTargetID;
}

void ActionSetAbilities::remove(const ActionType & action)
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

size_t ActionSetAbilities::getAbilityTarget(size_t index) const
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