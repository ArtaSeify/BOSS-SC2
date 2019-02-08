/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"

namespace BOSS
{
    
    struct ActionTypeData
    {
        std::string                 name            = "None";
        std::string                 raceName        = "None";
        ActionID                    id              = 0;
        ActionID                    raceActionID    = 0;
        RaceID                      race            = Races::None;            
        int                         mineralCost     = 0;      
        int                         gasCost         = 0;       
        int                         supplyCost      = 0;   
        int                         energyCost      = 0;
        int                         supplyProvided  = 0;   
        int                         buildTime       = 0;
        int                         numProduced     = 1;
        int                         startingEnergy  = 0;
        int                         maxEnergy       = 0;
        bool                        isUnit          = false;
        bool                        isUpgrade       = false;
        bool                        isAbility       = false;
        bool                        isBuilding      = false;
        bool                        isWorker        = false;
        bool                        isRefinery      = false;
        bool                        isSupplyProvider= false;
        bool                        isDepot         = false;
        bool                        isAddon         = false;
        ActionType                  whatBuilds;
        std::vector<bool>           whatBuildsVector;           // the size of this vector is the number of actions of this actions' race. 
        uint4                       whatBuildsCount = 1;
        ActionType                  whatBuildsAddon;
        std::string                 whatBuildsStr;
        std::string                 whatBuildsStatus;
        std::string                 whatBuildsAddonStr;
        std::vector<std::string>    equivalentStrings;
        std::vector<std::string>    requiredStrings;
        std::vector<ActionType>     equivalent;
        std::vector<ActionType>     required;

        static void Init(const json & filename);
        static void CreateActionTypeData(const json & actions, RaceID race);
        static const ActionTypeData & GetActionTypeData(const ActionID & action);
        static const ActionTypeData & GetActionTypeData(const std::string & name);
        static const std::vector<ActionTypeData> & GetAllActionTypeData();
    };

}
