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
    NumUnits    m_id;               // index in GameState::m_Units
    TimeType    m_frameStarted;     // the frame the production of this unit started
    TimeType    m_frameFinished;    // the frame the production of this unit finished
    NumUnits    m_builderID;        // id of the unit that built this Unit
    ActionType  m_type;             // type of this Unit
    ActionType  m_addon;            // type of completed addon this Unit has
    ActionType  m_buildType;        // type of the Unit currently being built by this Unit
    NumUnits    m_buildID;          // id of the Unit currently being built by this Unit
    NumUnits    m_job;              // current job this Unit has (UnitJobs::XXX)
    TimeType    m_timeUntilBuilt;   // time remaining until this Unit is completed
    TimeType    m_timeUntilFree;    // time remaining until this Unit can build again
    TimeType    m_timeChronoBoost;    // time remaining on Chrono Boost
    TimeType    m_timeChronoBoostAgain; // time until chronoboost can be used on this building again
    //int1        m_numLarva;         // number of larva this building currently has (Hatch only)
    FracType    m_maxEnergyAllowed; // maximum energy allowed for this building
    FracType    m_energy;            // energy of the building
    bool        m_morphed;           // has the building been morphed. if true, this unit is no longer used for anything, but we keep it around as a placeholder

public:
    Unit();
    Unit(ActionType type, NumUnits id, NumUnits builderID, TimeType frameStarted);
    //void        initializeUnit(ActionType type, ActionID id, NumUnits builderID, TimeType frameStarted);

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
    bool        getMorphed()                const   { return m_morphed; }

    void        setTimeUntilBuilt(TimeType time)    { m_timeUntilBuilt = time; }
    void        setTimeUntilFree(TimeType time)     { m_timeUntilFree = time; }
    void        setBuilderID(NumUnits id)           { m_builderID = id; }
    void        reduceEnergy(FracType energy)       { m_energy -= energy; }

    void        applyChronoBoost(TimeType time, Unit & unitBeingProduced);
    void        castAbility(ActionType type, Unit & abilityTarget, Unit & abilityTargetProduction);
    void        complete(TimeType frameFinished);
    void        startBuilding(Unit & Unit);
    //void        morph(ActionType newType);
    void        fastForward(TimeType frames);
    int         whenCanBuild(ActionType type) const;

    // writes the data of a unit in CSV format to a ss
    void        writeToSS(std::stringstream & ss) const;
    json        writeToJson() const;
};

}
