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
    typedef StaticVector<Unit, VectorLimit>         Vector_unit;
    typedef StaticVector<uint2, VectorLimit / 2>    Vector_sizet;
    typedef StaticVector<AbilityAction, 5>          Vector_abilityaction;

    Vector_unit                 m_units;
    Vector_sizet	            m_unitsBeingBuilt;      // indices of m_units which are not completed, sorted descending by finish time
    Vector_sizet                m_unitsSortedEndFrame;  // indices of m_units which are completed, in order
    Vector_abilityaction        m_chronoBoosts;
    uint4                       m_numUnits;
    RaceID					    m_race;
    float				        m_minerals;
    float				        m_gas;
    short					    m_currentSupply;
    short					    m_maxSupply;
    short					    m_currentFrame;
    short					    m_previousFrame;
    short					    m_mineralWorkers;
    short					    m_gasWorkers;
    short					    m_buildingWorkers;
    short					    m_numRefineries;
    short					    m_numDepots;
    ActionType			        m_lastAction;
    AbilityAction               m_lastAbility;

    short		getBuilderID(ActionType type)               const;
    bool		haveBuilder(ActionType type)                const;
    bool		havePrerequisites(ActionType type)          const;

    short		whenSupplyReady(ActionType action)          const;
    short		whenPrerequisitesReady(ActionType action)   const;
    short		whenResourcesReady(ActionType action)       const;
    short		whenBuilderReady(ActionType action)         const;

    void        addUnitToSpecialVectors(uint4 unitIndex);

    Unit &		getUnit(uint4 id)
                { 
                    //BOSS_ASSERT(id < m_numUnits, "getUnit() called with invalid id");
                    return m_units[id]; 
                }
    void		completeUnit(Unit & Unit);

public: 

    GameState();

    short				                    whenCanBuild(ActionType action)                     const;
    short                                   whenCanCast(ActionType action, uint4 targetID)     const;
    short                                   whenEnergyReady(ActionType action)                  const;
    short			                        getSupplyInProgress()                                       const;
    short                                   getNextFinishTime(ActionType type)                  const;

    void                                    getSpecialAbilityTargets(ActionSetAbilities & actionSet)    const;
    void                                    storeChronoBoostTargets(ActionSetAbilities & actionSet)     const;
    bool                                    chronoBoostableTarget(const Unit & unit)                    const;
    bool                                    canChronoBoostTarget(const Unit & unit)                     const;
    bool                                    canChronoBoost()                                            const;

    uint4			                        getNumInProgress(ActionType action)                 const;
    uint4			                        getNumCompleted(ActionType action)                  const;
    uint4			                        getNumTotal(ActionType action)                      const;
    void			                        getLegalActions(std::vector<ActionType> & legalActions)     const;
    bool			                        isLegal(ActionType type)                            const;
    bool			                        haveType(ActionType action)                         const;

    bool                                    doAbility(ActionType type, uint4 targetID);
    void			                        doAction(ActionType type);
    void			                        fastForward(short frames);
    void			                        addUnit(ActionType Unit, short builderID = -1);
  
    void			                        setMinerals(float minerals) { m_minerals = minerals; }
    void			                        setGas(float gas) { m_gas = gas; }

    bool			                        canBuildNow(ActionType action) const { return whenCanBuild(action) == getCurrentFrame(); }
    uint4			                        getNumMineralWorkers() const { return m_mineralWorkers; }
    uint4			                        getNumGasWorkers() const { return m_gasWorkers; }
    uint4                                   getNumberChronoBoostsCast() const { return m_chronoBoosts.size(); }
    uint4                                   getNumUnits() const { return m_numUnits; }
    short                                   getLastActionFinishTime() const { return m_unitsBeingBuilt.empty() ? getCurrentFrame() : m_units[m_unitsBeingBuilt.front()].getTimeUntilBuilt(); }
    short		                            getCurrentSupply() const { return m_currentSupply; }
    short		                            getMaxSupply() const { return m_maxSupply; }
    short 	                                getCurrentFrame() const { return m_currentFrame; }
    short				                    getRace() const { return m_race; }
    float	                                getMinerals() const { return m_minerals; }
    float	                                getGas() const { return m_gas; }
    const Vector_abilityaction &            getChronoBoostTargets() const { return m_chronoBoosts; }
    const Vector_sizet &                    getFinishedUnits() const { return m_unitsSortedEndFrame; }
    ActionType                              getUnitType(uint4 id) const { return m_units[id].getType(); }
    ActionType                              getLastAction() const { return m_lastAction; }
    const AbilityAction &                   getLastAbility() const { return m_lastAbility; }
    const Unit &	                        getUnit(uint4 id) const
                                            { 
                                                //BOSS_ASSERT(id < m_numUnits, "getUnit() called with invalid id");
                                                return m_units[id]; 
                                            }
    
    std::string		                        toString() const;
    void                                    printunitsbeingbuilt() const;
};
}
