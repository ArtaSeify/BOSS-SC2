#pragma once

#include "Common.h"
#include "ActionType.h"
#include "GameState.h"


namespace BOSS
{

class BuildOrder
{
    std::vector<ActionType>	m_buildOrder;
    std::vector<size_t>		m_typeCount;
    std::vector<size_t>     m_abilityTargets;

public:

    BuildOrder();

    void                    add(const ActionType & type);
    void                    add(const ActionType & type, const size_t & abilityTargetID);
    void                    add(const ActionType & type, const int & amount);
    void                    add(const BuildOrder & other);
    void                    clear();
    const ActionType &      back() const;
    void                    pop_back();
    void                    sortByPrerequisites();
    
    const ActionType &      operator [] (const size_t & i) const;
    ActionType &            operator [] (const size_t & i);

    const size_t            size() const;
    const size_t            getTypeCount(const ActionType & type) const;
    const bool              empty() const;
    const size_t &          getAbilityTarget(const size_t & index) const;

    std::string             getJSONString() const;
    std::string             getNumberedString() const;
    std::string             getIDString() const;
    std::string             getNameString(const size_t charactersPerName = 0) const;
    std::string             getNameString(const GameState & state, const size_t charactersPerName = 0, const size_t printUpToIndex = -1) const;
};

}