#pragma once

#include "Common.h"
#include <vector>

namespace BOSS
{
class ActionType;
class ActionSetAbilities
{
    typedef std::pair<ActionType, uint4> ActionTargetPair;
    typedef std::vector<ActionTargetPair> Actions;
    Actions m_actionsAndTargets;

public:
    ActionSetAbilities();

    size_t size() const { return m_actionsAndTargets.size(); }
    bool isEmpty() const { return m_actionsAndTargets.empty(); }
    void clear() { m_actionsAndTargets.clear(); }

    bool contains(ActionType action) const;

    void add(ActionType action);
    void add(const ActionSetAbilities & set);
    void add(ActionType action, uint4 abilityTargetID);

    void remove(ActionType action);
    void remove(const ActionSetAbilities & set);

    uint4 getAbilityTarget(uint4 index) const;

    const std::string toString() const;

    // iterator
    typedef Actions::iterator iterator;
    typedef Actions::const_iterator const_iterator;
    iterator begin() { return m_actionsAndTargets.begin(); }
    const_iterator begin() const { return m_actionsAndTargets.begin(); }
    iterator end() { return m_actionsAndTargets.end(); }
    const_iterator end() const { return m_actionsAndTargets.end(); }

    // index
    ActionTargetPair & operator[] (uint4 index) { return m_actionsAndTargets[index]; }
    const ActionTargetPair & operator[] (uint4 index) const { return m_actionsAndTargets[index]; }
};

}