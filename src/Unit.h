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
    size_t      m_id;               // index in GameState::m_Units
    size_t      m_frameStarted;     // the frame the production of this unit started
    size_t      m_frameFinished;    // the frame the production of this unit finished
    size_t      m_builderID;        // id of the unit that built this Unit
    ActionType  m_type;             // type of this Unit
    ActionType  m_addon;            // type of completed addon this Unit has
    ActionType  m_buildType;        // type of the Unit currently being built by this Unit
    size_t      m_buildID;          // id of the Unit currently being built by this Unit
    int         m_job;              // current job this Unit has (UnitJobs::XXX)
    int         m_timeUntilBuilt;   // time remaining until this Unit is completed
    int         m_timeUntilFree;    // time remaining until this Unit can build again
	int		    m_timeChronoBoost;	// time remaining on Chrono Boost
    int         m_timeChronoBoostAgain; // time until chronoboost can be used on this building again
	int         m_numLarva;         // number of larva this building currently has (Hatch only)
    double      m_maxEnergyAllowed; // maximum energy allowed for this building
	double		m_energy;			// energy of the building

public:

    Unit(ActionType type, size_t id, int builderID, size_t frameStarted);

    const int getTimeUntilFree() const;
    const int getTimeUntilBuilt() const;
    void setTimeUntilBuilt(int time);
    ActionType getType() const;
    ActionType getAddon() const;
    ActionType getBuildType() const;
    size_t getID() const;
    size_t getBuilderID() const;
    size_t getBuildID() const;
    size_t getStartFrame() const;
    size_t getFinishFrame() const;

    int getChronoBoostAgainTime() const;
    void applyChronoBoost(int time, Unit & unitBeingProduced);
    void castAbility(ActionType type, Unit & abilityTarget, Unit & abilityTargetProduction);
	const double & getEnergy() const;
    void reduceEnergy(int energy);

    int whenCanBuild(ActionType type) const;

    void complete(size_t frameFinished);
    void setBuilderID(int id);
    void startBuilding(Unit & Unit);
    void fastForward(int frames);
};

}