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
    typedef StaticVector<Unit, 70>              Vector_unit;
    typedef StaticVector<uint2, 35>             Vector_sizet; //!!! PROBLEM name misleading (should be int?)
    typedef StaticVector<AbilityAction, 10>     Vector_abilityaction;

    Vector_unit  m_units;
    Vector_sizet m_unitsBeingBuilt;      // indices of m_units which are not completed, sorted descending by finish time
    Vector_sizet m_unitsSortedEndFrame;  // indices of m_units which are completed, in order
    Vector_abilityaction m_chronoBoosts;
    short m_numUnits;
    RaceID m_race;
    float m_minerals;
    float m_gas;
    short m_currentSupply;
    short m_maxSupply;
    int m_currentFrame;
    int m_previousFrame;
    short m_mineralWorkers;
    short m_gasWorkers;
    short m_buildingWorkers;
    short m_numRefineries;
    short m_numDepots;
    ActionType m_lastAction;
    AbilityAction m_lastAbility;

    int getBuilderID(ActionType type) const;
    bool haveBuilder(ActionType type) const;
    bool havePrerequisites(ActionType type) const;

    int whenSupplyReady(ActionType action) const;
    int	whenPrerequisitesReady(ActionType action) const;
    int	whenResourcesReady(ActionType action) const;
    int	whenBuilderReady(ActionType action) const;

    void addUnitToSpecialVectors(int unitIndex);

    Unit &getUnit(int id)
    { 
      //BOSS_ASSERT(id < m_numUnits, "getUnit() called with invalid id");
      return m_units[id]; 
    }
    void completeUnit(Unit & Unit);

  public: 

    GameState();

    int whenCanBuild(ActionType action) const;
    int whenCanCast(ActionType action, int targetID) const;
    int whenEnergyReady(ActionType action) const;
    int getSupplyInProgress() const;
    int getNextFinishTime(ActionType type) const;

    void getSpecialAbilityTargets(ActionSetAbilities & actionSet, int index) const;
    void storeChronoBoostTargets(ActionSetAbilities & actionSet, int index) const;
    bool chronoBoostableTarget(const Unit & unit) const;
    bool canChronoBoostTarget(const Unit & unit) const;
    bool canChronoBoost() const;

    int getNumInProgress(ActionType action) const;
    int getNumCompleted(ActionType action) const;
    int getNumTotal(ActionType action) const;
    void getLegalActions(std::vector<ActionType> & legalActions) const;
    bool isLegal(ActionType type) const;
    bool haveType(ActionType action) const;

    bool doAbility(ActionType type, int targetID);
    void doAction(ActionType type);
    void fastForward(int frames);
    void addUnit(ActionType Unit, int builderID = -1);
  
    void setMinerals(float minerals) { m_minerals = minerals; }
    void setGas(float gas) { m_gas = gas; }

    bool canBuildNow(ActionType action) const { return whenCanBuild(action) == getCurrentFrame(); }
    int getNumMineralWorkers() const { return m_mineralWorkers; }
    int getNumGasWorkers() const { return m_gasWorkers; }
    int getNumberChronoBoostsCast() const { return (int) m_chronoBoosts.size(); }
    int getNumUnits() const { return m_numUnits; }
    int getLastActionFinishTime() const { return m_unitsBeingBuilt.empty() ? getCurrentFrame() : m_units[m_unitsBeingBuilt.front()].getTimeUntilBuilt(); }
    int	getCurrentSupply() const { return m_currentSupply; }
    int	getMaxSupply() const { return m_maxSupply; }
    int getCurrentFrame() const { return m_currentFrame; }
    RaceID getRace() const { return m_race; }
    float getMinerals() const { return m_minerals; }
    float getGas() const { return m_gas; }
    const Vector_abilityaction &getChronoBoostTargets() const { return m_chronoBoosts; }
    const Vector_sizet &getFinishedUnits() const { return m_unitsSortedEndFrame; }
    ActionType getUnitType(int id) const { return m_units[id].getType(); }
    ActionType getLastAction() const { return m_lastAction; }
    const AbilityAction &getLastAbility() const { return m_lastAbility; }
    const Unit &getUnit(int id) const
    { 
      //BOSS_ASSERT(id < m_numUnits, "getUnit() called with invalid id");
      return m_units[id]; 
    }
    
    std::string	toString() const;
    void printunitsbeingbuilt() const;
  };
}
