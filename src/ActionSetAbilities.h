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

        int size() const { return m_actionsAndTargets.size(); }
        bool isEmpty() const { return m_actionsAndTargets.empty(); }
        void clear() { m_actionsAndTargets.clear(); }

        bool contains(ActionType action) const;

        void add(ActionType action);
        void add(const ActionSetAbilities & set);
        void add(ActionType action, int abilityTargetID);
        void add(ActionType action, int abilityTargetID, int index);

        void sort(const GameState & state, const CombatSearchParameters & params);

        void remove(ActionType action);
        void remove(ActionType action, int index);
        void remove(const ActionSetAbilities & set);

        int getAbilityTarget(int index) const;
        const std::string toString() const;

        // iterator
        using iterator = Actions::iterator;
        using const_iterator = Actions::const_iterator;
        iterator begin() { return m_actionsAndTargets.begin(); }
        const_iterator begin() const { return m_actionsAndTargets.begin(); }
        iterator end() { return m_actionsAndTargets.end(); }
        const_iterator end() const { return m_actionsAndTargets.end(); }

        // index
        ActionTargetPair & operator[] (int index) { return m_actionsAndTargets[index]; }
        const ActionTargetPair & operator[] (int index) const { return m_actionsAndTargets[index]; }
    };
}
