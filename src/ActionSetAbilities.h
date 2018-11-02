#pragma once

#include "Common.h"
#include <vector>

namespace BOSS
{
class ActionType;
class ActionSetAbilities
{
    typedef std::pair<ActionType, size_t> ActionTargetPair;
    typedef std::vector<ActionTargetPair> Actions;
    Actions m_actionsAndTargets;

public:
    ActionSetAbilities();

    size_t size() const { return m_actionsAndTargets.size(); }
    bool isEmpty() const { return m_actionsAndTargets.empty(); }
    void clear() { m_actionsAndTargets.clear(); }

    bool contains(const ActionType & action) const;

    void add(const ActionType & action);
    void add(const ActionSetAbilities & set);
    void add(const ActionType & action, size_t abilityTargetID);

    void remove(const ActionType & action);
    void remove(const ActionSetAbilities & set);

    size_t getAbilityTarget(size_t index) const;

    const std::string toString() const;

    // iterator
    typedef Actions::iterator iterator;
    typedef Actions::const_iterator const_iterator;
    iterator begin() { return m_actionsAndTargets.begin(); }
    const_iterator begin() const { return m_actionsAndTargets.begin(); }
    iterator end() { return m_actionsAndTargets.end(); }
    const_iterator end() const { return m_actionsAndTargets.end(); }

    // index
    ActionTargetPair & operator[] (size_t index) { return m_actionsAndTargets[index]; }
    const ActionTargetPair & operator[] (size_t index) const { return m_actionsAndTargets[index]; }
};

}