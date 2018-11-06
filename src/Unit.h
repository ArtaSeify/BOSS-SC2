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
    uint2       m_frameStarted;     // the frame the production of this unit started
    uint2       m_frameFinished;    // the frame the production of this unit finished
    short       m_builderID;        // id of the unit that built this Unit
    ActionType  m_type;             // type of this Unit
    ActionType  m_addon;            // type of completed addon this Unit has
    ActionType  m_buildType;        // type of the Unit currently being built by this Unit
    short       m_buildID;          // id of the Unit currently being built by this Unit
    short       m_job;              // current job this Unit has (UnitJobs::XXX)
    short       m_timeUntilBuilt;   // time remaining until this Unit is completed
    short       m_timeUntilFree;    // time remaining until this Unit can build again
    uint2		m_timeChronoBoost;	// time remaining on Chrono Boost
    uint2       m_timeChronoBoostAgain; // time until chronoboost can be used on this building again
    short       m_numLarva;         // number of larva this building currently has (Hatch only)
    float       m_maxEnergyAllowed; // maximum energy allowed for this building
	float		m_energy;			// energy of the building

public:
    Unit();
    //Unit(ActionType type, uint4 id, int builderID, uint4 frameStarted);
    void        initializeUnit(ActionType type, short id, short builderID, uint2 frameStarted);

    short       getID()                     const   { return m_id; }
    short       getBuilderID()              const   { return m_builderID; }
    short       getBuildID()                const   { return m_buildID; }
    uint2       getStartFrame()             const   { return m_frameStarted; }
    uint2       getFinishFrame()            const   { return m_frameFinished; }
    short       getTimeUntilFree()          const   { return m_timeUntilFree; }
    short       getTimeUntilBuilt()         const   { return m_timeUntilBuilt; }
    uint2       getChronoBoostAgainTime()   const   { return m_timeChronoBoostAgain; }
    float       getEnergy()                 const   { return m_energy; }
    ActionType  getType()                   const   { return m_type; }
    ActionType  getAddon()                  const   { return m_addon; }
    ActionType  getBuildType()              const   { return m_buildType; }

    void        setTimeUntilBuilt(short time)       { m_timeUntilBuilt = time; }
    void        setBuilderID(short id)              { m_builderID = id; }
    void        reduceEnergy(int energy)            { m_energy -= energy; }

    void        applyChronoBoost(uint2 time, Unit & unitBeingProduced);
    void        castAbility(ActionType type, Unit & abilityTarget, Unit & abilityTargetProduction);
    void        complete(uint2 frameFinished);
    void        startBuilding(Unit & Unit);
    void        fastForward(uint2 frames);
    short       whenCanBuild(ActionType type) const;
};

}