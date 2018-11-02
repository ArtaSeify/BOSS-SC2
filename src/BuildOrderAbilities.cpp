#include "BuildOrderAbilities.h"

using namespace BOSS;

BuildOrderAbilities::BuildOrderAbilities()
    : m_typeCount(128, 0)
{

}

void BuildOrderAbilities::add(const ActionType & type)
{
    BOSS_ASSERT((m_buildOrder.size() == 0) || (type.getRace() == m_buildOrder.back().first.getRace()), "Cannot have a build order with multiple races");

    m_buildOrder.emplace_back(type, AbilityAction());
    m_typeCount[type.getID()]++;
}

void BuildOrderAbilities::add(const ActionType & type, const AbilityAction & ability)
{
    m_buildOrder.emplace_back(type, ability);
    m_typeCount[type.getID()]++;
}

void BuildOrderAbilities::add(const ActionType & type, int amount)
{
    for (int i(0); i < amount; ++i)
    {
        add(type);
    }
}

void BuildOrderAbilities::add(const BuildOrderAbilities & other)
{
    for (size_t i(0); i < other.size(); ++i)
    {
        add(other[i]);
    }
}

void BuildOrderAbilities::clear()
{
    m_buildOrder.clear();
    m_typeCount.clear();
}

const bool BuildOrderAbilities::empty() const
{
    return size() == 0;
}

const ActionType & BuildOrderAbilities::getAbilityTargetType(size_t index) const
{
    return m_abilityTargets.at(index).targetType;
}

const size_t & BuildOrderAbilities::getAbilityTarget(size_t index) const
{
    return m_abilityTargets.at(index).targetID;
}

const AbilityAction & BuildOrderAbilities::getAbilityAction(size_t index) const
{
    return m_abilityTargets.at(index);
}

const size_t BuildOrderAbilities::getTypeCount(const ActionType & type) const
{
    if (empty())
    {
        return 0;
    }

    BOSS_ASSERT(type.getRace() == m_buildOrder[0].getRace(), "Trying to get type count of a different race type");

    return m_typeCount[type.getID()];
}

void BuildOrderAbilities::pop_back()
{
    if ((--m_buildOrder.end())->isAbility())
    {
        m_abilityTargets.erase(--m_abilityTargets.end());
    }
    m_buildOrder.pop_back();

}

const ActionType & BuildOrderAbilities::operator [] (size_t i) const
{
    return m_buildOrder[i];
}

ActionType & BuildOrderAbilities::operator [] (size_t i)
{
    return m_buildOrder[i];
}

const ActionType & BuildOrderAbilities::back() const
{
    return m_buildOrder.back();
}

const size_t BuildOrderAbilities::size() const
{
    return m_buildOrder.size();
}

void BuildOrderAbilities::sortByPrerequisites()
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

std::string BuildOrderAbilities::getJSONString() const
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

std::string BuildOrderAbilities::getNumberedString() const
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

std::string BuildOrderAbilities::getIDString() const
{
    std::stringstream ss;

    for (size_t i(0); i < m_buildOrder.size(); ++i)
    {
        ss << (int)m_buildOrder[i].getID() << " ";
    }

    return ss.str();
}

std::string BuildOrderAbilities::getNameString(size_t charactersPerName, size_t printUpToIndex) const
{
    std::stringstream ss;

    if (printUpToIndex == -1)
    {
        printUpToIndex = m_buildOrder.size();
    }

    for (size_t i(0); i < printUpToIndex; ++i)
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