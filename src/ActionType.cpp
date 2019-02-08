/* -*- c-basic-offset: 4 -*- */

#include "ActionType.h"
#include "ActionTypeData.h"
#include "ActionSetAbilities.h"

using namespace BOSS;

std::vector<ActionSetAbilities> allActionPrerequisites;
std::vector<ActionSetAbilities> allActionRecursivePrerequisites;

ActionType::ActionType()
    : m_id(0)
{
}

ActionType::ActionType(const ActionID & actionID)
    : m_id(actionID)
{
}

#if 0
//!!! mic: not necessary

ActionType::ActionType(const ActionType & type)
    : m_id(type.m_id)
{
}

ActionType & ActionType::operator = (ActionType & rhs)
{
    if (this != &rhs)
    {
        new (this) ActionType(rhs);
    }

    return *this;
}   
#endif

ActionID ActionType::getID()   const { return m_id; }
ActionID ActionType::getRaceActionID() const { return ActionTypeData::GetActionTypeData(m_id).raceActionID; }
RaceID   ActionType::getRace() const { return ActionTypeData::GetActionTypeData(m_id).race; }
const std::string & ActionType::getName() const { return ActionTypeData::GetActionTypeData(m_id).name; }
    
int  ActionType::buildTime()         const { return ActionTypeData::GetActionTypeData(m_id).buildTime; }
int  ActionType::mineralPrice()      const { return ActionTypeData::GetActionTypeData(m_id).mineralCost; }
int  ActionType::gasPrice()          const { return ActionTypeData::GetActionTypeData(m_id).gasCost; }
int  ActionType::supplyCost()        const { return ActionTypeData::GetActionTypeData(m_id).supplyCost; }
int  ActionType::energyCost()        const { return ActionTypeData::GetActionTypeData(m_id).energyCost; }
int  ActionType::supplyProvided()    const { return ActionTypeData::GetActionTypeData(m_id).supplyProvided; }
int  ActionType::numProduced()       const { return ActionTypeData::GetActionTypeData(m_id).numProduced; }
int  ActionType::startingEnergy()    const { return ActionTypeData::GetActionTypeData(m_id).startingEnergy; }
int  ActionType::maxEnergy()         const { return ActionTypeData::GetActionTypeData(m_id).maxEnergy; }
bool ActionType::isAddon()           const { return ActionTypeData::GetActionTypeData(m_id).isAddon; }
bool ActionType::isRefinery()        const { return ActionTypeData::GetActionTypeData(m_id).isRefinery; }
bool ActionType::isWorker()          const { return ActionTypeData::GetActionTypeData(m_id).isWorker; }
bool ActionType::isBuilding()        const { return ActionTypeData::GetActionTypeData(m_id).isBuilding; }
bool ActionType::isDepot()           const { return ActionTypeData::GetActionTypeData(m_id).isDepot; }
bool ActionType::isSupplyProvider()  const { return ActionTypeData::GetActionTypeData(m_id).isSupplyProvider; }
bool ActionType::isUnit()            const { return ActionTypeData::GetActionTypeData(m_id).isUnit; }
bool ActionType::isUpgrade()         const { return ActionTypeData::GetActionTypeData(m_id).isUpgrade; }
bool ActionType::isAbility()         const { return ActionTypeData::GetActionTypeData(m_id).isAbility; }
bool ActionType::isMorphed()         const { return false; }

ActionType ActionType::whatBuilds() const
{
    return ActionTypeData::GetActionTypeData(m_id).whatBuilds;
}

const std::vector<bool> & ActionType::whatBuildsVector() const
{
    return ActionTypeData::GetActionTypeData(m_id).whatBuildsVector;
}

const std::string & ActionType::whatBuildsStatus() const
{
    return ActionTypeData::GetActionTypeData(m_id).whatBuildsStatus;
}

ActionType ActionType::whatBuildsAddon() const
{
    return ActionTypeData::GetActionTypeData(m_id).whatBuildsAddon;
}

const std::vector<ActionType> & ActionType::required() const
{
    return ActionTypeData::GetActionTypeData(m_id).required;
}

const std::vector<ActionType> & ActionType::equivalent() const
{
    return ActionTypeData::GetActionTypeData(m_id).equivalent;
}

const ActionSetAbilities & ActionType::getPrerequisiteActionCount() const
{
    return allActionPrerequisites[m_id];
}

const ActionSetAbilities & ActionType::getRecursivePrerequisiteActionCount() const
{
    return allActionRecursivePrerequisites[m_id];
}

namespace BOSS
{
namespace ActionTypes
{
    std::vector<ActionType> allActionTypes;
    std::map<std::string, ActionType> nameMap;
    std::vector<int>        raceActionTypesCount = std::vector<int>(3, 0);
    std::vector<ActionType> workerActionTypes;
    std::vector<ActionType> refineryActionTypes;
    std::vector<ActionType> supplyProviderActionTypes;
    std::vector<ActionType> resourceDepotActionTypes;
    std::vector<ActionType> specialActionTypes;
    std::vector<ActionType> detectorTypes;

    void Init()
    {
        for (ActionID i(0); i < ActionTypeData::GetAllActionTypeData().size(); ++i)
        {
            allActionTypes.push_back(ActionType(i));
            nameMap[allActionTypes[i].getName()] = allActionTypes[i];
            
            raceActionTypesCount[allActionTypes[i].getRace()]++;
        }

        workerActionTypes.push_back(ActionTypes::GetActionType("Probe"));
        refineryActionTypes.push_back(ActionTypes::GetActionType("Assimilator"));
        supplyProviderActionTypes.push_back(ActionTypes::GetActionType("Pylon"));
        resourceDepotActionTypes.push_back(ActionTypes::GetActionType("Nexus"));
        specialActionTypes.push_back(ActionTypes::GetActionType("ChronoBoost"));
        detectorTypes.push_back(ActionTypes::GetActionType("Observer"));

        workerActionTypes.push_back(ActionTypes::GetActionType("SCV"));
        refineryActionTypes.push_back(ActionTypes::GetActionType("Refinery"));
        supplyProviderActionTypes.push_back(ActionTypes::GetActionType("SupplyDepot"));
        resourceDepotActionTypes.push_back(ActionTypes::GetActionType("CommandCenter"));

        workerActionTypes.push_back(ActionTypes::GetActionType("Drone"));
        refineryActionTypes.push_back(ActionTypes::GetActionType("Extractor"));
        supplyProviderActionTypes.push_back(ActionTypes::GetActionType("Overlord"));
        resourceDepotActionTypes.push_back(ActionTypes::GetActionType("Hatchery"));
        //detectorTypes.push_back(ActionTypes::GetActionType("Overseer"));

        // calculate all action prerequisites
        for (size_t i(0); i < allActionTypes.size(); ++i)
        {
            allActionPrerequisites.push_back(CalculatePrerequisites(allActionTypes[i]));
        }

        // calculate all action recursive prerequisites
        for (size_t i(0); i < allActionTypes.size(); ++i)
        {
            ActionSetAbilities recursivePrerequisites;
            CalculateRecursivePrerequisites(recursivePrerequisites, allActionTypes[i]);
            allActionRecursivePrerequisites.push_back(recursivePrerequisites);
        }
    }

    int GetRaceActionCount(RaceID raceID)
    {
        return raceActionTypesCount[raceID];
    }

    ActionType GetWorker(RaceID raceID)
    {
        return workerActionTypes[raceID];
    }

    ActionType GetSupplyProvider(RaceID raceID)
    {
        return supplyProviderActionTypes[raceID];
    }

    ActionType GetRefinery(RaceID raceID)
    {
        return refineryActionTypes[raceID];
    }

    ActionType GetResourceDepot(RaceID raceID)
    {
        return resourceDepotActionTypes[raceID];
    }

    ActionType GetSpecialAction(RaceID raceID)
    {
        return specialActionTypes[raceID];
    }

    ActionType GetDetector(RaceID raceID)
    {
        return detectorTypes[raceID];
    }
    
    ActionType GetActionType(const std::string & name)
    {
        BOSS_ASSERT(TypeExists(name), "ActionType name not found: %s", name.c_str());

        return nameMap[name];
    }

    bool TypeExists(const std::string & name) 
    {
        return nameMap.find(name) != nameMap.end();
    }

    const std::vector<ActionType> & GetAllActionTypes()
    {
        return allActionTypes;
    }

    ActionType None(0);

    ActionSetAbilities CalculatePrerequisites(ActionType /*action NOT USED? */)
    {
        ActionSetAbilities count;

        // add everything from whatBuilds and required

        //printf("Finish Prerequisites\n");
        return count;
    }

    void CalculateRecursivePrerequisites(ActionSetAbilities & allActions, ActionType action)
    {
        ActionSetAbilities pre = action.getPrerequisiteActionCount();

        if (action.gasPrice() > 0)
        {
            pre.add(ActionTypes::GetRefinery(action.getRace()));
        }

        for (ActionID a(0); a < pre.size(); ++a)
        {
            ActionType actionType(a);
            
            if (pre.contains(actionType) && !allActions.contains(actionType))
            {
                allActions.add(actionType);
                CalculateRecursivePrerequisites(allActions, actionType);
            }
        }
    }

}
}
