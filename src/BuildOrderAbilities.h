#pragma once

#include "Common.h"
#include "ActionType.h"
#include "AbilityAction.h"

namespace BOSS
{

    class BuildOrderAbilities
    {
        typedef std::pair<ActionType, AbilityAction>    ActionTargetPair;
        typedef std::vector<ActionTargetPair>           BuildOrder;
        BuildOrder                                      m_buildOrder;
        std::vector<ActionID>		                    m_typeCount;

    public:

        BuildOrderAbilities();

        void                        add(ActionType type);
        void                        add(ActionType type, const AbilityAction & ability);
        void                        add(ActionType type, int amount);
        void                        add(const BuildOrderAbilities & other);
        void                        clear();
        const ActionTargetPair &    back() const;
        void                        pop_back();
        void                        sortByPrerequisites();

        size_t                      size() const;
        int                         getTypeCount(ActionType type) const;
        bool                        empty() const;
        //ActionType          getAbilityTargetType(size_t index) const;
        //size_t                      getAbilityTarget(size_t index) const;
        //const AbilityAction &       getAbilityAction(size_t index) const;

        std::string                 getJSONString() const;
        std::string                 getNumberedString() const;
        std::string                 getIDString() const;
        std::string                 getNameString(size_t charactersPerName = 0, size_t printUpToIndex = -1) const;
    
        // iterator
        typedef BuildOrder::iterator iterator;
        typedef BuildOrder::const_iterator const_iterator;
        iterator begin() { return m_buildOrder.begin(); }
        const_iterator begin() const { return m_buildOrder.begin(); }
        iterator end() { return m_buildOrder.end(); }
        const_iterator end() const { return m_buildOrder.end(); }

        // index
        const ActionTargetPair &      operator [] (size_t i) const;
        ActionTargetPair &            operator [] (size_t i);
    };

}