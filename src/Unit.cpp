/* -*- c-basic-offset: 4 -*- */

#include "Unit.h"

using namespace BOSS;

Unit::Unit()
    : m_id                      (-1)
    , m_frameStarted            (-1)
    , m_frameFinished           (-1)
    , m_builderID               (-1)
    , m_type                    (-1)
    , m_addon                   (ActionTypes::None)
    , m_buildType               (ActionTypes::None)
    , m_buildID                 (0)
    , m_job                     (UnitJobs::None)
    , m_timeUntilBuilt          (0)
    , m_timeUntilFree           (0)
      //!!! PROBLEM UNUSED    , m_numLarva                (0)
    , m_timeChronoBoost         (0)
    , m_timeChronoBoostAgain    (0)
    , m_maxEnergyAllowed        (0)
    , m_energy                  (0)
{
}

/*Unit::Unit(ActionType type, uint4 id, int builderID, uint4 frameStarted)
  : m_job                     (UnitJobs::None)
  , m_id                      (id)
  , m_frameStarted            (frameStarted)
  , m_frameFinished           (-1)
  , m_type                    (type)
  , m_addon                   (ActionTypes::None)
  , m_buildType               (ActionTypes::None)
  , m_buildID                 (0)
  , m_timeUntilBuilt          (builderID != -1 ? type.buildTime() : 0)
  , m_timeUntilFree           (builderID != -1 ? type.buildTime() : 0)
  , m_numLarva                (0)
  , m_builderID               (builderID)
  , m_timeChronoBoost         (0)
  , m_timeChronoBoostAgain    (0)
  , m_maxEnergyAllowed        (float(type.maxEnergy()))
  , m_energy			        (float(type.startingEnergy()))
  {
    
  }*/

void Unit::initializeUnit(ActionType type, int id, int builderID, int frameStarted)
{
    m_id = id;
    m_frameStarted = frameStarted;
    m_type = type;
    m_timeUntilBuilt = builderID != -1 ? type.buildTime() : 0;
    m_timeUntilFree = builderID != -1 ? type.buildTime() : 0;
    m_builderID = builderID;
    m_maxEnergyAllowed = float(type.maxEnergy());
    m_energy = float(type.startingEnergy());
}

    void Unit::startBuilding(Unit & Unit)
    {
        // if it's not a probe, this Unit won't be free until the build time is done
        // !! Double check. Think it was a mistake
        //if (!m_type.isWorker() || !m_type.getRace() == Races::Protoss)
        if (!m_type.isWorker() || m_type.getRace() != Races::Protoss)
        {
            if (Unit.getType().whatBuildsStatus() != "None")
            {
                m_timeUntilFree = Unit.getType().buildTime();
            }
        }
    
        m_buildType = Unit.getType();
        m_buildID = Unit.getID();

        if (Unit.getType().isMorphed())
        {
            m_type = Unit.getType();
        }

        // the building might still be chrono boosted
        if (m_timeChronoBoost > 0)
        {
            applyChronoBoost(m_timeChronoBoost, Unit);
        }
    }

void Unit::complete(int frameFinished)
{
    m_timeUntilFree = 0;
    m_timeUntilBuilt = 0;
    m_frameFinished = frameFinished;
}

void Unit::fastForward(int frames)
{
    // if we are completing the thing that this Unit is building
    if ((m_buildType != ActionTypes::None) && frames >= m_timeUntilFree)
    {
        if (m_buildType.isAddon())
        {
            m_addon = m_buildType;
        }

        m_buildType = ActionTypes::None;
        m_buildID = 0;
        m_job = m_type.isWorker() ? UnitJobs::Minerals : UnitJobs::None;
    }
    // calculate energy. a building starts getting energy the moment it is built
    m_timeUntilBuilt -= frames;
    if (m_maxEnergyAllowed > 0 && m_timeUntilBuilt < 0)
    {
        m_energy += std::abs(m_timeUntilBuilt) * CONSTANTS::ERPF;
        m_energy = std::min(m_energy, m_maxEnergyAllowed);
    }
    m_timeUntilFree = std::max(0, m_timeUntilFree - frames);
    m_timeUntilBuilt = std::max(0, m_timeUntilBuilt);
    m_timeChronoBoostAgain = std::max(0, m_timeChronoBoostAgain - frames); 
    m_timeChronoBoost = std::min(m_timeChronoBoost, m_timeChronoBoostAgain);
}

// returns when this Unit can build a given type, -1 if it can't
int Unit::whenCanBuild(ActionType type) const
{
    // check to see if this type can build the given type
    // TODO: check equivalent types (hatchery gspire etc)
    if (type.whatBuilds() != m_type) { return -1; }

    // we can always use an ability if we have enough energy
    if (type.isAbility())
    {
        return ((int)std::ceil(std::max(0.0f, m_timeUntilBuilt + ((type.energyCost() - m_energy) / CONSTANTS::ERPF))));
    }

    // if it requires an addon and we won't ever have one, we can't build it
    if (type.whatBuildsAddon() != ActionTypes::None && type.whatBuildsAddon() != m_addon && type.whatBuildsAddon() != m_buildType)
    {
        return -1;
    }

    // if this is a worker and it's harvesting gas, it can't build
    if (m_type.isWorker() && (m_job == UnitJobs::Gas))
    {
        return -1;
    }

    return m_timeUntilFree;
}

    void Unit::castAbility(ActionType type, Unit & abilityTarget, Unit & abilityTargetProduction)
    {
        BOSS_ASSERT(type.whatBuilds() == m_type, "Ability %s can't be cast by unit %s on unit %s", type.getName().c_str(), m_type.getName().c_str(), abilityTarget.getType().getName().c_str());

        // reduce energy of unit based on energy cost of action
        reduceEnergy(type.energyCost());

        // use ability on target
        if (type.getName() == "ChronoBoost")
        {
            abilityTarget.applyChronoBoost(type.buildTime(), abilityTargetProduction);
        }
    }

void Unit::applyChronoBoost(int time, Unit & unitBeingProduced)
{
    if (m_timeChronoBoostAgain > 0)
    {
        BOSS_ASSERT(time < ActionTypes::GetActionType("ChronoBoost").buildTime(), "Can't Chrono Boost %s yet", m_type.getName().c_str());
    }
    
    if (m_timeChronoBoostAgain == 0)
    {
        m_timeChronoBoostAgain = time;
    }
    m_timeChronoBoost = time;

    BOSS_ASSERT(m_timeUntilFree > 0, "Chrono Boost used on %s, but it is not producing anything, %f", m_type.getName().c_str(), m_timeUntilFree);
    BOSS_ASSERT(unitBeingProduced.getTimeUntilBuilt() > 0, "Chrono Boost used on target that is not producing anything");

    // Chrono Boost speeds up production by 50%
    int newTimeUntilFree = (int)std::ceil(m_timeUntilFree / 1.5);
    // make changes to remaining production time and chronoboost time
    if (m_timeChronoBoost >= newTimeUntilFree)
    {
        m_timeChronoBoost -= newTimeUntilFree;
    }
    else
    {
        newTimeUntilFree = (int)std::ceil(m_timeUntilFree - (m_timeChronoBoost / 2.0));
        m_timeChronoBoost = 0;
    }

    m_timeUntilFree = newTimeUntilFree;
    unitBeingProduced.setTimeUntilBuilt(m_timeUntilFree);
}

