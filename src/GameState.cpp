/* -*- c-basic-offset: 4 -*- */

#include "GameState.h"
#include <numeric>
#include <iomanip>

using namespace BOSS;

GameState::GameState()
    : m_race(Races::None)
    , m_minerals(0.0f)
    , m_gas(0.0f)
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
  //!!! PROBLEM m_lastAbility undefined?
{
    //using Vector_Unit = BoundedVector<Unit, 70>;
    //using Vector_NumUnits = BoundedVector<NumUnits, 35>;
    //using Vector_AbilityAction = BoundedVector<AbilityAction, 10>;

    //Vector_Unit             m_units;
    //Vector_NumUnits         m_unitsBeingBuilt;
    //Vector_NumUnits         m_unitsSortedEndFrame; 
    //Vector_AbilityAction    m_chronoBoosts;
}

/*GameState::GameState(Vector_Unit & unitVector, RaceID race, FracType minerals, FracType gas,
    NumUnits currentSupply, NumUnits maxSupply, NumUnits mineralWorkers, NumUnits gasWorkers,
    NumUnits builerWorkers, TimeType currentFrame, NumUnits numRefineries, NumUnits numDepots)
    : m_units (unitVector)
    , m_race(race)
    , m_minerals(minerals)
    , m_gas(gas)
    , m_currentSupply(currentSupply)
    , m_maxSupply(maxSupply)
    , m_currentFrame(currentFrame)
    , m_previousFrame(0)
    , m_mineralWorkers(mineralWorkers)
    , m_gasWorkers(gasWorkers)
    , m_buildingWorkers(builerWorkers)
    , m_numRefineries(numRefineries)
    , m_numDepots(numDepots)
    , m_lastAction(ActionTypes::None)
{
    /*for (auto & unit : m_units)
    {
        std::cout << "name: " << unit.getType().getName() << std::endl;
    }
    std::cout << Races::GetRaceName(m_race) << std::endl;
    std::cout << m_minerals << std::endl;
    std::cout << m_gas << std::endl;
    std::cout << m_currentSupply << std::endl;
    std::cout << m_maxSupply << std::endl;
    std::cout << m_currentFrame << std::endl;
    std::cout << m_mineralWorkers << std::endl;
    std::cout << m_gasWorkers << std::endl;
    std::cout << m_buildingWorkers << std::endl;
    std::cout << m_numRefineries << std::endl;
    std::cout << m_numDepots << std::endl;*/
//}

void GameState::getLegalActions(std::vector<ActionType> & legalActions) const
{
    legalActions.clear();
    for (auto & type : ActionTypes::GetAllActionTypes())
    {
        if (isLegal(type)) { legalActions.push_back(type); }
    }
}

bool GameState::isLegal(ActionType action) const
{
    // if the race can't do the action
    if (action.getRace() != m_race) { return false; }    

    // if we have no gas income we can't make a gas unit
    if ((m_gas < action.gasPrice()) && (m_gasWorkers == 0)) { return false; }

    const int mineralWorkers = m_mineralWorkers + m_buildingWorkers;
    // if we have no mineral workers, we can't make any unit
    if (mineralWorkers == 0) { return false; }

    // if we have no mineral income we'll never have a minerla unit
    if ((m_minerals < action.mineralPrice()) && (mineralWorkers == 0)) { return false; }

    // TODO: require an extra for refineries but not buildings
    // rules for buildings which are built by workers
    if (action.isBuilding() && !action.isMorphed() && !action.isAddon() && (mineralWorkers == 0)) { return false; }

    const int refineriesInProgress = getNumInProgress(ActionTypes::GetRefinery(m_race));
    const int numRefineries = m_numRefineries + refineriesInProgress;
    // don't build a refinery if we don't have enough mineral workers to transfer over
    if (action.isRefinery() && (mineralWorkers <= (CONSTANTS::WorkersPerRefinery * (numRefineries + 1)))) { return false; }

    const int numDepots = m_numDepots + getNumInProgress(ActionTypes::GetResourceDepot(m_race));
    // don't build more refineries than resource depots
    if (action.isRefinery() && (numRefineries >= 2 * numDepots)) { return false; }

    // we can Chrono Boost as long as we have a Nexus
    if (action.isAbility() && m_race == Races::Protoss && numDepots == 0) { return false; }


    const NumUnits totalSupply = m_maxSupply + getSupplyInProgress();
    // if it's a unit and we are out of supply and aren't making a supply providing unit, it's not legal
    if (!action.isMorphed() && !action.isSupplyProvider() && ((m_currentSupply + action.supplyCost()) > totalSupply)) { return false; }    

    // we don't need to go over the maximum supply limit with supply providers
    if (action.isSupplyProvider() && (totalSupply > 400)) { return false; }

    // need to have at least 1 Pylon to build Protoss buildings, except if it's an Assimilator
    if (m_race == Races::Protoss && action.isBuilding() && !action.isSupplyProvider() && !action.isRefinery() 
                                && !haveType(ActionTypes::GetSupplyProvider(m_race))) { return false; }

    // Pylon is invalid if we have 16 or over free supply
    if (m_race == Races::Protoss && action == ActionTypes::GetSupplyProvider(m_race) && totalSupply - m_currentSupply >= 16) { return false; }

    // TODO: can only build one of a tech type
    // TODO: check to see if an addon can ever be built
    if (!haveBuilder(action)) { return false; }

    if (!havePrerequisites(action)) { return false; }

    return true;
}

void GameState::doAction(ActionType type)
{
    BOSS_ASSERT(!type.isAbility(), "doAction should not be called with an ability");

    //!!!PROBLEM UNUSED short previousFrame = m_currentFrame;
    m_lastAction = type;

    // figure out when this action can be done and fast forward to it
    const TimeType timeWhenReady = whenCanBuild(type);
    fastForward(timeWhenReady);

    // the builder of action
    NumUnits buildID = getBuilderID(type);

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

void GameState::doAbility(ActionType type, NumUnits targetID)
{
    BOSS_ASSERT(type.isAbility(), "doAbility should not be called with a non-ability action");
    BOSS_ASSERT(targetID != -1, "Target of ability %s is invalid. Target ID: %u", type.getName().c_str(), targetID);

    //!!!PROBLEM UNUSED short previousFrame = m_currentFrame;
    m_lastAction = type;

    // figure out when this action can be done and fast forward to it
    const TimeType timeWhenReady = whenCanCast(type, targetID);

    BOSS_ASSERT(timeWhenReady != -1, "Unable to cast ability");

    fastForward(timeWhenReady);

    if (m_race == Races::Protoss)
    {
        AbilityAction abilityAction(type, m_currentFrame, targetID, getUnit(targetID).getBuildID(), getUnit(targetID).getType());
        m_lastAbility = abilityAction;

        // cast chronoboost
        getUnit(getBuilderID(type)).castAbility(type, getUnit(targetID), getUnit(getUnit(targetID).getBuildID()));
        
        m_chronoBoosts.push_back(abilityAction);

        // have to resort the list, because build time of a unit is changed
        // find the index of the unit whos time is changed
        int index = 0;
        int buildID = getUnit(targetID).getBuildID();
        for (; index < m_unitsBeingBuilt.size(); ++index)
        {
            if (m_unitsBeingBuilt[index] == buildID)
            {
                break;
            }
        }

        // resort
        for (int i = index; i < (int)m_unitsBeingBuilt.size() - 1; ++i)
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
}

void GameState::fastForward(TimeType toFrame)
{
    if (toFrame == m_currentFrame) { return; }

    BOSS_ASSERT(toFrame > m_currentFrame, "Must ff to the future");

    m_previousFrame                 = m_currentFrame;
    TimeType previousFrame          = m_currentFrame;
    TimeType lastActionFinishTime   = m_currentFrame;
    
    // iterate backward over actions in progress since they're sorted
    // that way for ease of deleting the finished ones
    while (!m_unitsBeingBuilt.empty())
    {
        Unit & unit = getUnit(m_unitsBeingBuilt.back());
        ActionType type = unit.getType();

        // if the current action in progress will finish after the ff time, we can stop
        const TimeType actionCompletionTime = previousFrame + unit.getTimeUntilBuilt();
        if (actionCompletionTime > toFrame) { break; }

        // add the resources we gathered during this time period
        const TimeType timeElapsed    = actionCompletionTime - lastActionFinishTime;
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
    TimeType timeElapsed = toFrame - lastActionFinishTime;
    m_minerals      += timeElapsed * CONSTANTS::MPWPF * m_mineralWorkers;
    m_gas           += timeElapsed * CONSTANTS::GPWPF * m_gasWorkers;

    // update all the intances to the ff time
    for (int i(0); i < int(m_units.size()); ++i )
    {
        m_units[i].fastForward(toFrame - previousFrame);
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
        BOSS_ASSERT(m_numRefineries <= 2 * (m_numDepots + getNumInProgress(ActionTypes::GetResourceDepot(m_race))), "Shouldn't have more refineries than 2*depots");
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
void GameState::addUnit(ActionType type, NumUnits builderID)
{
    BOSS_ASSERT(m_race == Races::None || type.getRace() == m_race, "Adding an Unit of a different race");

    m_race = type.getRace();
    Unit unit(type, NumUnits(m_units.size()), builderID, m_currentFrame);
    m_units.push_back(unit);
    m_currentSupply += unit.getType().supplyCost();
    
    // if we have a valid builder for this object, add it to the Units being built
    if (builderID != -1)
    {
        getUnit(builderID).startBuilding(m_units[m_units.size() - 1]);

        // add the Unit ID being built and sort the list
        m_unitsBeingBuilt.push_back(unit.getID());

        // we know the list is already sorted when we add this unit, so we just swap it from the end until it's in the right place
        for (int i = (int)m_unitsBeingBuilt.size() - 1; i > 0; i--)
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
        completeUnit(m_units[m_units.size() - 1]);
    }
}

void GameState::addUnitToSpecialVectors(NumUnits unitIndex)
{
    // we don't want to store units that we start with
    if (m_units[unitIndex].getBuilderID() == -1)
    {
        return;
    }

    // if it's a combat unit add it to the armyUnits vector
    /*ActionType type = m_units[unitIndex].getType();
    if (!type.isBuilding() && !type.isWorker() && !type.isSupplyProvider())
    {
        m_armyUnits.push_back(unitIndex);
    }*/
  
    // add to finished units vector
    m_unitsSortedEndFrame.push_back(unitIndex);
}

int GameState::whenCanBuild(ActionType action) const
{
    if (action.isAbility())
    {
        return whenEnergyReady(action);
    }

    // figure out when prerequisites will be ready
    TimeType maxTime         = m_currentFrame;
    TimeType prereqTime      = whenPrerequisitesReady(action);
    TimeType resourceTime    = whenResourcesReady(action);
    TimeType supplyTime      = whenSupplyReady(action);
    TimeType builderTime     = whenBuilderReady(action);

    // figure out the max of all these times
    maxTime = std::max(resourceTime,    maxTime);
    maxTime = std::max(prereqTime,      maxTime);
    maxTime = std::max(supplyTime,      maxTime);
    maxTime = std::max(builderTime,     maxTime);

    // return the time
    return maxTime;
}

int GameState::whenCanCast(ActionType action, NumUnits targetID) const
{
    BOSS_ASSERT(action.isAbility(), "whenCanCast should only be called with an ability");
    BOSS_ASSERT(getUnit(targetID).getTimeUntilBuilt() == 0, "Casting on a unit that is not built yet");

    TimeType maxTime                = m_currentFrame;
    TimeType energyReady            = whenEnergyReady(action);
    //TimeType buildingFinished       = m_currentFrame + getUnit(targetID).getTimeUntilBuilt();
    TimeType canChronoBoostAgain    = m_currentFrame + getUnit(targetID).getChronoBoostAgainTime();

    maxTime = std::max(energyReady,         maxTime);
    //maxTime = std::max(buildingFinished,    maxTime);
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
int GameState::whenResourcesReady(ActionType action) const
{
    if (m_minerals >= action.mineralPrice() && m_gas >= action.gasPrice())
    {
        return getCurrentFrame();
    }

    TimeType previousFrame          = m_currentFrame;
    NumUnits currentMineralWorkers  = m_mineralWorkers;
    NumUnits currentGasWorkers      = m_gasWorkers;
    TimeType lastActionFinishFrame  = m_currentFrame;
    TimeType addedTime              = 0;
    float addedMinerals             = 0;
    float addedGas                  = 0;
    float mineralDifference         = action.mineralPrice() - m_minerals;
    float gasDifference             = action.gasPrice() - m_gas;

    // loop through each action in progress, adding the minerals we would gather from each interval
    for (int i = (int)(m_unitsBeingBuilt.size() - 1); i >= 0 ; --i)
    {
        const Unit & unit = getUnit(m_unitsBeingBuilt[i]);
        TimeType actionCompletionTime = previousFrame + unit.getTimeUntilBuilt();

        // the time elapsed and the current minerals per frame
        TimeType elapsed = actionCompletionTime - lastActionFinishFrame;

        // the amount of minerals that would be added this time step
        float tempAddMinerals = elapsed * currentMineralWorkers * CONSTANTS::MPWPF;
        float tempAddGas      = elapsed * currentGasWorkers * CONSTANTS::GPWPF;

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

        TimeType mineralTimeNeeded = (TimeType)std::ceil((mineralDifference - addedMinerals) / (currentMineralWorkers * CONSTANTS::MPWPF));
        TimeType gasTimeNeeded     = (TimeType)std::ceil((gasDifference - addedGas) / (currentGasWorkers * CONSTANTS::GPWPF));
        addedTime               += std::max(mineralTimeNeeded, gasTimeNeeded);
    }
    
    return addedTime + m_currentFrame;
}

int GameState::whenBuilderReady(ActionType action) const
{
    NumUnits builderID = getBuilderID(action);

    // Probably unnecessary, given that we check for this condition inside of getBuilderID()
    BOSS_ASSERT(builderID != -1, "Didn't find when builder ready for %s", action.getName().c_str());

    if (action.isAbility())
    {
        return m_currentFrame + getUnit(builderID).whenCanBuild(action);
    }

    return m_currentFrame + getUnit(builderID).getTimeUntilFree();
}

int GameState::whenSupplyReady(ActionType action) const
{
    int supplyNeeded = action.supplyCost() + m_currentSupply - m_maxSupply;
    if (supplyNeeded <= 0) { return m_currentFrame; }

    // search the actions in progress in reverse for the first supply provider
    for (int i(0); i < m_unitsBeingBuilt.size(); ++i)
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

int GameState::whenPrerequisitesReady(ActionType action) const
{
    // if this action requires no prerequisites, then they are ready right now
    if (action.required().empty())
    {
        return m_currentFrame;
    }

    int whenPrereqReady = 0;

    // Protoss needs to have a Pylon to be able to produce buildings
    if (m_race == Races::Protoss)
    {
        if (action.isBuilding() && !action.isRefinery() && !action.isSupplyProvider())
        {
            whenPrereqReady = timeUntilFirstPylonDone();

            if (whenPrereqReady == std::numeric_limits<int>::max())
            {
                return whenPrereqReady;
            }
        }
    }

    // if it has prerequisites, we need to find the max-min time that any of the prereqs are free
    for (auto & req : action.required())
    {
        // find the minimum time that this particular prereq will be ready
        int minReady = std::numeric_limits<int>::max();
        for (int i(0); i < int(m_units.size()); ++i)
        {
            auto & unit = m_units[i];
            if (unit.getType() != req) { continue; }
            minReady = std::min(minReady, unit.getTimeUntilFree());
            if (unit.getTimeUntilFree() == 0) { break; }
        }
        // we can only build the type after the LAST of the prereqs are ready
        whenPrereqReady = std::max(whenPrereqReady, minReady);
        BOSS_ASSERT(whenPrereqReady != std::numeric_limits<TimeType>::max(), "Did not find a prerequisite required to build %s", action.getName().c_str());
    }
      
    return m_currentFrame + whenPrereqReady;
}

int GameState::timeUntilFirstPylonDone() const
{
    for (auto & unit : m_units)
    {
        if (unit.getType() == ActionTypes::GetSupplyProvider(Races::Protoss))
        {
            return unit.getTimeUntilBuilt();
        }
    }

    return std::numeric_limits<TimeType>::max();
}

int GameState::whenEnergyReady(ActionType action) const
{
    TimeType minWhenReady = std::numeric_limits<TimeType>::max();

    // look over all our units and get when the next builder type is free
    for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];
        TimeType whenReady = unit.whenCanBuild(action);

        // shortcut return if we found something that can cast now
        if (whenReady == 0)
        {
            return m_currentFrame;
        }

        // if the Unit can cast the unit, set the new min
        if (whenReady != -1 && whenReady < minWhenReady)
        {
            minWhenReady = whenReady;
        }
    }

    return m_currentFrame + minWhenReady;
}

int GameState::getBuilderID(ActionType action) const
{
    TimeType minWhenReady = std::numeric_limits<TimeType>::max();
    int builderID = -1;

    // look over all our units and get when the next builder type is free
    for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];
        TimeType whenReady = unit.whenCanBuild(action);
        
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

bool GameState::haveBuilder(ActionType type) const
{
    return std::any_of(m_units.begin(), m_units.end(), 
           [&type](const Unit & u){ return u.whenCanBuild(type) != -1; });

    /*for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];
        if (unit.whenCanBuild(type) != -1)
        {
            return true;
        }
    }
    return false;*/
}

bool GameState::havePrerequisites(ActionType type) const
{
    return std::all_of(type.required().begin(), type.required().end(), 
           [this](ActionType req) { return this->haveType(req); });
}

int GameState::getNumInProgress(ActionType action) const
{
    return (int)std::count_if(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(),
           [this, &action](int id) { return action == this->getUnit(id).getType(); } );

   /* int numInProgress = 0;
    for (int i(0); i < m_unitsBeingBuilt.size(); ++i)
    {
        if (getUnit(m_unitsBeingBuilt[i]).getType() == action)
        {
            numInProgress++;
        }
    }
    return numInProgress;*/
}

int GameState::getNumCompleted(ActionType action) const
{
    return (int)std::count_if(m_units.begin(), m_units.end(),
           [&action](const Unit & unit) { return action == unit.getType() && unit.getTimeUntilBuilt() == 0; } );

    /*int numCompleted = 0;
    for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];
        if (unit.getType() == action && unit.getTimeUntilBuilt() == 0)
        {
            numCompleted++;
        }
    }
    return numCompleted;*/
}

int GameState::getNumTotal(ActionType action) const
{
    if (action.isAbility())
    { 
        if (m_race == Races::Protoss)
        {
            return int(m_chronoBoosts.size());
        }
    }

    return (int)std::count_if(m_units.begin(), m_units.end(), 
           [&action](const Unit & unit) { return action == unit.getType(); } );

    /*int numTotal = 0;
    for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];
        if (unit.getType() == action)
        {
            numTotal++;
        }
    }
    return numTotal;*/
}

bool GameState::haveType(ActionType action) const
{
    return std::any_of(m_units.begin(), m_units.end(), 
           [&action](const Unit & i){ return i.getType() == action; });

    /*for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];
        if (unit.getType() == action)
        {
            return true;
        }
    }
    return false;*/
}

int GameState::getSupplyInProgress() const
{
    return std::accumulate(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(), 0, 
           [this](int lhs, int rhs) { return lhs + this->getUnit(rhs).getType().supplyProvided(); });

    /*int supplyInProgress = 0;
    for (int i(0); i < m_unitsBeingBuilt.size(); ++i)
    {
        supplyInProgress += getUnit(m_unitsBeingBuilt[i]).getType().supplyProvided();
    }
    return supplyInProgress;*/
}

void GameState::getSpecialAbilityTargets(ActionSetAbilities & actionSet, int index) const
{
    if (m_race == Races::Protoss)
    {
        // if there are any chronoboostable targets, we remove the placeholder CB
        // action with target -1. Otherwise, we keep it in
        if (storeChronoBoostTargets(actionSet, index) != 0)
        {
            actionSet.remove(ActionTypes::GetSpecialAction(m_race), index);
        }
    }
}

// checks whether any Nexus has 50 energy to cast Chrono Boost
bool GameState::canChronoBoost() const
{
    if (m_race != Races::Protoss)
        return false;

    return std::any_of(m_units.begin(), m_units.end(),
        [this](const Unit & u) { return (u.getType() == ActionTypes::GetResourceDepot(this->getRace()) && 
                                                u.getTimeUntilBuilt() == 0 && u.getEnergy() >= ActionTypes::GetSpecialAction(m_race).energyCost()); });

    /*for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];
        if (unit.getType() == ActionTypes::GetResourceDepot(m_race) && unit.getTimeUntilBuilt() == 0 &&
            unit.getEnergy() >= float(ActionTypes::GetSpecialAction(m_race).energyCost()))
        {
            return true;
        }
    }
    return false;*/
}

int GameState::storeChronoBoostTargets(ActionSetAbilities & actionSet, int index) const
{
    int numTargets = 0;
    for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];

        if (chronoBoostableTarget(unit))
        {
            //std::cout << "Chronoboost target: " << unit.getType().getName() << std::endl;
            actionSet.add(ActionTypes::GetSpecialAction(m_race), unit.getID(), index);
            numTargets++;
        }
    }

    return numTargets;
}

// minimum criteria that a unit must meet in order to be Chronoboostable
bool GameState::chronoBoostableTarget(const Unit & unit) const
{
    // can only be used on buildings
    if (!unit.getType().isBuilding()) { return false; }

    // can't chrono boost refinery
    if (unit.getType().isRefinery()) { return false; }

    // can't chrono boost pylon
    if (unit.getType().isSupplyProvider()) { return false; }

    // the building must be finished
    if (unit.getTimeUntilBuilt() > 0) { return false; }

    // the unit must be producing something
    if (whenCanCast(ActionTypes::GetSpecialAction(m_race), unit.getID()) == -1) { return false; }

    // only allow chronoboost to be used on buildings that are producing a combat unit
    if (unit.getBuildType().isWorker()) { return false; }

    return true;
}

int GameState::getNextFinishTime(ActionType type) const
{
    auto it = std::find_if(m_unitsBeingBuilt.rbegin(), m_unitsBeingBuilt.rend(),
              [this, &type](int uid) { return this->getUnit(uid).getType() == type; });

    return it == m_unitsBeingBuilt.rend() ? getCurrentFrame() : m_units[*it].getTimeUntilFree();

    /*bool typeFound = false;
    int i = m_units.size();
    for (; i >= 0; --i)
    {
        if (getUnit(i).getType() == type)
        {
            typeFound = true;
            break;
        }
    }

    return typeFound ? m_units[i].getTimeUntilFree() : getCurrentFrame();*/
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
  //!!! PROBLEM UNUSED const std::vector<ActionType> & allActions = ActionTypes::GetAllActionTypes();
  for (auto & type : ActionTypes::GetAllActionTypes())
  {
    int numCompleted = getNumCompleted(type);
    if (numCompleted > 0) 
    {
      ss << "\t" << numCompleted << "\t" << type.getName() << "\n";
    }
  }

  ss << "\nUnits In Progress:\n";
  for (int i(0); i < m_unitsBeingBuilt.size(); ++i)
  {
    auto id = m_unitsBeingBuilt[i];
    auto & unit = getUnit(id);
    sprintf(buf, "%5d %5d %s\n", unit.getID(), unit.getTimeUntilBuilt(), unit.getType().getName().c_str());
    ss << buf;
  }

    ss << "\nAll Units:\n";
    for (int i(0); i < int(m_units.size()); ++i)
    {
        auto & unit = m_units[i];

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
    for (int i(0); i < m_unitsBeingBuilt.size(); ++i)
    {
        auto index = m_unitsBeingBuilt[i];
        std::cout << getUnit(index).getTimeUntilBuilt() << std::endl;
    }
    std::cout << std::endl;
}
