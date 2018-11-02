#pragma once

#include "Common.h"
#include "ActionSetAbilities.h"

namespace BOSS
{

class ActionType
{
    const ActionID	    m_id;

public:
	
    ActionType();
    ActionType(const ActionType & type);
    ActionType(const ActionID & actionID);

    ActionType & operator = (const ActionType & rhs);
    const std::string & getName()       const;

    ActionID getID()             const;
    RaceID   getRace()           const;
	
	int  buildTime()        const;
	int  mineralPrice()     const;
    int  gasPrice()         const;
    int  supplyCost()       const;
    int  energyCost()       const;
    int  supplyProvided()   const;
    int  numProduced()      const;
	int  startingEnergy()	const;
	int  maxEnergy()		const;
    bool isUnit()           const;
    bool isUpgrade()        const;
    bool isAbility()        const;
    bool isBuilding()       const;
    bool isWorker()         const;
    bool isRefinery()       const;
    bool isDepot()          const;
    bool isSupplyProvider() const;
    bool isAddon()          const;
    bool isMorphed()        const;
    
    const ActionType & whatBuilds() const;
    const std::string & whatBuildsStatus() const;
    const ActionType & whatBuildsAddon() const;
    const std::vector<ActionType> & required() const;
    const std::vector<ActionType> & equivalent() const;
    const ActionSetAbilities & getPrerequisiteActionCount() const;
    const ActionSetAbilities & getRecursivePrerequisiteActionCount() const;

    bool operator == (const ActionType & rhs)     const { return m_id == rhs.m_id; }
    bool operator != (const ActionType & rhs)     const { return m_id != rhs.m_id; }
    bool operator <  (const ActionType & rhs)     const { return m_id < rhs.m_id; }
};

namespace ActionTypes
{
    void Init();
    const std::vector<ActionType> & GetAllActionTypes();
    ActionType GetWorker(const RaceID raceID);
    ActionType GetSupplyProvider(const RaceID raceID);
    ActionType GetRefinery(const RaceID raceID);
    ActionType GetResourceDepot(const RaceID raceID);
    ActionType GetSpecialAction(const RaceID raceID);
    ActionType GetActionType(const std::string & name);
    const bool TypeExists(const std::string & name);

    ActionSetAbilities CalculatePrerequisites(const ActionType & action);
    void CalculateRecursivePrerequisites(ActionSetAbilities & count, const ActionType & action);

    extern ActionType None;
}
}