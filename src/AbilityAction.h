/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"

namespace BOSS
{
    struct AbilityAction
    {
        ActionType  type;                // type of the ability
        TimeType    frameCast;           // the frame the ability was cast
        NumUnits    targetID;            // index inside m_units of GameState class
        NumUnits    targetProductionID;  // index inside m_units of the unit being produced by the target of this ability
        ActionType  targetType;          // type of unit it was used on


        AbilityAction()
        : type(ActionTypes::None)
        , frameCast(0)
        , targetID(0)
        , targetProductionID(0)
        , targetType(0) 
        {
        
        }

        AbilityAction(ActionType newType, TimeType newFrameCast, NumUnits newTargetID, NumUnits newTargetProductionID, ActionType newTargetType)
        {
            type = newType;
            frameCast = newFrameCast;
            targetID = newTargetID;
            targetProductionID = newTargetProductionID;
            targetType = newTargetType;
        }

        void writeToSS(std::stringstream & ss) const;
    };
}

