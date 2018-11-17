/* -*- c-basic-offset: 4 -*- */

#include "BuildOrder.h"

using namespace BOSS;

BuildOrder::BuildOrder()
    : m_typeCount(128, 0)
{

}

void BuildOrder::add(ActionType type)
{
    BOSS_ASSERT((m_buildOrder.size() == 0) || (type.getRace() == m_buildOrder.back().getRace()), "Cannot have a build order with multiple races");

    m_buildOrder.push_back(type);
    m_typeCount[type.getID()]++;
}

void BuildOrder::add(ActionType type, const AbilityAction & ability)
{
    add(type);
    if (type.isAbility())
    {
        m_abilityTargets[m_buildOrder.size() - 1] = ability;
    }
}

void BuildOrder::add(ActionType type, int amount)
{
    for (int i(0); i < amount; ++i)
    {
        add(type);
    }
}

void BuildOrder::add(const BuildOrder & other)
{
    for (size_t i(0); i < other.size(); ++i)
    {
        add(other[i]);
    }
}

void BuildOrder::clear()
{
    m_buildOrder.clear();
    m_typeCount.clear();
}

/*ActionType BuildOrder::getAbilityTargetType(size_t index) const
{
    return m_abilityTargets.at(index).targetType;
}

const size_t & BuildOrder::getAbilityTarget(size_t index) const
{
    return m_abilityTargets.at(index).targetID;
}

const AbilityAction & BuildOrder::getAbilityAction(size_t index) const
{
    return m_abilityTargets.at(index);
}*/

int BuildOrder::getTypeCount(ActionType type) const
{
    if (empty())
    {
        return 0;
    }

    BOSS_ASSERT(type.getRace() == m_buildOrder[0].getRace(), "Trying to get type count of a different race type");

    return m_typeCount[type.getID()];
}

void BuildOrder::pop_back()
{
    if ((--m_buildOrder.end())->isAbility())
    {
        m_abilityTargets.erase(--m_abilityTargets.end());
    }
    m_buildOrder.pop_back();
    
}

void BuildOrder::sortByPrerequisites()
{
    for (size_t i(0); i < m_buildOrder.size() - 1; ++i)
    {
        for (size_t j(i + 1); j < m_buildOrder.size(); ++j)
        {
            const auto & recursivePre = m_buildOrder[i].getRecursivePrerequisiteActionCount();

            if (recursivePre.contains(m_buildOrder[j]))
            {
                std::swap(m_buildOrder[i], m_buildOrder[j]);
            }
        }
    }
}

std::string BuildOrder::getJSONString() const
{
    std::stringstream ss;

    ss << "\"Build Order\" : [";

    for (size_t i(0); i < m_buildOrder.size(); ++i)
    {
        ss << "\"" << m_buildOrder[i].getName() << "\"" << (i < m_buildOrder.size() - 1 ? ", " : "");
    }

    ss << "]";

    return ss.str();
}

std::string BuildOrder::getNumberedString() const
{
    std::stringstream ss;

    for (size_t i(0); i < m_buildOrder.size(); ++i)
    {
        std::stringstream num;
        num << i;
        while (num.str().length() < 5)
        {
            num << " ";
        }

        ss << num.str() << m_buildOrder[i].getName() << std::endl;
    }

    return ss.str();
}

std::string BuildOrder::getIDString() const
{
    std::stringstream ss;

    for (size_t i(0); i < m_buildOrder.size(); ++i)
    {
        ss << (int)m_buildOrder[i].getID() << " ";
    }

    return ss.str();
}

std::string BuildOrder::getNameString(int charactersPerName, int printUpToIndex) const
{
    std::stringstream ss;

#if 0
    //!!!PROBLEM never true
    if (printUpToIndex == -1)
    {
        printUpToIndex = m_buildOrder.size();
    }
#endif
    
    for (int i(0); i < printUpToIndex; ++i)
    {
        std::string name = charactersPerName == 0 ? m_buildOrder[i].getName() : m_buildOrder[i].getName().substr(0, charactersPerName);;
        if (m_buildOrder[i].getName() == "ChronoBoost")
        {
            if (charactersPerName == 0)
            {
                name += "_" + m_abilityTargets.at(i).targetType.getName();
            }
            else
            {
                name += "_" + m_abilityTargets.at(i).targetType.getName().substr(0, charactersPerName);
            }
        }

        ss << name << " ";
    }
    
    return ss.str();
}
