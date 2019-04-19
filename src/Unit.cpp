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
    , m_morphID                 (-1)
    , m_morphed                 (false)
{
}

//Unit::Unit(ActionType type)
//    : m_id(-1)
//    , m_frameStarted(-1)
//    , m_frameFinished(-1)
//    , m_builderID(-1)
//    , m_type(type)
//    , m_addon(ActionTypes::None)
//    , m_buildType(ActionTypes::None)
//    , m_buildID(0)
//    , m_job(UnitJobs::None)
//    , m_timeUntilBuilt(0)
//    , m_timeUntilFree(0)
//    //!!! PROBLEM UNUSED    , m_numLarva                (0)
//    , m_timeChronoBoost(0)
//    , m_timeChronoBoostAgain(0)
//    , m_maxEnergyAllowed(0)
//    , m_energy(0)
//    , m_morphed(false)
//{
//}

Unit::Unit(ActionType type, NumUnits id, NumUnits builderID, TimeType frameStarted)
    : m_id                      (id)
    , m_frameStarted            (frameStarted)
    , m_frameFinished           (-1)
    , m_builderID               (builderID)
    , m_type                    (type)
    , m_addon                   (ActionTypes::None)
    , m_buildType               (ActionTypes::None)
    , m_buildID                 (0)
    , m_job                     (UnitJobs::None)
    , m_timeUntilBuilt          (builderID != -1 ? type.buildTime() : 0)
    , m_timeUntilFree           (builderID != -1 ? type.buildTime() : 0)
    //, m_numLarva                (0)
    , m_timeChronoBoost         (0)
    , m_timeChronoBoostAgain    (0)
    , m_maxEnergyAllowed        (float(type.maxEnergy()))
    , m_energy                  (float(type.startingEnergy()))
    , m_morphID                 (-1)
    , m_morphed                 (false)
{
    
}

void Unit::startBuilding(Unit & Unit)
{
    if (m_morphed)
    {
        return;
    }

    if (Unit.getType().isMorphed())
    {
        m_morphID = Unit.getID();
        return;
    }

    // if it's not a probe, this Unit won't be free until the build time is done
    if (!m_type.isWorker() || m_type.getRace() != Races::Protoss)
    {
        if (Unit.getType().whatBuildsStatus() != "None")
        {
            m_timeUntilFree = Unit.getType().buildTime();
        }

        if (Unit.getType().whatBuilds() == ActionTypes::GetActionType("WarpGate"))
        {
            std::string unitName = Unit.getType().getName();
            if (unitName == "ZealotWarped")
            {
                m_timeUntilFree = 448;
            }
            else if (unitName == "StalkerWarped")
            {
                m_timeUntilFree = 512;
            }
            else if (unitName == "AdeptWarped")
            {
                m_timeUntilFree = 448;
            }
            else if (unitName == "SentryWarped")
            {
                m_timeUntilFree = 512;
            }
            else if (unitName == "HighTemplarWarped")
            {
                m_timeUntilFree = 720;
            }
            else if (unitName == "DarkTemplarWarped")
            {
                m_timeUntilFree = 720;
            }           
        }
    }
    
    m_buildType = Unit.getType();
    m_buildID = Unit.getID();

    // the building might still be chrono boosted
    if (m_timeChronoBoost > 0)
    {
        applyChronoBoost(m_timeChronoBoost, Unit);
    }
}

//void Unit::morph(ActionType newType)
//{
//    BOSS_ASSERT(newType.isMorphed(), "function 'morph' called with a non-morph action");
//    
//    m_type = newType;
//    m_buildType = newType;
//    m_buildID = m_id;
//
//    m_timeUntilBuilt = newType.buildTime();
//    m_timeUntilFree = newType.buildTime();
//}

void Unit::complete(TimeType frameFinished)
{
    m_timeUntilFree = 0;
    m_timeUntilBuilt = 0;
    m_frameFinished = frameFinished;
}

void Unit::fastForward(TimeType frames)
{
    if (m_morphed)
    {
        return;
    }

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
    m_timeUntilFree         = (TimeType)std::max(0, m_timeUntilFree - frames);
    m_timeUntilBuilt        = (TimeType)std::max((TimeType)0, m_timeUntilBuilt);
    m_timeChronoBoostAgain  = (TimeType)std::max(0, m_timeChronoBoostAgain - frames);
    m_timeChronoBoost       = (TimeType)std::min(m_timeChronoBoost, m_timeChronoBoostAgain);
}

// returns when this Unit can build a given type, -1 if it can't
int Unit::whenCanBuild(ActionType type) const
{
    if (m_morphed)
    {
        return -1;
    }

    // check to see if this type can build the given type
    // TODO: check equivalent types (hatchery gspire etc)
    //if (type.whatBuilds() != m_type) { return -1; }
    BOSS_ASSERT(type.whatBuilds() == m_type, "Unit %s doesn't build type %s", m_type.getName().c_str(), type.getName().c_str());

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

void Unit::castAbility(ActionType type, Unit & abilityTarget, Unit & abilityTargetProduction, Unit & abilityTargetMorphed)
{
    BOSS_ASSERT(type.whatBuilds() == m_type, "Ability %s can't be cast by unit %s on unit %s", type.getName().c_str(), m_type.getName().c_str(), abilityTarget.getType().getName().c_str());

    // reduce energy of unit based on energy cost of action
    reduceEnergy(FracType(type.energyCost()));

    // use ability on target
    if (type.getName() == "ChronoBoost")
    {
        int change = abilityTarget.applyChronoBoost(type.buildTime(), abilityTargetProduction);

        // if chronoboosting a unit that a gateway is producing and we have researched warpgate, then the build time
        // of the warpgate is also reduced
        if (abilityTarget.getMorphID() != -1)
        {
            abilityTargetMorphed.setTimeUntilBuilt(abilityTargetMorphed.getTimeUntilBuilt() - change);
            abilityTargetMorphed.setTimeUntilFree(abilityTargetMorphed.getTimeUntilBuilt());
        }
    }
}

int Unit::applyChronoBoost(TimeType time, Unit & unitBeingProduced)
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

    int change = m_timeUntilFree - newTimeUntilFree;
    m_timeUntilFree = newTimeUntilFree;

    if (!m_type.isMorphed())
    {
        unitBeingProduced.setTimeUntilBuilt(m_timeUntilFree);
        unitBeingProduced.setTimeUntilFree(m_timeUntilFree);
    }

    return change;
}

std::string Unit::toString() const
{
    std::stringstream ss;
    ss << m_type.getName() << ", ID: " << m_id << std::endl;
    ss << "Frame started: " << m_frameStarted << ". Frame finished: " << m_frameFinished << std::endl;
    ss << "BuilderID: " << m_builderID << std::endl;
    ss << "Currently building: " << m_buildType.getName() << " with ID: " << m_buildID << std::endl;
    ss << "Time until built: " << m_timeUntilBuilt << ". Time until free: " << m_timeUntilFree << std::endl;
    ss << "Time Chrono Boosted: " << m_timeChronoBoost << ". Time Chrono Boost again: " << m_timeChronoBoostAgain << std::endl;
    ss << "Energy: " << m_energy << "/" << m_maxEnergyAllowed << std::endl;
    ss << "Morphed: " << (m_morphed ? "true" : "false") << std::endl;
    if (m_morphed)
    {
        ss << "MorphID: " << m_morphID << std::endl;
    }

    return ss.str();
}

void Unit::writeToSS(std::stringstream & ss) const
{    
    ss << "[";
    ss << m_id << ",";
    ss << m_frameStarted << ",";
    ss << m_frameFinished << ",";
    ss << m_builderID << ",";
    ss << m_type.getID() << ",";
    ss << m_addon.getID() << ",";
    ss << m_buildType.getID() << ",";
    ss << m_buildID << ",";
    ss << m_job << ",";
    ss << m_timeUntilBuilt << ",";
    ss << m_timeUntilFree << ",";
    ss << m_timeChronoBoost << ",";
    ss << m_timeChronoBoostAgain << ",";
    //ss << m_numLarva << ", ";
    ss << m_maxEnergyAllowed << ",";
    ss << m_energy;
    ss << m_morphID;
    ss << m_morphed << "]";
    ss << "]";
}

json Unit::writeToJson() const
{
    json data;

    data["ID"] = m_id;
    data["FrameStarted"] = m_frameStarted;
    data["FrameFinished"] = m_frameFinished;
    data["BuilderID"] = m_builderID;
    data["TypeID"] = m_type.getID();
    data["AddonID"] = m_addon.getID();
    data["BuildTypeID"] = m_buildType.getID();
    data["BuildID"] = m_buildID;
    data["Job"] = m_job;
    data["TimeUntilBuilt"] = m_timeUntilBuilt;
    data["TimeUntilFree"] = m_timeUntilFree;
    data["TimeChronoBoost"] = m_timeChronoBoost;
    data["TimeChronoBoostAgain"] = m_timeChronoBoostAgain;
    data["MaxEnergyAllowed"] = m_maxEnergyAllowed;
    data["Energy"] = m_energy;
    data["MorphID"] = m_morphID;
    data["Morphed"] = m_morphed;

    return data;
}