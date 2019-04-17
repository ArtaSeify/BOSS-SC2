/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"
#include "Unit.h"
#include "AbilityAction.h"
#include "BoundedVector.h"

namespace BOSS
{
    //using Vector_Unit = BoundedVector<Unit, 70>;
    //using Vector_BuildingUnits = BoundedVector<NumUnits, 25>;
    //using Vector_FinishedUnits = BoundedVector<NumUnits, 50>;
    //using Vector_AbilityAction = BoundedVector<AbilityAction, 7>;
class GameState 
{
    //Vector_Unit             m_units;
    //Vector_BuildingUnits    m_unitsBeingBuilt;      // indices of m_units which are not completed, sorted descending by finish time
    //Vector_FinishedUnits    m_unitsSortedEndFrame;  // indices of m_units which are completed, in order
    //Vector_AbilityAction    m_chronoBoosts;
    std::vector<Unit>           m_units;
    std::vector<NumUnits>       m_unitsBeingBuilt;      // indices of m_units which are not completed, sorted descending by finish time
    std::vector<NumUnits>       m_unitsSortedEndFrame;  // indices of m_units which are completed, in order
    std::vector<AbilityAction>  m_chronoBoosts;
    std::vector<int>            m_unitTypes;             // a vector containing all the unit types we have
    RaceID                      m_race;
    FracType                    m_minerals;
    FracType                    m_gas;
    NumUnits                    m_currentSupply;
    NumUnits                    m_maxSupply;
    NumUnits                    m_inProgressSupply;     // the amount of supply that is in construction 
    TimeType                    m_currentFrame;
    TimeType                    m_previousFrame;
    NumUnits                    m_mineralWorkers;
    NumUnits                    m_gasWorkers;
    NumUnits                    m_buildingWorkers;
    NumUnits                    m_numRefineries;
    NumUnits                    m_inProgressRefineries;     // the amount of refineries being built
    NumUnits                    m_numDepots;
    NumUnits                    m_inProgressDepots;         // the amount of depots being built
    ActionType                  m_lastAction;
    AbilityAction               m_lastAbility;
 
    int                             getBuilderID(ActionType type)               const;
    bool                            haveBuilder(ActionType type)                const;
    bool                            havePrerequisites(ActionType type)          const;

    int                             whenSupplyReady(ActionType action)          const;
    int                             whenPrerequisitesReady(ActionType action)   const;
    int                             whenResourcesReady(ActionType action)       const;
    int                             whenBuilderReady(ActionType action)         const;

    Unit &                          getUnit(NumUnits id) { return m_units[id]; }
    void                            completeUnit(Unit & Unit);

  public: 
    GameState();
    GameState(const std::vector<Unit> & unitVector, RaceID race, FracType minerals, FracType gas,
        NumUnits currentSupply, NumUnits maxSupply, NumUnits mineralWorkers, NumUnits gasWorkers,
        NumUnits builerWorkers, TimeType currentFrame, NumUnits numRefineries, NumUnits numDepots);

    int                             whenCanBuild(ActionType action, NumUnits targetID = -1)      const;
    int                             whenCanCast(ActionType action, NumUnits targetID)       const;
    int                             whenEnergyReady(ActionType action)                      const;
    int                             getNextFinishTime(ActionType type)                      const;
    int                             timeUntilFirstPylonDone()                               const;

    void                            getSpecialAbilityTargets(ActionSetAbilities & actionSet, int index)         const;
    int                             storeChronoBoostTargets(ActionSetAbilities & actionSet, int index)          const;
    bool                            chronoBoostableTarget(const Unit & unit)                                    const;
    bool                            canChronoBoost()                                                            const;

    int                             getNumInProgress(ActionType action)                     const;
    int                             getNumCompleted(ActionType action)                      const;
    int                             getNumTotal(ActionType action)                          const;
    void                            getLegalActions(std::vector<ActionType> & legalActions) const;
    bool                            isLegal(ActionType type)                                const;
    bool                            haveType(ActionType action)                             const;
    int                             timeUntilResearchDone(ActionType action)                const;

    void                            doAbility(ActionType type, NumUnits targetID);
    void                            doAbility(ActionType type, NumUnits tragetID, TimeType frame);
    void                            doAction(ActionType type);
    void                            fastForward(TimeType frames);
    void                            addUnit(ActionType Unit, NumUnits builderID = -1);
  
    void                            setMinerals(FracType minerals) { m_minerals = minerals; }
    void                            setGas(FracType gas) { m_gas = gas; }

    bool                            canBuildNow(ActionType action)      const { return whenCanBuild(action) == getCurrentFrame(); }
    int                             getNumMineralWorkers()              const { return m_mineralWorkers; }
    int                             getNumGasWorkers()                  const { return m_gasWorkers; }
    int                             getNumTotalWorkers()                const { return m_mineralWorkers + m_gasWorkers + m_buildingWorkers; }
    int                             getNumberChronoBoostsCast()         const { return int(m_chronoBoosts.size()); }
    int                             getNumUnits()                       const { return int(m_units.size()); }
    int                             getLastActionFinishTime()           const { return m_unitsBeingBuilt.empty() ? getCurrentFrame() : m_units[m_unitsBeingBuilt.front()].getTimeUntilBuilt(); }
    int                             getCurrentSupply()                  const { return m_currentSupply; }
    int                             getMaxSupply()                      const { return m_maxSupply; }
    int                             getSupplyInProgress()               const { return m_inProgressSupply; }
    int                             getCurrentFrame()                   const { return m_currentFrame; }
    RaceID                          getRace()                           const { return m_race; }
    FracType                        getMinerals()                       const { return m_minerals; }
    FracType                        getGas()                            const { return m_gas; }
    const std::vector<AbilityAction>&getChronoBoostTargets()            const { return m_chronoBoosts; }
    const std::vector<NumUnits> &   getFinishedUnits()                  const { return m_unitsSortedEndFrame; }
    ActionType                      getUnitType(NumUnits id)            const { return m_units[id].getType(); }
    ActionType                      getLastAction()                     const { return m_lastAction; }
    const AbilityAction &           getLastAbility()                    const { return m_lastAbility; }
    const Unit &                    getUnit(NumUnits id)                const { return m_units[id]; }
    const std::vector<NumUnits> &   getUnitsBeingBuilt()                const { return m_unitsBeingBuilt; }
    const std::vector<int> &        getUnitTypes()                      const { return m_unitTypes; }
    int                             getNumType(ActionType type)         const { return m_unitTypes[type.getRaceActionID()]; }
    
    std::string                     toString()                          const;
    void                            printunitsbeingbuilt()              const;
    void                            printUnits()                        const;
    void                            writeToSS(std::stringstream & ss, const CombatSearchParameters & params)   const;
    json                            writeToJson(const CombatSearchParameters & params)               const;
};
}
