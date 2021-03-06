/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"
#include "AbilityAction.h"

namespace BOSS
{
    class BuildOrder
    {
        using ActionAbilityPair = std::pair<ActionType, AbilityAction>;
        using BuildOrderList = std::vector<ActionAbilityPair>;

        BuildOrderList m_buildOrder;
        std::vector<int> m_typeCount;

    public:

        BuildOrder();

        void add(ActionType type);
        void add(ActionType type, const AbilityAction & ability);
        void add(const ActionAbilityPair & pair);
        void add(ActionType type, int amount);
        void add(const BuildOrder & other);

        void remove(int index);

        void clear();
        const ActionAbilityPair & back() const;
        void pop_back();
        void sortByPrerequisites();

        void print() const;

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
        using iterator = BuildOrderList::iterator;
        using const_iterator = BuildOrderList::const_iterator;
        iterator begin() { return m_buildOrder.begin(); }
        const_iterator begin() const { return m_buildOrder.cbegin(); }
        iterator end() { return m_buildOrder.end(); }
        const_iterator end() const { return m_buildOrder.cend(); }

        // index
        const ActionAbilityPair & operator [] (int i) const;
        ActionAbilityPair & operator [] (int i);
    };

}
