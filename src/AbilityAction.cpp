/* -*- c-basic-offset: 4 -*- */

#include "AbilityAction.h"

using namespace BOSS;

void AbilityAction::writeToFile(std::ofstream & file) const
{
    file << "[";
    file << type.getID() << ", ";
    file << frameCast << ", ";
    file << targetID << ", ";
    file << targetProductionID << ", ";
    file << targetType.getID() << "]";
}