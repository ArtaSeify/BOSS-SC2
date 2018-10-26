#pragma once

#include "Common.h"
#include "ActionType.h"

namespace BOSS
{
    struct AbilityAction
    {
        size_t targetID;        // index inside m_units of GameState class
        ActionType type;        // type of unit it was used on

        AbilityAction(): targetID(0), type(0) {}
        AbilityAction(size_t target, const ActionType & action)
        {
            targetID = target;
            type = action;
        }
    };
}

