/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
//#include "ActionType.h"
//#include "CombatSearchParameters.h"

namespace BOSS
{
    class ActionType;
    class CombatSearchParameters;
    class ActionSetAbilities
    {
        typedef std::pair<ActionType, NumUnits> ActionTargetPair;
        typedef std::vector<ActionTargetPair> Actions;
        Actions m_actionsAndTargets;

    public:
        ActionSetAbilities();
        void setNewSet(const ActionSetAbilities & newSet);

        size_t size() const { return m_actionsAndTargets.size(); }
        bool isEmpty() const { return m_actionsAndTargets.empty(); }
        void clear() { m_actionsAndTargets.clear(); }

        bool contains(ActionType action) const;

        void add(ActionType action);
        void add(const ActionSetAbilities & set);
        void add(ActionType action, NumUnits abilityTargetID);
        void add(ActionType action, NumUnits abilityTargetID, int index);

        void sort(const GameState & state, const CombatSearchParameters & params);

        void remove(ActionType action);
        void remove(ActionType action, int index);
        void remove(const ActionSetAbilities & set);

        int getAbilityTarget(int index) const;
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
