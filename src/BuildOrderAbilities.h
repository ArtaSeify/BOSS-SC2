/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"
#include "AbilityAction.h"

namespace BOSS
{
    class BuildOrderAbilities
    {
        using ActionTargetPair = std::pair<ActionType, AbilityAction>;
        using BuildOrder = std::vector<ActionTargetPair>;

        BuildOrder m_buildOrder;
        std::vector<int> m_typeCount;

    public:

        BuildOrderAbilities();

        void add(ActionType type);
        void add(ActionType type, const AbilityAction & ability);
        void add(const ActionTargetPair & pair);
        void add(ActionType type, int amount);
        void add(const BuildOrderAbilities & other);

        void clear();
        const ActionTargetPair & back() const;
        void pop_back();
        void sortByPrerequisites();

        int size() const { return int(m_buildOrder.size()); }
        int getTypeCount(ActionType type) const;
        bool empty() const { return size() == 0; }
        //ActionType          getAbilityTargetType(size_t index) const;
        //size_t                      getAbilityTarget(size_t index) const;
        //const AbilityAction &       getAbilityAction(size_t index) const;

        std::string getJSONString() const;
        std::string getNumberedString() const;
        std::string getIDString() const;
        std::string getNameString(int charactersPerName = 0, int printUpToIndex = -1, bool withComma = false) const;
    
        // iterator
        using iterator = BuildOrder::iterator;
        using const_iterator = BuildOrder::const_iterator;
        iterator begin() { return m_buildOrder.begin(); }
        const_iterator begin() const { return m_buildOrder.begin(); }
        iterator end() { return m_buildOrder.end(); }
        const_iterator end() const { return m_buildOrder.end(); }

        // index
        const ActionTargetPair & operator [] (int i) const;
        ActionTargetPair & operator [] (int i);
    };

}
