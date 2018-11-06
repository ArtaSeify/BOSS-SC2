#pragma once

#include "Common.h"
#include "ActionType.h"

namespace BOSS
{
    struct AbilityAction
    {
        ActionType  type;                // type of the ability
        size_t      frameCast;           // the frame the ability was cast
        size_t      targetID;            // index inside m_units of GameState class
        size_t      targetProductionID;  // index inside m_units of the unit being produced by the target of this ability
        ActionType  targetType;          // type of unit it was used on


        AbilityAction(): type(0), frameCast(0), targetID(0), targetProductionID(0), targetType(0) {}
        /*AbilityAction(size_t id, size_t target, ActionType action)
        {
            targetID = target;
            type = action;
        }*/

        AbilityAction(ActionType newType, size_t newFrameCast, size_t newTargetID, size_t newTargetProductionID, ActionType newTargetType)
        {
            type = newType;
            frameCast = newFrameCast;
            targetID = newTargetID;
            targetProductionID = newTargetProductionID;
            targetType = newTargetType;
        }
    };
}

