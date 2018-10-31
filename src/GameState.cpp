#include "GameState.h"
#include <numeric>
#include <iomanip>

using namespace BOSS;

GameState::GameState()
    : m_minerals(0.0f)
    , m_gas(0.0f)
    , m_race(Races::None)
    , m_currentSupply(0)
    , m_maxSupply(0)
    , m_currentFrame(0)
    , m_previousFrame(0)
    , m_mineralWorkers(0)
    , m_gasWorkers(0)
    , m_buildingWorkers(0)
    , m_numRefineries(0)
    , m_numDepots(0)
    , m_lastAction(ActionTypes::None)
{
    //m_units.reserve(20);
    //m_unitsBeingBuilt.reserve(2);
    //m_chronoBoosts.reserve(2);
    //m_unitsSortedEndFrame.reserve(2);
    //m_armyUnits.reserve(2);
}

GameState::GameState(const GameState & state)
{
    m_race = state.m_race;
    m_minerals = state.m_minerals;
    m_gas = state.m_gas;
    m_currentSupply = state.m_currentSupply;
    m_maxSupply = state.m_maxSupply;
    m_currentFrame = state.m_currentFrame;
    m_previousFrame = state.m_previousFrame;
    m_mineralWorkers = state.m_mineralWorkers;
    m_gasWorkers = state.m_gasWorkers;
    m_buildingWorkers = state.m_buildingWorkers;
    m_numRefineries = state.m_numRefineries;
    m_numDepots = state.m_numDepots;
    m_lastAction = state.m_lastAction;

    m_units = state.m_units;
    m_unitsBeingBuilt = state.m_unitsBeingBuilt;
    m_chronoBoosts = state.m_chronoBoosts;
    m_unitsSortedEndFrame = state.m_unitsSortedEndFrame;
    m_armyUnits = state.m_armyUnits;

    //m_units.reserve(m_units.size() + 1);
    //m_unitsBeingBuilt.reserve(m_unitsBeingBuilt.size() + 1);
    //m_chronoBoosts.reserve(m_chronoBoosts.size() + 1);
    //m_unitsSortedEndFrame.reserve(m_unitsSortedEndFrame.size() + 3);
    //m_armyUnits.reserve(m_armyUnits.size() + 3);
}

void GameState::getLegalActions(std::vector<ActionType> & legalActions) const
{
    legalActions.clear();
    for (auto & type : ActionTypes::GetAllActionTypes())
    {
        if (isLegal(type)) { legalActions.push_back(type); }
    }
}

bool GameState::isLegal(const ActionType & action) const
{
    // if the race can't do the action
    if (action.getRace() != m_race) { return false; }    
    
    const size_t mineralWorkers			= m_mineralWorkers + m_buildingWorkers;
    const size_t refineriesInProgress	= getNumInProgress(ActionTypes::GetRefinery(m_race));
    const size_t numRefineries			= m_numRefineries + refineriesInProgress;
    const size_t numDepots				= m_numDepots + getNumInProgress(ActionTypes::GetResourceDepot(m_race));

    // we can Chrono Boost as long as we have a Nexus
    if (action.isAbility() && m_race == Races::GetRaceID("Protoss") && numDepots == 0) { return false; }

    // if we have no mineral workers, we can't make any unit
    if (mineralWorkers == 0) { return false; }
        
    // if it's a unit and we are out of supply and aren't making a supply providing unit, it's not legal
    if (!action.isMorphed() && !action.isSupplyProvider() && ((m_currentSupply + action.supplyCost()) > (m_maxSupply + getSupplyInProgress()))) { return false; }

    // TODO: require an extra for refineries but not buildings
    // rules for buildings which are built by workers
    if (action.isBuilding() && !action.isMorphed() && !action.isAddon() && (mineralWorkers == 0)) { return false; }

    // if we have no gas income we can't make a gas unit
    if ((m_gas < action.gasPrice()) && (m_gasWorkers == 0)) { return false; }

    // if we have no mineral income we'll never have a minerla unit
    if ((m_minerals < action.mineralPrice()) && (mineralWorkers == 0)) { return false; }

    // don't build more refineries than resource depots
    if (action.isRefinery() && (numRefineries >= 2 * numDepots)) { return false; }

    // don't build a refinery if we don't have enough mineral workers to transfer over
    if (action.isRefinery() && (mineralWorkers <= (CONSTANTS::WorkersPerRefinery * (numRefineries + 1))))
    {
        //std::cout << "workers: " << mineralWorkers << ". Asking for " << CONSTANTS::WorkersPerRefinery << " workers for gas." << std::endl;
        return false; 
    }

    // we don't need to go over the maximum supply limit with supply providers
    if (action.isSupplyProvider() && (m_maxSupply + getSupplyInProgress() > 400)) { return false; }

    // TODO: can only build one of a tech type
    // TODO: check to see if an addon can ever be built
    if (!haveBuilder(action)) { return false; }

    if (!havePrerequisites(action)) { return false; }

    return true;
}

void GameState::doAction(const ActionType & type)
{
    BOSS_ASSERT(!type.isAbility(), "doAction should not be called with an ability");

    int previousFrame = m_currentFrame;
    m_lastAction = type;

    // figure out when this action can be done and fast forward to it
    const int timeWhenReady = whenCanBuild(type);
    fastForward(timeWhenReady);

    // the builder of action
    int buildID = getBuilderID(type);

    // subtract the resource cost
    m_minerals  -= type.mineralPrice();
    m_gas       -= type.gasPrice();

    // if it's a Terran building that's not an addon, the worker removed from minerals
    if (type.getRace() == Races::Terran && type.isBuilding() && !type.isAddon())
    {
        m_mineralWorkers--;
        m_buildingWorkers++;
    }

    // if it's a Zerg unit that requires a Drone, remove a mineral worker
    if (type.getRace() == Races::Zerg && type.whatBuilds().isWorker())
    {
        m_mineralWorkers--;
    }

    // get a builder for this type and start building it
    addUnit(type, buildID);

    return;
}

bool GameState::doAbility(const ActionType & type, size_t targetID)
{
    BOSS_ASSERT(type.isAbility(), "doAbility should not be called with a non-ability action");
    BOSS_ASSERT(targetID != -1, "Target of ability %s is invalid. Target ID: %u", type.getName().c_str(), targetID);

    int previousFrame = m_currentFrame;
    m_lastAction = type;

    // figure out when this action can be done and fast forward to it
    const int timeWhenReady = whenCanCast(type, targetID);
    // can't cast ability
    if (timeWhenReady == -1)
    {
        return false;
    }

    fastForward(timeWhenReady);

    if (m_race == Races::GetRaceID("Protoss"))
    {
        AbilityAction abilityAction(type, m_currentFrame, targetID, getUnit(targetID).getBuildID(), getUnit(targetID).getType());
        m_lastAbility = abilityAction;

        // cast chronoboost
        getUnit(getBuilderID(type)).castAbility(type, getUnit(targetID), getUnit(getUnit(targetID).getBuildID()));
        
        m_chronoBoosts.push_back(abilityAction);

        // have to resort the list, because build time of a unit is changed
        size_t index = find(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(), getUnit(targetID).getBuildID()) - m_unitsBeingBuilt.begin();
        for (size_t i = index; i < m_unitsBeingBuilt.size() - 1; ++i)
        {
            if (getUnit(m_unitsBeingBuilt[i]).getTimeUntilBuilt() < getUnit(m_unitsBeingBuilt[i + 1]).getTimeUntilBuilt())
            {
                std::swap(m_unitsBeingBuilt[i], m_unitsBeingBuilt[i + 1]);
            }
            else
            {
                break;
            }
        }
    }
    return true;
}

void GameState::fastForward(int toFrame)
{
    if (toFrame == m_currentFrame) { return; }

    BOSS_ASSERT(toFrame > m_currentFrame, "Must ff to the future");

    m_previousFrame             = m_currentFrame;
    int previousFrame           = m_currentFrame;
    int lastActionFinishTime    = m_currentFrame;
    
    // iterate backward over actions in progress since they're sorted
    // that way for ease of deleting the finished ones
    while (!m_unitsBeingBuilt.empty())
    {
        Unit & unit = getUnit(m_unitsBeingBuilt.back());
        ActionType type = unit.getType();

        // if the current action in progress will finish after the ff time, we can stop
        const int actionCompletionTime = previousFrame + unit.getTimeUntilBuilt();
        if (actionCompletionTime > toFrame) { break; }

        // add the resources we gathered during this time period
        const int timeElapsed	= actionCompletionTime - lastActionFinishTime;
        m_minerals              += timeElapsed * CONSTANTS::MPWPF * m_mineralWorkers;
        m_gas                   += timeElapsed * CONSTANTS::GPWPF * m_gasWorkers;
        lastActionFinishTime    = actionCompletionTime;
        m_currentFrame += timeElapsed;
        
        // if it's a Terran building that's not an addon, the worker returns to minerals
        if (type.getRace() == Races::Terran && type.isBuilding() && !type.isAddon())
        {
            m_mineralWorkers++;
        }

        // complete the action and remove it from the list
        completeUnit(unit);
        m_unitsBeingBuilt.pop_back();
    }

    // update resources from the last action finished to the ff frame
    int timeElapsed = toFrame - lastActionFinishTime;
    m_minerals      += timeElapsed * CONSTANTS::MPWPF * m_mineralWorkers;
    m_gas           += timeElapsed * CONSTANTS::GPWPF * m_gasWorkers;

    // update all the intances to the ff time
    for (auto & unit : m_units)
    {
        unit.fastForward(toFrame - previousFrame);
    }

    m_currentFrame += timeElapsed;
}

void GameState::completeUnit(Unit & unit)
{
    unit.complete(m_currentFrame);
    m_maxSupply += unit.getType().supplyProvided();

    //addUnitToSpecialVectors(unit.getID());

    // stores units in the order they were finished, except the units we start with
    if (m_units[unit.getID()].getBuilderID() != -1)
    {
        m_unitsSortedEndFrame.push_back(unit.getID());
    }

    // if it's a worker, assign it to the correct job
    if (unit.getType().isWorker())
    {
        m_mineralWorkers++;
        int needGasWorkers = std::max(0, (CONSTANTS::WorkersPerRefinery*m_numRefineries - m_gasWorkers));
        BOSS_ASSERT(needGasWorkers < m_mineralWorkers, "Shouldn't need more gas workers than we have mineral workers. "
                                        "%d required gas workers, %d mineral workers", needGasWorkers, m_mineralWorkers);
        m_mineralWorkers -= needGasWorkers;
        m_gasWorkers += needGasWorkers;
    }
    else if (unit.getType().isRefinery())
    {
        //std::cout << "mineral workers before refinery build: " << m_mineralWorkers << std::endl;
        m_numRefineries++;
        //std::cout << "we have " << m_numRefineries << " refineries, and " << (int)(m_numDepots + getNumInProgress(ActionTypes::GetResourceDepot(m_race))) << " bases." << std::endl;
        BOSS_ASSERT(m_numRefineries <= 2 * (int)(m_numDepots + getNumInProgress(ActionTypes::GetResourceDepot(m_race))), "Shouldn't have more refineries than 2*depots");
        int needGasWorkers = std::max(0, (CONSTANTS::WorkersPerRefinery*m_numRefineries - m_gasWorkers));
        BOSS_ASSERT(needGasWorkers < m_mineralWorkers, "Shouldn't need more gas workers than we have mineral workers. "
                                                       "%d required gas workers, %d mineral workers", needGasWorkers, m_mineralWorkers);
        m_mineralWorkers -= needGasWorkers;
        m_gasWorkers += needGasWorkers;
        //std::cout << "mineral workers after refinery build: " << m_mineralWorkers << std::endl;
    }
    else if (unit.getType().isDepot())
    {
        m_numDepots++;
    }
}

// add a unit of the given type to the state
// if builderID is -1 (default) the unit is added as completed, otherwise it begins construction with the builder
void GameState::addUnit(const ActionType & type, int builderID)
{
    BOSS_ASSERT(m_race == Races::None || type.getRace() == m_race, "Adding an Unit of a different race");

    m_race = type.getRace();
    Unit unit(type, m_units.size(), builderID, m_currentFrame);

    m_units.push_back(unit);
    m_currentSupply += unit.getType().supplyCost();
    
    // if we have a valid builder for this object, add it to the Units being built
    if (builderID != -1)
    {
        getUnit(builderID).startBuilding(m_units.back());

        // add the Unit ID being built and sort the list
        m_unitsBeingBuilt.push_back(unit.getID());

        // we know the list is already sorted when we add this unit, so we just swap it from the end until it's in the right place
        for (size_t i = m_unitsBeingBuilt.size() - 1; i > 0; i--)
        {
            if (getUnit(m_unitsBeingBuilt[i]).getTimeUntilBuilt() > getUnit(m_unitsBeingBuilt[i - 1]).getTimeUntilBuilt())
            {
                std::swap(m_unitsBeingBuilt[i], m_unitsBeingBuilt[i - 1]);
            }
            else
            {
                break;
            }
        }
    }
    // if there's no builder, complete the unit now and skip the unit in progress step
    else
    {
        completeUnit(m_units.back());
    }
}

void GameState::addUnitToSpecialVectors(size_t unitIndex)
{
    // we don't want to store units that we start with
    if (m_units[unitIndex].getBuilderID() == -1)
    {
        return;
    }

    // if it's a combat unit add it to the armyUnits vector
    /*const ActionType & type = m_units[unitIndex].getType();
    if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
    {
        m_armyUnits.push_back(unitIndex);
    }*/
  
    // add to finished units vector
    m_unitsSortedEndFrame.push_back(unitIndex);
}

int GameState::whenCanBuild(const ActionType & action) const
{
    if (action.isAbility())
    {
        return whenEnergyReady(action);
    }

    // figure out when prerequisites will be ready
    int maxTime = m_currentFrame;
    int prereqTime      = whenPrerequisitesReady(action);
    int resourceTime    = whenResourcesReady(action);
    int supplyTime      = whenSupplyReady(action);
    int builderTime     = whenBuilderReady(action);

    // figure out the max of all these times
    maxTime = std::max(resourceTime,    maxTime);
    maxTime = std::max(prereqTime,      maxTime);
    maxTime = std::max(supplyTime,      maxTime);
    maxTime = std::max(builderTime,     maxTime);

    // return the time
    return maxTime;
}

int GameState::whenCanCast(const ActionType & action, size_t targetID) const
{
    BOSS_ASSERT(action.isAbility(), "whenCanCast should only be called with an ability");

    int maxTime = m_currentFrame;
    int energyReady = whenEnergyReady(action);
    int buildingFinished = m_currentFrame + getUnit(targetID).getTimeUntilBuilt();
    int canChronoBoostAgain = m_currentFrame + getUnit(targetID).getChronoBoostAgainTime();

    maxTime = std::max(energyReady,         maxTime);
    maxTime = std::max(buildingFinished,    maxTime);
    maxTime = std::max(canChronoBoostAgain, maxTime);

    // the building will finish its production by the time we can chrono boost it
    if (maxTime >= m_currentFrame + getUnit(targetID).getTimeUntilFree() && m_race == Races::Protoss)
    {
        return -1;
    }

    return maxTime;
}

// returns the game frame that we will have the resources available to construction given action type
// this function assumes the action is legal (must be checked beforehand)
int GameState::whenResourcesReady(const ActionType & action) const
{
    if (m_minerals >= action.mineralPrice() && m_gas >= action.gasPrice())
    {
        return getCurrentFrame();
    }

    int previousFrame           = m_currentFrame;
    int currentMineralWorkers   = m_mineralWorkers;
    int currentGasWorkers       = m_gasWorkers;
    int lastActionFinishFrame   = m_currentFrame;
    int addedTime               = 0;
    double addedMinerals        = 0;
    double addedGas             = 0;
    double mineralDifference    = action.mineralPrice() - m_minerals;
    double gasDifference        = action.gasPrice() - m_gas;

    // loop through each action in progress, adding the minerals we would gather from each interval
    for (size_t i(0); i < m_unitsBeingBuilt.size(); ++i)
    {
        const Unit & unit = getUnit(m_unitsBeingBuilt[m_unitsBeingBuilt.size() - 1 - i]);
        int actionCompletionTime = previousFrame + unit.getTimeUntilBuilt();

        // the time elapsed and the current minerals per frame
        int elapsed = actionCompletionTime - lastActionFinishFrame;

        // the amount of minerals that would be added this time step
        double tempAddMinerals = elapsed * currentMineralWorkers * CONSTANTS::MPWPF;
        double tempAddGas      = elapsed * currentGasWorkers * CONSTANTS::GPWPF;

        // if this amount isn't enough, update the amount added for this interval
        if (addedMinerals + tempAddMinerals < mineralDifference || addedGas + tempAddGas < gasDifference)
        {
            addedMinerals += tempAddMinerals;
            addedGas += tempAddGas;
            addedTime += elapsed;
        }
        else { break; }

        // finishing a building as terran gives you a mineral worker back
        if (unit.getType().isBuilding() && !unit.getType().isAddon() && (unit.getType().getRace() == Races::Terran))
        {
            currentMineralWorkers++;
        }
        // finishing a worker gives us another mineral worker
        else if (unit.getType().isWorker())
        {
            currentMineralWorkers++;
        }
        // finishing a refinery adjusts the worker count
        else if (unit.getType().isRefinery())
        {
            BOSS_ASSERT(currentMineralWorkers > CONSTANTS::WorkersPerRefinery, "Not enough mineral workers \n");
            currentMineralWorkers -= CONSTANTS::WorkersPerRefinery;
            currentGasWorkers += CONSTANTS::WorkersPerRefinery;
        }

        // update the last action
        lastActionFinishFrame = actionCompletionTime;
    }

    // if we still haven't added enough minerals, add more time
    if (addedMinerals < mineralDifference || addedGas < gasDifference)
    {
        BOSS_ASSERT(currentMineralWorkers > 0, "Shouldn't have 0 mineral workers");

        int mineralTimeNeeded = (int)std::ceil((mineralDifference - addedMinerals) / (currentMineralWorkers * CONSTANTS::MPWPF));
        int gasTimeNeeded     = (int)std::ceil((gasDifference - addedGas) / (currentGasWorkers * CONSTANTS::GPWPF));
        addedTime             += std::max(mineralTimeNeeded, gasTimeNeeded);
    }
    
    return addedTime + m_currentFrame;
}

int GameState::whenBuilderReady(const ActionType & action) const
{
    int builderID = getBuilderID(action);

    // Probably unnecessary, given that we check for this condition inside of getBuilderID()
    BOSS_ASSERT(builderID != -1, "Didn't find when builder ready for %s", action.getName().c_str());

    if (action.isAbility())
    {
        return m_currentFrame + getUnit(builderID).whenCanBuild(action);
    }

    return m_currentFrame + getUnit(builderID).getTimeUntilFree();
}

int GameState::whenSupplyReady(const ActionType & action) const
{
    int supplyNeeded = action.supplyCost() + m_currentSupply - m_maxSupply;
    if (supplyNeeded <= 0) { return m_currentFrame; }

    // search the actions in progress in reverse for the first supply provider
    for (size_t i(0); i < m_unitsBeingBuilt.size(); ++i)
    {
        const Unit & unit = getUnit(m_unitsBeingBuilt[m_unitsBeingBuilt.size() - 1 - i]);   
        if (unit.getType().supplyProvided() > supplyNeeded)
        {
            return m_currentFrame + unit.getTimeUntilBuilt();
        }
    }

    BOSS_ASSERT(false, "Didn't find any supply in progress to build %s", action.getName().c_str());
    return m_currentFrame;
}

int GameState::whenPrerequisitesReady(const ActionType & action) const
{
    // if this action requires no prerequisites, then they are ready right now
    if (action.required().empty())
    {
        return m_currentFrame;
    }

    // if it has prerequisites, we need to find the max-min time that any of the prereqs are free
    int whenPrereqReady = 0;
    for (auto & req : action.required())
    {
        // find the minimum time that this particular prereq will be ready
        int minReady = std::numeric_limits<int>::max();
        for (auto & unit : m_units)
        {
            if (unit.getType() != req) { continue; }
            minReady = std::min(minReady, unit.getTimeUntilFree());
            if (unit.getTimeUntilFree() == 0) { break; }
        }
        // we can only build the type after the LAST of the prereqs are ready
        whenPrereqReady = std::max(whenPrereqReady, minReady);
        BOSS_ASSERT(whenPrereqReady != std::numeric_limits<int>::max(), "Did not find a prerequisite required to build %s", action.getName().c_str());
    }
      
    return m_currentFrame + whenPrereqReady;
}

int GameState::whenEnergyReady(const ActionType & action) const
{
    int minWhenReady = std::numeric_limits<int>::max();

    for (auto & unit : m_units)
    {
        int whenReady = unit.whenCanBuild(action);

        // shortcut return if we found something that can build now
        if (whenReady == 0)
        {
            return m_currentFrame;
        }

        // if the Unit can build the unit, set the new min
        if (whenReady != -1 && whenReady < minWhenReady)
        {
            minWhenReady = whenReady;
        }
    }

    return m_currentFrame + minWhenReady;
}

int GameState::getBuilderID(const ActionType & action) const
{
    int minWhenReady = std::numeric_limits<int>::max();
    int builderID = -1;

    // look over all our units and get when the next builder type is free
    for (auto & unit : m_units)
    {
        int whenReady = unit.whenCanBuild(action);
        
        // shortcut return if we found something that can build now
        if (whenReady == 0)
        {
            return unit.getID();
        }

        // if the Unit can build the unit, set the new min
        if (whenReady != -1 && whenReady < minWhenReady)
        {
            minWhenReady = whenReady;
            builderID = unit.getID();
        }
    }

    BOSS_ASSERT(builderID != -1, "Didn't find a builder for %s", action.getName().c_str());

    return builderID;
}

bool GameState::haveBuilder(const ActionType & type) const
{
    return std::any_of(m_units.begin(), m_units.end(), 
           [&type](const Unit & u){ return u.whenCanBuild(type) != -1; });
}

bool GameState::havePrerequisites(const ActionType & type) const
{
    return std::all_of(type.required().begin(), type.required().end(), 
           [this](const ActionType & req) { return this->haveType(req); });
}

size_t GameState::getNumInProgress(const ActionType & action) const
{
    return std::count_if(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(),
           [this, &action](const size_t & id) { return action == this->getUnit(id).getType(); } );
}

size_t GameState::getNumCompleted(const ActionType & action) const
{
    return std::count_if(m_units.begin(), m_units.end(),
           [&action](const Unit & unit) { return action == unit.getType() && unit.getTimeUntilBuilt() == 0; } );
}

size_t GameState::getNumTotal(const ActionType & action) const
{
    return std::count_if(m_units.begin(), m_units.end(), 
           [&action](const Unit & unit) { return action == unit.getType(); } );
}

size_t GameState::getNumTotalCompleted(const ActionType & action) const
{
    return std::count_if(m_units.begin(), m_units.end(),
        [&action](const Unit & unit) { return (action == unit.getType() && unit.getTimeUntilBuilt() == 0); });
}

bool GameState::haveType(const ActionType & action) const
{
    return std::any_of(m_units.begin(), m_units.end(), 
           [&action](const Unit & i){ return i.getType() == action; });
}

int GameState::getSupplyInProgress() const
{
    return std::accumulate(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(), 0, 
           [this](size_t lhs, size_t rhs) { return lhs + this->getUnit(rhs).getType().supplyProvided(); });
}

void GameState::getSpecialAbilityTargets(ActionSet & actionSet) const
{
    if (m_race == Races::GetRaceID("Protoss"))
    {
        actionSet.remove(ActionTypes::GetSpecialAction(m_race));
        storeChronoBoostTargets(actionSet);
    }
}


// checks whether any Nexus has 50 energy to cast Chrono Boost
bool GameState::canChronoBoost() const
{
    if (m_race != Races::Protoss)
        return false;

    return std::any_of(m_units.begin(), m_units.end(),
        [this](const Unit & u) { return (u.getType() == ActionTypes::GetResourceDepot(this->getRace()) && 
                                                u.getTimeUntilBuilt() == 0 && u.getEnergy() >= 50.0); });
}

void GameState::storeChronoBoostTargets(ActionSet & actionSet) const
{
    for (const Unit & unit : m_units)
    {
        if (chronoBoostableTarget(unit))
        {
            //std::cout << "Chronoboost target: " << unit.getType().getName() << std::endl;
            actionSet.add(ActionTypes::GetSpecialAction(m_race), unit.getID());
        }
    }
}

bool GameState::chronoBoostableTarget(const Unit & unit) const
{
    // can only be used on buildings
    if (!unit.getType().isBuilding()) { return false; }

    // can't chrono boost refinery
    if (unit.getType().isRefinery()) { return false; }

    // can't chrono boost pylon
    if (unit.getType().isSupplyProvider()) { return false; }

    // the unit must be producing something
    if (unit.getTimeUntilFree() == 0) { return false; }

    return true;
}

bool GameState::canChronoBoostTarget(const Unit & unit) const
{
    // can only be used on buildings
    if (!unit.getType().isBuilding()) { return false; }

    // the unit must be fully built
    if (unit.getTimeUntilBuilt() > 0) { return false; }

    // the unit must be producing something
    if (unit.getTimeUntilFree() == 0) { return false; }

    // can't chronoboost a building that is already chronoboosted
    if (unit.getChronoBoostAgainTime() > 0) { return false; }

    return true;
}

const std::vector<size_t> & GameState::getFinishedArmyUnits() const
{
    return m_armyUnits;
}

const std::vector<size_t> & GameState::getFinishedUnits() const
{
    return m_unitsSortedEndFrame;
}

const Unit & GameState::getUnit(size_t id) const
{
    return m_units[id];
}

ActionType GameState::getUnitType(size_t id)
{
    return m_units[id].getType();
}

Unit & GameState::getUnit(size_t id)
{
    return m_units[id];
}

const double & GameState::getMinerals() const
{
    return m_minerals;
}

const double & GameState::getGas() const
{
    return m_gas;
}

const int & GameState::getCurrentSupply() const
{
    return m_currentSupply;
}

const int & GameState::getMaxSupply() const
{
    return m_maxSupply;
}

const int & GameState::getCurrentFrame() const
{
    return m_currentFrame;
}

void GameState::setMinerals(double minerals)
{
    m_minerals = minerals;
}

void GameState::setGas(double gas)
{
    m_gas = gas;
}

int GameState::getRace() const
{
    return m_race;
}

bool GameState::canBuildNow(const ActionType & action) const
{
    return whenCanBuild(action) == getCurrentFrame();
}

size_t GameState::getNumMineralWorkers() const
{
    return m_mineralWorkers;
}

size_t GameState::getNumGasWorkers() const
{
    return m_gasWorkers;
}

size_t GameState::getNumberChronoBoostsCast() const
{
    return m_chronoBoosts.size();
}

const std::vector<AbilityAction> & GameState::getChronoBoostTargets() const
{
    return m_chronoBoosts;
}

const ActionType & GameState::getLastAction() const
{
    return m_lastAction;
}

const AbilityAction & GameState::getLastAbility() const
{
    return m_lastAbility;
}

int GameState::getLastActionFinishTime() const
{
    return m_unitsBeingBuilt.empty() ? getCurrentFrame() : m_units[m_unitsBeingBuilt.front()].getTimeUntilBuilt();
}

size_t GameState::getNumUnits() const
{
    return m_units.size();
}

int GameState::getNextFinishTime(const ActionType & type) const
{
    auto it = std::find_if(m_unitsBeingBuilt.rbegin(), m_unitsBeingBuilt.rend(),
              [this, &type](const size_t & uid) { return this->getUnit(uid).getType() == type; });

    return it == m_unitsBeingBuilt.rend() ? getCurrentFrame() : m_units[*it].getTimeUntilFree();
}

std::string GameState::toString() const
{
    std::stringstream ss;
    char buf[1024];
    ss << std::setfill('0') << std::setw(7);
    ss << "\n--------------------------------------\n";
    
    ss << "Current  Frame: " << m_currentFrame  << " (" << (m_currentFrame  / (60 * 24)) << "m " << ((m_currentFrame  / 24) % 60) << "s)\n";
    ss << "Previous Frame: " << m_previousFrame << " (" << (m_previousFrame / (60 * 24)) << "m " << ((m_previousFrame / 24) % 60) << "s)\n\n";

    ss << "Units Completed:\n";
    const std::vector<ActionType> & allActions = ActionTypes::GetAllActionTypes();
    for (auto & type : ActionTypes::GetAllActionTypes())
    {
        size_t numCompleted = getNumCompleted(type);
        if (numCompleted > 0) 
        {
            ss << "\t" << numCompleted << "\t" << type.getName() << "\n";
        }
    }

    ss << "\nUnits In Progress:\n";
    for (auto & id : m_unitsBeingBuilt) 
    {
        auto & unit = getUnit(id);
        sprintf(buf, "%5d %5d %s\n", unit.getID(), unit.getTimeUntilBuilt(), unit.getType().getName().c_str());
        ss << buf;
    }

    ss << "\nAll Units:\n";
    for (auto & unit : m_units) 
    {
        sprintf(buf, "%5d %5d %s\n", unit.getID(), unit.getTimeUntilFree(), unit.getType().getName().c_str());
        ss << buf;
    }
    
    ss << "\nLegal Actions:\n";
    std::vector<ActionType> legalActions;
    getLegalActions(legalActions);
    ss << "--------------------------------------\n";
    sprintf(buf, "%5s %5s %5s %5s\n", "total", "build", "res", "pre");
    ss << buf;
    ss << "--------------------------------------\n";
    for (auto & type : legalActions)
    {
        sprintf(buf, "%5d %5d %5d %5d %s\n", (whenCanBuild(type)-m_currentFrame), (whenBuilderReady(type)-m_currentFrame), (whenResourcesReady(type)-m_currentFrame), (whenPrerequisitesReady(type)-m_currentFrame), type.getName().c_str());
        ss << buf;
    }

    ss << "\nResources:\n";
    sprintf(buf, "%7d   Minerals\n%7d   Gas\n%7d   Mineral Workers\n%7d   Gas Workers\n%3d/%3d  Supply\n", (int)m_minerals, (int)m_gas, m_mineralWorkers, m_gasWorkers, m_currentSupply/2, m_maxSupply/2);
    ss << buf;

    ss << "--------------------------------------\n";
    ss << "Supply In Progress: " << getSupplyInProgress() << "\n";
    ss << "Next Probe Finish: " << getNextFinishTime(ActionTypes::GetActionType("Probe")) << "\n";
    //printPath();

    return ss.str();
}

void GameState::printunitsbeingbuilt() const
{
    for (auto & index : m_unitsBeingBuilt)
    {
        std::cout << getUnit(index).getTimeUntilBuilt() << std::endl;
    }
    std::cout << std::endl;
}