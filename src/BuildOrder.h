/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"
#include "AbilityAction.h"

namespace BOSS
{

    class BuildOrder
    {
        std::vector<ActionType>	     m_buildOrder;
        std::vector<int>             m_typeCount;
        std::map<int, AbilityAction> m_abilityTargets;

    public:

        BuildOrder();

        void add(ActionType type);
        void add(ActionType type, const AbilityAction & ability);
        void add(ActionType type, int amount);
        void add(const BuildOrder & other);
        void clear();
        void pop_back();
        void sortByPrerequisites();

        // iterator
        using iterator = std::vector<ActionType>::iterator;
        using const_iterator = std::vector<ActionType>::const_iterator;
        iterator begin() { return m_buildOrder.begin(); }
        iterator end() { return m_buildOrder.end(); }
        const_iterator begin() const { return m_buildOrder.begin(); }
        const_iterator end() const { return m_buildOrder.end(); }

        // index
        ActionType operator [] (size_t i) const
        {
            assert(i < size());
            return m_buildOrder[i];
        }

        ActionType & operator [] (size_t i) 
        {
            assert(i < size());
            return m_buildOrder[i];
        }

        ActionType back() const { return m_buildOrder.back(); }

        size_t size() const { return m_buildOrder.size(); }
  
        bool empty() const { return size() == 0; }

        int getTypeCount(ActionType type) const;

        //ActionType      getAbilityTargetType(size_t index) const;
        //const size_t &          getAbilityTarget(size_t index) const;
        //const AbilityAction &   getAbilityAction(size_t index) const;

        std::string getJSONString() const;
        std::string getNumberedString() const;
        std::string getIDString() const;
        std::string getNameString(int charactersPerName = 0, int printUpToIndex = -1) const;
    };

}
