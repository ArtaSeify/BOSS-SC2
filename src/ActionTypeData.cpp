/* -*- c-basic-offset: 4 -*- */

#include "ActionTypeData.h"
#include "JSONTools.h"
#include <unordered_map>

using namespace BOSS;

std::vector<ActionTypeData>                 AllActionTypeData;
std::unordered_map<std::string, ActionID>   ActionTypeNameMap;

const ActionTypeData & ActionTypeData::GetActionTypeData(const std::string & name)
{
    BOSS_ASSERT(ActionTypeNameMap.find(name) != ActionTypeNameMap.end(), "Action name not found: %s", name.c_str());

    return AllActionTypeData[ActionTypeNameMap.at(name)];
}

const ActionTypeData & ActionTypeData::GetActionTypeData(const ActionID & action)
{
    //BOSS_ASSERT(action < AllActionTypeData.size(), "ActionID overflow: %d", action);

    return AllActionTypeData[action];
}

const std::vector<ActionTypeData> & ActionTypeData::GetAllActionTypeData()
{
    return AllActionTypeData;
}

void ActionTypeData::CreateActionTypeData(const json & actions, RaceID race)
{
    int raceActionID = 0;

    for (ActionID a(1); a < actions.size() + 1; ++a)
    {
        ActionTypeData data;
        size_t index = a - 1;

        JSONTools::ReadString("race", actions[index], data.raceName);
        data.race = Races::GetRaceID(data.raceName);
        if (data.race == race)
        {
            data.id = a;
            data.raceActionID = raceActionID;
            JSONTools::ReadString("name", actions[index], data.name);
            JSONTools::ReadInt("mineralCost", actions[index], data.mineralCost);
            JSONTools::ReadInt("gasCost", actions[index], data.gasCost);
            JSONTools::ReadFloat("supplyCost", actions[index], data.supplyCost); // demical supply cost is possible in SC2
            JSONTools::ReadInt("energyCost", actions[index], data.energyCost);
            JSONTools::ReadInt("supplyProvided", actions[index], data.supplyProvided);
            JSONTools::ReadInt("buildTime", actions[index], data.buildTime);
            JSONTools::ReadInt("numProduced", actions[index], data.numProduced);
            JSONTools::ReadInt("startingEnergy", actions[index], data.startingEnergy);
            JSONTools::ReadInt("maxEnergy", actions[index], data.maxEnergy);
            JSONTools::ReadBool("isUnit", actions[index], data.isUnit);
            JSONTools::ReadBool("isUpgrade", actions[index], data.isUpgrade);
            JSONTools::ReadBool("isAbility", actions[index], data.isAbility);
            JSONTools::ReadBool("isBuilding", actions[index], data.isBuilding);
            JSONTools::ReadBool("isWorker", actions[index], data.isWorker);
            JSONTools::ReadBool("isRefinery", actions[index], data.isRefinery);
            JSONTools::ReadBool("isSupplyProvider", actions[index], data.isSupplyProvider);
            JSONTools::ReadBool("isResourceDepot", actions[index], data.isDepot);
            JSONTools::ReadBool("isAddon", actions[index], data.isAddon);

            BOSS_ASSERT(actions[index].count("whatBuilds"), "no 'whatBuilds' member");
            auto & whatBuilds = actions[index]["whatBuilds"];
            data.whatBuildsStr = whatBuilds[0].get<std::string>();
            data.whatBuildsCount = std::stoul(whatBuilds[1].get<std::string>());
            data.whatBuildsStatus = whatBuilds[2].get<std::string>();
            if (data.whatBuildsStatus == "Morphed")
            {
                data.isMorphed = true;
            }
            if (whatBuilds.size() == 4) { data.whatBuildsAddonStr = whatBuilds[3].get<std::string>(); }

            BOSS_ASSERT(actions[index].count("required"), "no 'required' member");
            for (auto & req : actions[index]["required"])
            {
                data.requiredStrings.push_back(req);
            }

            BOSS_ASSERT(actions[index].count("equivalent"), "no 'equivalent' member");
            for (auto & equiv : actions[index]["equivalent"])
            {
                data.equivalentStrings.push_back(equiv);
            }

            // the name map stores the index that will hold this data, which is the current size
            ActionTypeNameMap[data.name] = ActionID(AllActionTypeData.size());

            // then we add the data to the vector
            AllActionTypeData.push_back(data);

            raceActionID++;

            //std::cout << AllActionTypeData.back().name << " " << AllActionTypeData.back().mineralCost << "\n";
        }
    }
}

void ActionTypeData::Init(const json & j)
{
    // add the None type for error returns
    AllActionTypeData.push_back(ActionTypeData());
    ActionTypeNameMap["None"] = 0;

    std::vector<int> numUnitsEachRace;

    // read all of the action types in the file
    if (j.count("Types") && j["Types"].is_array())
    {
        const json & actions = j["Types"];

        CreateActionTypeData(actions, Races::Protoss);
        numUnitsEachRace.push_back(AllActionTypeData.back().raceActionID + 1);

        CreateActionTypeData(actions, Races::Terran);
        numUnitsEachRace.push_back(AllActionTypeData.back().raceActionID + 1);

        CreateActionTypeData(actions, Races::Zerg);
        numUnitsEachRace.push_back(AllActionTypeData.back().raceActionID + 1);
    }

    // now we have to re-iterate over all established types to get the ids
    for (auto & data : AllActionTypeData)
    {
        if (data.whatBuildsStr.size() > 0)
        {
            // get the types of the thing that builds this type
            data.whatBuilds = ActionType(ActionTypeNameMap.at(data.whatBuildsStr));
        }

        if (data.whatBuildsAddonStr.size() > 0) 
        {
            // get the types of the addon required by the builder of this thing
            data.whatBuildsAddon = ActionType(AllActionTypeData[ActionTypeNameMap.at(data.whatBuildsAddonStr)].id);
        }

        // add the types of all the equivalent types
        for (size_t i(0); i < data.equivalentStrings.size(); ++i)
        {
            data.equivalent.push_back(ActionType(ActionTypeNameMap.at(data.equivalentStrings[i])));
        }

        // add the ids of all the prerequisites
        for (size_t i(0); i < data.requiredStrings.size(); ++i)
        {
            if (ActionTypeNameMap.find(data.requiredStrings[i]) != ActionTypeNameMap.end())
            {
                data.required.push_back(ActionType(ActionTypeNameMap.at(data.requiredStrings[i])));
            }
        }
    }

    // whatBuildsVector contains a 1 in the slot pertaining to the raceActionID of
    // the unit that produces this unit
    for (auto & data : AllActionTypeData)
    {
        if (data.id == (ActionTypes::None).getID())
        {
            continue;
        }

        data.whatBuildsVector = std::vector<bool>(numUnitsEachRace[data.race], false);

        if (data.whatBuildsStr.size() > 0)
        {
            data.whatBuildsVector[data.whatBuilds.getRaceActionID()] = true;
        }
    }
}
