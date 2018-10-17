#include "Unit.h"

using namespace BOSS;

Unit::Unit(const ActionType & type, const size_t & id, int builderID)
    : m_job                     (UnitJobs::None)
    , m_id                      (id)
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
	, m_energy			        (type.startingEnergy())
{
    
}

void Unit::startBuilding(Unit & Unit)
{
    // if it's not a probe, this Unit won't be free until the build time is done
    if (!m_type.isWorker() || !m_type.getRace() == Races::Protoss)
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

void Unit::complete()
{
    m_timeUntilFree = 0;
    m_timeUntilBuilt = 0;
}

void Unit::fastForward(const int & frames)
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

	m_energy = std::min(m_energy + std::max(0, frames - m_timeUntilBuilt) * CONSTANTS::ERPF, 
						double(m_type.maxEnergy()));

    m_timeUntilFree = std::max(0, m_timeUntilFree - frames);
    m_timeUntilBuilt = std::max(0, m_timeUntilBuilt - frames);
    m_timeChronoBoostAgain = std::max(0, m_timeChronoBoostAgain - frames);
    m_timeChronoBoost = std::max(0.0, std::min(m_timeChronoBoost, double(m_timeChronoBoostAgain)));
}

// returns when this Unit can build a given type, -1 if it can't
int Unit::whenCanBuild(const ActionType & type) const
{
    // check to see if this type can build the given type
    // TODO: check equivalent types (hatchery gspire etc)
    if (type.whatBuilds() != m_type) { return -1; }

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

    // we can always use an ability if we have enough energy
    if (type.isAbility())
    {
        return ((int)std::ceil(std::max(0.0, (type.energyCost() - m_energy)/CONSTANTS::ERPF)));
    }

    return (int)std::ceil(m_timeUntilFree);
}

void Unit::castAbility(const ActionType & type, Unit & abilityTarget, Unit & abilityTargetProduction)
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

void Unit::applyChronoBoost(const double & time, Unit & unitBeingProduced)
{
    //std::cout << "Chronoboost on: " << m_type.getName() << std::endl;
    //std::cout << "Current target has chronoboost timer: " << m_timeChronoBoost << std::endl;
    //std::cout << "Current target will have chronoboost timer: " << time << std::endl;
    if (m_timeChronoBoostAgain == 0)
    {
        m_timeChronoBoostAgain = time;
    }

    m_timeChronoBoost = time;

    BOSS_ASSERT(m_timeUntilFree > 0, "Chrono Boost used on target that is not producing anything");
    //std::cout << "Previous build time: " << m_timeUntilFree << std::endl;;
    //std::cout << "Previous chrono boost time: " << m_timeChronoBoost << std::endl;
    // Chrono Boost speeds up production by 50%
    double newTimeUntilFree;
    // make changes to remaining production time and chronoboost time
    if (m_timeChronoBoost >= m_timeUntilFree / 1.5)
    {
        newTimeUntilFree = m_timeUntilFree / 1.5;
        m_timeChronoBoost -= newTimeUntilFree;
    }
    else
    {
        newTimeUntilFree = m_timeUntilFree - (m_timeChronoBoost / 2.0);
        m_timeChronoBoost = 0;
    }

    /*if (m_buildType == ActionTypes::GetActionType("Zealot"))
    {
        std::cout << "before chronoboost" << m_timeUntilFree << std::endl;
    }*/

    m_timeUntilFree = static_cast <int> (std::ceil(newTimeUntilFree));
    unitBeingProduced.setTimeUntilBuilt(m_timeUntilFree);

    /*if (m_buildType == ActionTypes::GetActionType("Zealot"))
    {
        std::cout << "after chronoboost" << m_timeUntilFree << std::endl;
    }*/
}

const int & Unit::getTimeUntilBuilt() const
{
    return m_timeUntilBuilt;
}

void Unit::setTimeUntilBuilt(const int & time)
{
    m_timeUntilBuilt = time;
}

const int Unit::getTimeUntilFree() const
{
    return static_cast<int>(std::ceil(m_timeUntilFree));
}

const ActionType & Unit::getType() const
{
    return m_type;
}

const ActionType & Unit::getAddon() const
{
    return m_addon;
}

const ActionType & Unit::getBuildType() const
{
    return m_buildType;
}

const size_t & Unit::getID() const
{
    return m_id;
}

const size_t & Unit::getBuilderID() const
{
    return m_builderID;
}

const double & Unit::getEnergy() const
{
	return m_energy;
}

const double & Unit::getChronoBoostAgainTime() const
{
    return m_timeChronoBoostAgain;
}

const size_t & Unit::getBuildID() const
{
    return m_buildID;
}

void Unit::reduceEnergy(const double & energy)
{
    m_energy -= energy;
}

void Unit::setBuilderID(const int & id)
{
    m_builderID = id;
}