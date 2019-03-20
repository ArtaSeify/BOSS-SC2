/* -*- c-basic-offset: 4 -*- */

#include "AbilityAction.h"

using namespace BOSS;

void AbilityAction::writeToSS(std::stringstream & ss) const
{
    ss << "[";
    ss << type.getID() << ",";
    ss << frameCast << ",";
    ss << targetID << ",";
    ss << targetProductionID << ",";
    ss << targetType.getID() << ",";
    ss << targetProductionType.getID() << "]";
}

json AbilityAction::writeToJson() const
{
    json data;

    data["ID"] = type.getID();
    data["FrameCast"] = frameCast;
    data["targetID"] = targetID;
    data["targetProductionID"] = targetProductionID;
    data["targetType"] = targetType.getID();
    data["targetProductionType"] = targetProductionType.getID();

    return data;
}