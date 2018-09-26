#pragma once

#include "Common.h"
#include "ActionSet.h"

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
    
    ActionType whatBuilds() const;
    const std::string & whatBuildsStatus() const;
    ActionType whatBuildsAddon() const;
    const std::vector<ActionType> & required() const;
    const std::vector<ActionType> & equivalent() const;
    const ActionSet & getPrerequisiteActionCount() const;
    const ActionSet & getRecursivePrerequisiteActionCount() const;

    const bool operator == (const ActionType & rhs)     const;
    const bool operator != (const ActionType & rhs)     const;
    const bool operator <  (const ActionType & rhs)     const;
};

namespace ActionTypes
{
    void Init();
    const std::vector<ActionType> & GetAllActionTypes();
    const ActionType & GetWorker(const RaceID raceID);
    const ActionType & GetSupplyProvider(const RaceID raceID);
    const ActionType & GetRefinery(const RaceID raceID);
    const ActionType & GetResourceDepot(const RaceID raceID);
    const ActionType & GetSpecialAction(const RaceID raceID);
    const ActionType & GetActionType(const std::string & name);
    const bool         TypeExists(const std::string & name);

    ActionSet CalculatePrerequisites(const ActionType & action);
    void CalculateRecursivePrerequisites(ActionSet & count, const ActionType & action);

    extern ActionType None;
}
}