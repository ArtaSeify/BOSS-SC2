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
bool ActionType::isMorphed()         const { return ActionTypeData::GetActionTypeData(m_id).isMorphed; }

ActionType ActionType::whatBuilds() const
{
    return ActionTypeData::GetActionTypeData(m_id).whatBuilds;
}

//ActionType ActionType::whatBuildsSecond() const
//{
//    return ActionTypeData::GetActionTypeData(m_id).whatBuildsSecond;
//}

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

const std::vector<ActionType> & ActionType::strongAgainst(RaceID race) const
{
    return ActionTypeData::GetActionTypeData(m_id).strongAgainst[race];
}

const std::vector<ActionType> & ActionType::weakAgainst(RaceID race) const
{
    return ActionTypeData::GetActionTypeData(m_id).weakAgainst[race];
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
    std::vector<std::vector<ActionType>> raceActionTypes = std::vector<std::vector<ActionType>>(3, std::vector<ActionType>());
    std::vector<ActionType> workerActionTypes;
    std::vector<ActionType> refineryActionTypes;
    std::vector<ActionType> supplyProviderActionTypes;
    std::vector<ActionType> resourceDepotActionTypes;
    std::vector<ActionType> specialActionTypes;
    std::vector<ActionType> detectorTypes;
    ActionType              warpGateResearch;
    ActionType              gatewayActionType;
    ActionType              warpgateActionType;

    void Init()
    {
        std::ofstream actionTypesFile(CONSTANTS::ExecutablePath + "/data/ActionData.txt");
        for (ActionID i(0); i < ActionTypeData::GetAllActionTypeData().size(); ++i)
        {
            allActionTypes.push_back(ActionType(i));
            nameMap[allActionTypes[i].getName()] = allActionTypes[i];

            //std::cout << allActionTypes[i].getName() << " " << allActionTypes[i].getRaceActionID() << std::endl;

            if (allActionTypes[i].getRace() == Races::None)
            {
                for (auto& race : raceActionTypes)
                {
                    race.push_back(allActionTypes[i]);
                }
            }
            
            else
            {
                raceActionTypes[allActionTypes[i].getRace()].push_back(allActionTypes[i]);
            }
            actionTypesFile << i << "," << allActionTypes[i].getName() << std::endl;

            //std::cout << allActionTypes[i].getName() << ": ";
            //if (allActionTypes[i].strongAgainst(Races::Terran).size() > 0)
            //{
            //    std::cout << allActionTypes[i].strongAgainst(Races::Terran)[0].getName();
            //}
            //if (allActionTypes[i].weakAgainst(Races::Terran).size() > 0)
            //{
            //    std::cout << ", " << allActionTypes[i].weakAgainst(Races::Terran)[0].getName();
            //}
            //std::cout << std::endl;
        }

        workerActionTypes.push_back(ActionTypes::GetActionType("Probe"));
        refineryActionTypes.push_back(ActionTypes::GetActionType("Assimilator"));
        supplyProviderActionTypes.push_back(ActionTypes::GetActionType("Pylon"));
        resourceDepotActionTypes.push_back(ActionTypes::GetActionType("Nexus"));
        specialActionTypes.push_back(ActionTypes::GetActionType("ChronoBoost"));
        detectorTypes.push_back(ActionTypes::GetActionType("Observer"));
        warpGateResearch = ActionTypes::GetActionType("WarpGateResearch");
        gatewayActionType = ActionTypes::GetActionType("Gateway");
        warpgateActionType = ActionTypes::GetActionType("WarpGate");

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
        return (int)raceActionTypes[raceID].size();
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

    ActionType GetWarpGateResearch()
    {
        return warpGateResearch;
    }

    ActionType GetGatewayAction()
    {
        return gatewayActionType;
    }

    ActionType GetWarpgateAction()
    {
        return warpgateActionType;
    }
    
    ActionType GetActionType(const std::string & name)
    {
        BOSS_ASSERT(TypeExists(name), "ActionType name not found: %s", name.c_str());

        return nameMap[name];
    }

    ActionType GetActionType(ActionID id)
    {
        BOSS_ASSERT(id < allActionTypes.size(), "id of action %i is not valid", id);

        return allActionTypes[id];
    }

    ActionType GetRaceActionType(ActionID id, RaceID race)
    {
        BOSS_ASSERT(id < raceActionTypes[race].size(), "id of action %i is not valid for race %s", id, Races::GetRaceName(race).c_str());

        return raceActionTypes[race][id];
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
