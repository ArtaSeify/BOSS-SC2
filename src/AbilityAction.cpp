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