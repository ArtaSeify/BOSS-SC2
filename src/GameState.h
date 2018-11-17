/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"
#include "Unit.h"
#include "AbilityAction.h"
#include "StaticVector.h"

namespace BOSS
{

class GameState 
{
    typedef StaticVector<Unit, 70>              Vector_Unit;
    typedef StaticVector<NumUnits, 35>          Vector_NumUnits;
    typedef StaticVector<AbilityAction, 10>     Vector_AbilityAction;

    Vector_Unit                     m_units;
    Vector_NumUnits	                m_unitsBeingBuilt;      // indices of m_units which are not completed, sorted descending by finish time
    Vector_NumUnits                 m_unitsSortedEndFrame;  // indices of m_units which are completed, in order
    Vector_AbilityAction            m_chronoBoosts;
    RaceID					        m_race;
    FracType    		            m_minerals;
    FracType				        m_gas;
    NumUnits					    m_currentSupply;
    NumUnits					    m_maxSupply;
    TimeType					    m_currentFrame;
    TimeType					    m_previousFrame;
    NumUnits					    m_mineralWorkers;
    NumUnits					    m_gasWorkers;
    NumUnits					    m_buildingWorkers;
    NumUnits					    m_numRefineries;
    NumUnits					    m_numDepots;
    ActionType			            m_lastAction;
    AbilityAction                   m_lastAbility;

    int		                        getBuilderID(ActionType type)               const;
    bool		                    haveBuilder(ActionType type)                const;
    bool		                    havePrerequisites(ActionType type)          const;

    int		                        whenSupplyReady(ActionType action)          const;
    int		                        whenPrerequisitesReady(ActionType action)   const;
    int		                        whenResourcesReady(ActionType action)       const;
    int		                        whenBuilderReady(ActionType action)         const;

    void                            addUnitToSpecialVectors(NumUnits unitIndex);

    Unit &		                    getUnit(NumUnits id) { return m_units[id]; }
    void		                    completeUnit(Unit & Unit);

  public: 

    GameState();

    TimeType				        whenCanBuild(ActionType action)                         const;
    TimeType                        whenCanCast(ActionType action, NumUnits targetID)       const;
    TimeType                        whenEnergyReady(ActionType action)                      const;
    NumUnits			            getSupplyInProgress()                                   const;
    TimeType                        getNextFinishTime(ActionType type)                      const;

    void                            getSpecialAbilityTargets(ActionSetAbilities & actionSet, NumUnits index)      const;
    void                            storeChronoBoostTargets(ActionSetAbilities & actionSet, NumUnits index)       const;
    bool                            chronoBoostableTarget(const Unit & unit)                                    const;
    bool                            canChronoBoostTarget(const Unit & unit)                                     const;
    bool                            canChronoBoost()                                                            const;

    NumUnits			            getNumInProgress(ActionType action)                     const;
    NumUnits			            getNumCompleted(ActionType action)                      const;
    NumUnits			            getNumTotal(ActionType action)                          const;
    void			                getLegalActions(std::vector<ActionType> & legalActions) const;
    bool			                isLegal(ActionType type)                                const;
    bool			                haveType(ActionType action)                             const;

    bool                            doAbility(ActionType type, NumUnits targetID);
    void			                doAction(ActionType type);
    void			                fastForward(TimeType frames);
    void			                addUnit(ActionType Unit, NumUnits builderID = -1);
  
    void			                setMinerals(FracType minerals) { m_minerals = minerals; }
    void			                setGas(FracType gas) { m_gas = gas; }

    bool			                canBuildNow(ActionType action)      const { return whenCanBuild(action) == getCurrentFrame(); }
    NumUnits			            getNumMineralWorkers()              const { return m_mineralWorkers; }
    NumUnits			            getNumGasWorkers()                  const { return m_gasWorkers; }
    size_t                          getNumberChronoBoostsCast()         const { return m_chronoBoosts.size(); }
    size_t                          getNumUnits()                       const { return m_units.size(); }
    TimeType                        getLastActionFinishTime()           const { return m_unitsBeingBuilt.empty() ? getCurrentFrame() : m_units[m_unitsBeingBuilt.front()].getTimeUntilBuilt(); }
    NumUnits		                getCurrentSupply()                  const { return m_currentSupply; }
    NumUnits		                getMaxSupply()                      const { return m_maxSupply; }
    TimeType 	                    getCurrentFrame()                   const { return m_currentFrame; }
    RaceID				            getRace()                           const { return m_race; }
    FracType	                    getMinerals()                       const { return m_minerals; }
    FracType	                    getGas()                            const { return m_gas; }
    const Vector_AbilityAction &    getChronoBoostTargets()             const { return m_chronoBoosts; }
    const Vector_NumUnits &         getFinishedUnits()                  const { return m_unitsSortedEndFrame; }
    ActionType                      getUnitType(NumUnits id)            const { return m_units[id].getType(); }
    ActionType                      getLastAction()                     const { return m_lastAction; }
    const AbilityAction &           getLastAbility()                    const { return m_lastAbility; }
    const Unit &	                getUnit(NumUnits id)                const { return m_units[id]; }
    
    std::string		                toString()                          const;
    void                            printunitsbeingbuilt()              const;
};
}
