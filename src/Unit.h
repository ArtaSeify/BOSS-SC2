/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"

namespace BOSS
{

namespace UnitJobs
{
    enum { None, Minerals, Gas, Build };
}

class Unit
{
    short       m_id;               // index in GameState::m_Units
    int         m_frameStarted;     // the frame the production of this unit started
    int         m_frameFinished;    // the frame the production of this unit finished
    short       m_builderID;        // id of the unit that built this Unit
    ActionType  m_type;             // type of this Unit
    ActionType  m_addon;            // type of completed addon this Unit has
    ActionType  m_buildType;        // type of the Unit currently being built by this Unit
    short       m_buildID;          // id of the Unit currently being built by this Unit
    short       m_job;              // current job this Unit has (UnitJobs::XXX)
    int         m_timeUntilBuilt;   // time remaining until this Unit is completed
    int         m_timeUntilFree;    // time remaining until this Unit can build again
    int         m_timeChronoBoost;	// time remaining on Chrono Boost
    int         m_timeChronoBoostAgain; // time until chronoboost can be used on this building again
  //!!! PROBLEM UNUSED short       m_numLarva;         // number of larva this building currently has (Hatch only)
    float       m_maxEnergyAllowed; // maximum energy allowed for this building
    float	m_energy;			// energy of the building

public:
    Unit();
    //Unit(ActionType type, uint4 id, int builderID, uint4 frameStarted);
    void        initializeUnit(ActionType type, int id, int builderID, int frameStarted);

    int         getID()                     const   { return m_id; }
    int         getBuilderID()              const   { return m_builderID; }
    int         getBuildID()                const   { return m_buildID; }
    int         getStartFrame()             const   { return m_frameStarted; }
    int         getFinishFrame()            const   { return m_frameFinished; }
    int         getTimeUntilFree()          const   { return m_timeUntilFree; }
    int         getTimeUntilBuilt()         const   { return m_timeUntilBuilt; }
    int         getChronoBoostAgainTime()   const   { return m_timeChronoBoostAgain; }
    float       getEnergy()                 const   { return m_energy; }
    ActionType  getType()                   const   { return m_type; }
    ActionType  getAddon()                  const   { return m_addon; }
    ActionType  getBuildType()              const   { return m_buildType; }

    void        setTimeUntilBuilt(int time)       { m_timeUntilBuilt = time; }
    void        setBuilderID(int id)              { m_builderID = id; }
    void        reduceEnergy(float energy)            { m_energy -= energy; }

    void        applyChronoBoost(int time, Unit & unitBeingProduced);
    void        castAbility(ActionType type, Unit & abilityTarget, Unit & abilityTargetProduction);
    void        complete(int frameFinished);
    void        startBuilding(Unit & Unit);
    void        fastForward(int frames);
    int       whenCanBuild(ActionType type) const;
};

}
