#pragma once

#include "Common.h"
#include <vector>

namespace BOSS
{

class ActionType;
class ActionSet
{
	std::vector<ActionType> m_actions;
    std::vector<size_t>     m_abilityTargets;

public:
    ActionSet();

    size_t size() const;

    bool isEmpty() const;
    bool contains(const ActionType & action) const;
    void add(const ActionType & action);
    void add(const ActionSet & set);
    void add(const ActionType & action, size_t abilityTargetID);
    void remove(const ActionType & action);
    void remove(const ActionSet & set);
    void clear();

          ActionType & operator[] (size_t index);
    const ActionType & operator[] (size_t index) const;

    const size_t & getAbilityTarget(size_t index) const;
    const std::vector<size_t> & getAbilityTargets() const;

    const std::string toString() const;
};

}