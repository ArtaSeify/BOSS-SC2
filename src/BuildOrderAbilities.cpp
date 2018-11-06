#include "BuildOrderAbilities.h"

using namespace BOSS;

BuildOrderAbilities::BuildOrderAbilities()
    : m_typeCount(128, 0)
{

}

void BuildOrderAbilities::add(ActionType type)
{
    BOSS_ASSERT((m_buildOrder.size() == 0) || (type.getRace() == m_buildOrder.back().first.getRace()), "Cannot have a build order with multiple races");

    m_buildOrder.emplace_back(type, AbilityAction());
    m_typeCount[type.getID()]++;
}

void BuildOrderAbilities::add(ActionType type, const AbilityAction & ability)
{
    m_buildOrder.emplace_back(type, ability);
    m_typeCount[type.getID()]++;
}

void BuildOrderAbilities::add(ActionType type, int amount)
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
        add(other[i].first);
    }
}

void BuildOrderAbilities::clear()
{
    m_buildOrder.clear();
    m_typeCount.clear();
}

bool BuildOrderAbilities::empty() const
{
    return size() == 0;
}

/*ActionType BuildOrderAbilities::getAbilityTargetType(size_t index) const
{
    return m_buildOrder[index].second.targetType;
}

size_t BuildOrderAbilities::getAbilityTarget(size_t index) const
{
    return m_buildOrder[index].second.targetID;
}

const AbilityAction & BuildOrderAbilities::getAbilityAction(size_t index) const
{
    return m_buildOrder[index].second;
}*/

size_t BuildOrderAbilities::getTypeCount(ActionType type) const
{
    if (empty())
    {
        return 0;
    }

    BOSS_ASSERT(type.getRace() == m_buildOrder[0].first.getRace(), "Trying to get type count of a different race type");

    return m_typeCount[type.getID()];
}

void BuildOrderAbilities::pop_back()
{
    m_typeCount[m_buildOrder.back().first.getID()]--;
    m_buildOrder.pop_back();
}

const BuildOrderAbilities::ActionTargetPair & BuildOrderAbilities::operator [] (size_t i) const
{
    return m_buildOrder[i];
}

BuildOrderAbilities::ActionTargetPair & BuildOrderAbilities::operator [] (size_t i)
{
    return m_buildOrder[i];
}

const BuildOrderAbilities::ActionTargetPair & BuildOrderAbilities::back() const
{
    return m_buildOrder.back();
}

size_t BuildOrderAbilities::size() const
{
    return m_buildOrder.size();
}

void BuildOrderAbilities::sortByPrerequisites()
{
    for (size_t i(0); i < m_buildOrder.size() - 1; ++i)
    {
        for (size_t j(i + 1); j < m_buildOrder.size(); ++j)
        {
            const auto & recursivePre = m_buildOrder[i].first.getRecursivePrerequisiteActionCount();

            if (recursivePre.contains(m_buildOrder[j].first))
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
        ss << "\"" << m_buildOrder[i].first.getName() << "\"" << (i < m_buildOrder.size() - 1 ? ", " : "");
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

        ss << num.str() << m_buildOrder[i].first.getName() << std::endl;
    }

    return ss.str();
}

std::string BuildOrderAbilities::getIDString() const
{
    std::stringstream ss;

    for (size_t i(0); i < m_buildOrder.size(); ++i)
    {
        ss << (int)m_buildOrder[i].first.getID() << " ";
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
        std::string name = charactersPerName == 0 ? m_buildOrder[i].first.getName() : m_buildOrder[i].first.getName().substr(0, charactersPerName);;
        if (m_buildOrder[i].first.getName() == "ChronoBoost")
        {
            if (charactersPerName == 0)
            {
                name += "_" + m_buildOrder[i].second.targetType.getName();
            }
            else
            {
                name += "_" + m_buildOrder[i].second.targetType.getName().substr(0, charactersPerName);
            }
        }

        ss << name << " ";
    }

    return ss.str();
}