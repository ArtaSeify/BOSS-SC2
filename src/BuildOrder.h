#pragma once

#include "Common.h"
#include "ActionType.h"


namespace BOSS
{

    class BuildOrder
    {
        std::vector<ActionType>    m_buildOrder;
        std::vector<size_t>        m_typeCount;

    public:

        BuildOrder();

        void                    add(const ActionType & type);
        void                    add(const ActionType & type, int amount);
        void                    add(const BuildOrder & other);
        void                    clear();
        void                    pop_back();
        void                    sortByPrerequisites();

        const ActionType &      operator [] (size_t i) const;
        ActionType &            operator [] (size_t i);

        size_t            size() const;
        size_t            getTypeCount(ActionType type) const;
        bool              empty() const;

        std::string             getJSONString() const;
        std::string             getNumberedString() const;
        std::string             getIDString() const;
        std::string             getNameString(const size_t charactersPerName = 0) const;

        // iterator
        using iterator = std::vector<ActionType>::iterator;
        using const_iterator = std::vector<ActionType>::const_iterator;
        iterator begin() { return m_buildOrder.begin(); }
        const_iterator begin() const { return m_buildOrder.begin(); }
        iterator end() { return m_buildOrder.end(); }
        const_iterator end() const { return m_buildOrder.end(); }
    };

}