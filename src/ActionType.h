/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"

namespace BOSS
{
    class ActionSet;

    class ActionType
    {
        ActionID m_id;

    public:
    
        ActionType();
        ActionType(const ActionID & actionID);
        // ActionType(const ActionType & type);
        // ActionType & operator = (ActionType rhs);
        const std::string & getName() const;

        ActionID getID()             const;
        ActionID getRaceActionID()   const;
        RaceID   getRace()           const;
    
        int  buildTime()        const;
        int  mineralPrice()     const;
        int  gasPrice()         const;
        int  supplyCost()       const;
        int  energyCost()       const;
        int  supplyProvided()   const;
        int  numProduced()      const;
        int  startingEnergy()    const;
        int  maxEnergy()        const;
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
        //ActionType whatBuildsSecond() const;
        const std::vector<bool> & whatBuildsVector() const;
        const std::string & whatBuildsStatus() const;
        ActionType whatBuildsAddon() const;
        const std::vector<ActionType> & required() const;
        const std::vector<ActionType> & equivalent() const;
        const std::vector<ActionType> & strongAgainst(RaceID race) const;
        const std::vector<ActionType> & weakAgainst(RaceID race) const;
        const ActionSet & getPrerequisiteActionCount() const;
        const ActionSet & getRecursivePrerequisiteActionCount() const;

        bool operator == (ActionType rhs) const { return m_id == rhs.m_id; }
        bool operator != (ActionType rhs) const { return m_id != rhs.m_id; }
        bool operator <  (ActionType rhs) const { return m_id < rhs.m_id; }
    };

    namespace ActionTypes
    {
        void Init();
        const std::vector<ActionType> & GetAllActionTypes();
        int GetRaceActionCount(RaceID raceID);
        ActionType GetWorker(RaceID raceID);
        ActionType GetSupplyProvider(RaceID raceID);
        ActionType GetRefinery(RaceID raceID);
        ActionType GetResourceDepot(RaceID raceID);
        ActionType GetSpecialAction(RaceID raceID);
        ActionType GetDetector(RaceID raceID);
        ActionType GetWarpGateResearch();
        ActionType GetGatewayAction();
        ActionType GetWarpgateAction();
        ActionType GetActionType(const std::string & name);
        ActionType GetActionType(ActionID id);
        ActionType GetRaceActionType(ActionID id, RaceID race);
        bool TypeExists(const std::string & name);

        ActionSet CalculatePrerequisites(ActionType action);
        void CalculateRecursivePrerequisites(ActionSet & allActions, ActionType action);

        extern ActionType None;
    }
}
