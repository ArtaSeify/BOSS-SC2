/* -*- c-basic-offset: 4 -*- */

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

void BuildOrderAbilities::add(const ActionTargetPair & pair)
{
    m_buildOrder.push_back(pair);
    m_typeCount[pair.first.getID()]++;
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
    for (const auto &x : other) {
        add(x.first);
    }
}

void BuildOrderAbilities::clear()
{
    m_buildOrder.clear();
    m_typeCount.clear();
}

void BuildOrderAbilities::print() const
{
    for (int index = 0; index < m_buildOrder.size(); ++index)
    {
        auto & pair = m_buildOrder[index];
        std::cout << index << " " << pair.first.getName() << ", " << pair.second.targetID << std::endl;
    }
}

int BuildOrderAbilities::getTypeCount(ActionType type) const
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

const BuildOrderAbilities::ActionTargetPair & BuildOrderAbilities::operator [] (int i) const
{
    return m_buildOrder[i];
}

BuildOrderAbilities::ActionTargetPair & BuildOrderAbilities::operator [] (int i)
{
    return m_buildOrder[i];
}

const BuildOrderAbilities::ActionTargetPair & BuildOrderAbilities::back() const
{
    return m_buildOrder.back();
}

void BuildOrderAbilities::sortByPrerequisites()
{
    for (int i(0); i < (int)m_buildOrder.size() - 1; ++i)
    {
        for (int j(i + 1); j < (int)m_buildOrder.size(); ++j)
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

    for (const auto &x : m_buildOrder)
    {
        ss << (int)x.first.getID() << " ";
    }

    return ss.str();
}

std::string BuildOrderAbilities::getNameString(int charactersPerName, int printUpToIndex, bool withComma) const
{
    std::stringstream ss;

    if (printUpToIndex == -1)
    {
        printUpToIndex = int(m_buildOrder.size());
    }
    
    for (int i(0); i < printUpToIndex; ++i)
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

        ss << name;

        if (withComma)
        {
            ss << ",";
        }

        else
        {
            ss << " ";
        }
    }

    return ss.str();
}
