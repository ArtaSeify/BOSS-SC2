#pragma once

#include "Common.h"
#include "ActionType.h"
#include "AbilityAction.h"

namespace BOSS
{

class BuildOrder
{
    std::vector<ActionType>	        m_buildOrder;
    std::vector<size_t>		        m_typeCount;
    std::map<size_t, AbilityAction> m_abilityTargets;

public:

    BuildOrder();

    void                    add(const ActionType & type);
    void                    add(const ActionType & type, const AbilityAction & ability);
    void                    add(const ActionType & type, int amount);
    void                    add(const BuildOrder & other);
    void                    clear();
    const ActionType &      back() const;
    void                    pop_back();
    void                    sortByPrerequisites();

    // iterator
    typedef std::vector<ActionType>::iterator iterator;
    typedef std::vector<ActionType>::const_iterator const_iterator;
    iterator begin() { return m_buildOrder.begin(); }
    iterator end() { return m_buildOrder.end(); }
    
    const ActionType &      operator [] (size_t i) const;
    ActionType &            operator [] (size_t i);

    const size_t            size() const;
    const size_t            getTypeCount(const ActionType & type) const;
    const bool              empty() const;
    const ActionType &      getAbilityTargetType(size_t index) const;
    const size_t &          getAbilityTarget(size_t index) const;
    const AbilityAction &   getAbilityAction(size_t index) const;

    std::string             getJSONString() const;
    std::string             getNumberedString() const;
    std::string             getIDString() const;
    std::string             getNameString(size_t charactersPerName = 0, size_t printUpToIndex = -1) const;
};

}