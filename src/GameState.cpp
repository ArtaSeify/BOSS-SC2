/* -*- c-basic-offset: 4 -*- */

#include "GameState.h"
#include "CombatSearchParameters.h"
#include <numeric>
#include <iomanip>

using namespace BOSS;

GameState::GameState()
    : m_units()
    , m_unitsBeingBuilt()
    , m_unitsSortedEndFrame()
    , m_chronoBoosts()
    , m_unitTypes()
    , m_race(Races::None)
    , m_minerals(0.0f)
    , m_gas(0.0f)
    , m_currentSupply(0)
    , m_maxSupply(0)
    , m_inProgressSupply(0)
    , m_currentFrame(0)
    , m_previousFrame(0)
    , m_mineralWorkers(0)
    , m_gasWorkers(0)
    , m_buildingWorkers(0)
    , m_numRefineries(0)
    , m_inProgressRefineries(0)
    , m_numDepots(0)
    , m_inProgressDepots(0)
    , m_lastAction(ActionTypes::None)
    , m_lastAbility(AbilityAction())
{
    //using Vector_Unit = BoundedVector<Unit, 70>;
    //using Vector_NumUnits = BoundedVector<NumUnits, 35>;
    //using Vector_AbilityAction = BoundedVector<AbilityAction, 10>;

    //Vector_Unit             m_units;
    //Vector_NumUnits         m_unitsBeingBuilt;
    //Vector_NumUnits         m_unitsSortedEndFrame; 
    //Vector_AbilityAction    m_chronoBoosts;
}

GameState::GameState(const std::vector<Unit> & unitVector, RaceID race, FracType minerals, FracType gas,
    NumUnits currentSupply, NumUnits maxSupply, NumUnits mineralWorkers, NumUnits gasWorkers,
    NumUnits builerWorkers, TimeType currentFrame, NumUnits numRefineries, NumUnits numDepots)
    : m_units (unitVector)
    , m_unitsSortedEndFrame()
    , m_chronoBoosts()
    , m_unitTypes(ActionTypes::GetRaceActionCount(race), std::vector<NumUnits>())
    , m_race(race)
    , m_minerals(minerals)
    , m_gas(gas)
    , m_currentSupply(currentSupply)
    , m_maxSupply(0)
    , m_inProgressSupply(0)
    , m_currentFrame(currentFrame)
    , m_previousFrame(0)
    , m_mineralWorkers(mineralWorkers)
    , m_gasWorkers(gasWorkers)
    , m_buildingWorkers(builerWorkers)
    , m_numRefineries(0)
    , m_inProgressRefineries(0)
    , m_numDepots(0)
    , m_inProgressDepots(0)
    , m_lastAction(ActionTypes::None)
    , m_lastAbility(AbilityAction())
{
    NumUnits calculatedSupply = 0;
    for (auto & unit : m_units)
    {
        calculatedSupply += unit.getType().supplyCost();
        if (unit.getTimeUntilBuilt() > 0)
        {
            m_inProgressSupply += unit.getType().supplyProvided();
            m_unitsBeingBuilt.push_back(unit.getID());

            if (unit.getType().isRefinery())
            {
                m_inProgressRefineries++;
            }

            else if (unit.getType().isDepot())
            {
                m_inProgressDepots++;
            }
        }
        else
        {
            m_maxSupply += unit.getType().supplyProvided();

            if (unit.getType().isRefinery())
            {
                m_numRefineries++;
            }

            else if (unit.getType().isDepot())
            {
                m_numDepots++;
            }
        }

        m_unitTypes[unit.getType().getRaceActionID()].push_back(unit.getID());
        //std::cout << "name: " << unit.getType().getName() << std::endl;
    }
    std::sort(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(),
        [this](int lhs, int rhs) { return m_units[lhs].getTimeUntilBuilt() > m_units[rhs].getTimeUntilBuilt(); });

    BOSS_ASSERT(m_mineralWorkers + m_gasWorkers + m_buildingWorkers == getNumCompleted(ActionTypes::GetWorker(m_race)), "Total number of workers doesn't add up. \
            mineral workers: %i, gas workers: %i, building workers: %i, total: %i", m_mineralWorkers, m_gasWorkers, m_buildingWorkers, getNumTotal(ActionTypes::GetWorker(m_race)));

    BOSS_ASSERT(calculatedSupply == currentSupply, "Actual current supply %i and the one calculated by BOSS %i are different", currentSupply, calculatedSupply);
    BOSS_ASSERT(m_currentSupply <= m_maxSupply, "Current supply %i must be less than or equal to max supply %i", m_currentSupply, m_maxSupply);

    std::cout << "Race: " << Races::GetRaceName(m_race) << std::endl;
    std::cout << "Minerals: " << m_minerals << std::endl;
    std::cout << "Gas: " << m_gas << std::endl;
    std::cout << "Current Supply: " << m_currentSupply << std::endl;
    std::cout << "In Progress Supply: " << m_inProgressSupply << std::endl;
    std::cout << "Max Supply: " << m_maxSupply << std::endl;
    std::cout << "Current Frame: " << m_currentFrame << std::endl;
    std::cout << "Mineral Workers: " << m_mineralWorkers << std::endl;
    std::cout << "Gas Workers: " << m_gasWorkers << std::endl;
    std::cout << "Building Workers: " << m_buildingWorkers << std::endl;
    std::cout << "Num Refineries: " << m_numRefineries << std::endl;
    std::cout << "In Progress Refineries: " << m_inProgressRefineries << std::endl;
    std::cout << "Num Depots: " << m_numDepots << std::endl;
    std::cout << "In Progress Depots: " << m_inProgressDepots << std::endl;
}

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

    // the number of workers allowed is determined by WorkersPerDepot + gas workers
    if (action.isWorker() && 
        getNumTotal(action) >= (m_numDepots + m_inProgressDepots) * (CONSTANTS::WorkersPerDepot + (2 * CONSTANTS::WorkersPerRefinery))) { return false; }

    // can only have one of each upgrade
    if (action.isUpgrade() && m_unitTypes[action.getRaceActionID()].size() > 0) { return false; }

    // only one Mothership is allowed
    if (action.supplyCost() == 8 && haveType(action)) { return false; }

    // TODO: require an extra for refineries but not buildings
    // rules for buildings which are built by workers
    if (action.isBuilding() && !action.isMorphed() && !action.isAddon() && (mineralWorkers == 0)) { return false; }

    const int numRefineries = m_numRefineries + m_inProgressRefineries;
    // don't build a refinery if we don't have enough mineral workers to transfer over
    if (action.isRefinery() && (mineralWorkers <= (CONSTANTS::WorkersPerRefinery * (numRefineries + 1)))) { return false; }

    const int numDepots = m_numDepots + m_inProgressDepots;
    // don't build more refineries than depots
    if (action.isRefinery() && (numRefineries == 2 * m_numDepots)) { return false; }

    // we can Chrono Boost as long as we have a Nexus
    if (action.isAbility() && m_race == Races::Protoss && numDepots == 0) { return false; }


    const NumUnits totalSupply = std::min(m_maxSupply + m_inProgressSupply, 200);
    // if it's a unit and we are out of supply and aren't making a supply providing unit, it's not legal
    if (!action.isMorphed() && !action.isSupplyProvider() && ((m_currentSupply + action.supplyCost()) > totalSupply)) { return false; }  

    // we don't need to go over the maximum supply limit with supply providers
    if (ActionTypes::GetSupplyProvider(m_race) == action && (totalSupply >= 200)) { return false; }

    // need to have at least 1 Pylon to build Protoss buildings, except if it's an Assimilator or Nexus
    if (m_race == Races::Protoss && action.isBuilding() && !action.isSupplyProvider() && !action.isRefinery() && !action.isDepot()
           && m_unitTypes[ActionTypes::GetSupplyProvider(m_race).getRaceActionID()].size() == 0) { return false; }

    // Don't build a supply depot if we have 16 or over free supply
    //if (m_race == Races::Protoss && action == ActionTypes::GetSupplyProvider(m_race) && totalSupply - m_currentSupply >= 16) { return false; }

    // TODO: can only build one of a tech type
    // TODO: check to see if an addon can ever be built
    if (!haveBuilder(action)) { return false; }

    // gateways automatically turn into warpgates when warpgate research is finished, so we can no longer
    // build gateway units
    if (action.whatBuilds() == ActionTypes::GetGatewayAction() && m_unitTypes[ActionTypes::GetWarpGateResearch().getRaceActionID()].size() > 0 &&
        timeUntilResearchDone(ActionTypes::GetWarpGateResearch()) + m_currentFrame <= whenCanBuild(action)) { return false; }

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

    // when we finish warpgate research, all gateways turn into warpgates on their own
    if (type == ActionTypes::GetWarpGateResearch())
    {
        ActionType gateway = ActionTypes::GetGatewayAction();
        for (int unitID : m_unitTypes[gateway.getRaceActionID()])
        {
            BOSS_ASSERT(getUnit(unitID).getType() == gateway, "assuming it's gateway but it's not");
            addUnit(ActionTypes::GetWarpgateAction(), unitID);
            //std::cout << "changing gateway to warpgate since upgrade finished!" << std::endl;
        }
    }

    // if we have WarpGateResearch then a Gateway is automatically turned into a WarpGate when it is finished.
    // So we need to add a WarpGate along with the Gateway
    else if (type == ActionTypes::GetGatewayAction() && timeUntilResearchDone(ActionTypes::GetWarpGateResearch()) == 0)
    {
        addUnit(ActionTypes::GetWarpgateAction(), m_units.back().getID());
        //std::cout << "wait!" << std::endl;
    }
}

void GameState::doAbility(ActionType type, NumUnits targetID)
{
    BOSS_ASSERT(type.isAbility(), "doAbility should not be called with a non-ability action");
    BOSS_ASSERT(targetID != -1, "Target of ability %s is invalid. Target ID: %u", type.getName().c_str(), targetID);

    //!!!PROBLEM UNUSED short previousFrame = m_currentFrame;
    m_lastAction = type;

    // figure out when this action can be done and fast forward to it
    const TimeType timeWhenReady = whenCanCast(type, targetID);

    if (timeWhenReady == -1)
    {
        TimeType maxTime = m_currentFrame;
        TimeType energyReady = whenEnergyReady(type);
        //TimeType buildingFinished       = m_currentFrame + getUnit(targetID).getTimeUntilBuilt();
        TimeType canChronoBoostAgain = m_currentFrame + getUnit(targetID).getChronoBoostAgainTime();

        maxTime = std::max(energyReady, maxTime);
        //maxTime = std::max(buildingFinished,    maxTime);
        maxTime = std::max(canChronoBoostAgain, maxTime);

        std::cout << "energyReady: " << energyReady << std::endl;
        std::cout << "canChronoBoostAgain: " << canChronoBoostAgain << std::endl;
        std::cout << "building will finish production before we can chronoboost: " << (maxTime >= m_currentFrame + getUnit(targetID).getTimeUntilFree() && m_race == Races::Protoss) << std::endl;
        std::cout << "target: " << getUnit(targetID).getType().getName() << std::endl;
        std::cout << "target time until free: " << getUnit(targetID).getTimeUntilFree() << std::endl;
    }
    BOSS_ASSERT(timeWhenReady != -1, "Unable to cast ability");

    fastForward(timeWhenReady);

    if (m_race == Races::Protoss)
    {
        AbilityAction abilityAction(type, m_currentFrame, targetID, getUnit(targetID).getBuildID(), getUnit(targetID).getType(), getUnit(targetID).getBuildType());
        m_lastAbility = abilityAction;

        // cast chronoboost 
        if (getUnit(targetID).getMorphID() == -1)
        {
            getUnit(getBuilderID(type)).castAbility(type, getUnit(targetID), getUnit(getUnit(targetID).getBuildID()), getUnit(targetID));
        }
        // this unit is warping into something else (Gateway into WarpGate), so we need to change the time of the WarpGate as well
        else
        {
            getUnit(getBuilderID(type)).castAbility(type, getUnit(targetID), getUnit(getUnit(targetID).getBuildID()), getUnit(getUnit(targetID).getMorphID()));
        }
        
        
        m_chronoBoosts.push_back(abilityAction);

        // have to resort the list, because build time of unit(s) is changed.
        std::sort(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(),
            [this](int lhs, int rhs) { return m_units[lhs].getTimeUntilBuilt() > m_units[rhs].getTimeUntilBuilt(); });

        //resortUnitsBeingBuilt(getUnit(targetID).getBuildID(), getUnit(targetID).getMorphID());
    }
}

void GameState::doAbility(ActionType type, NumUnits targetID, TimeType frame)
{
    BOSS_ASSERT(type.isAbility(), "doAbility should not be called with a non-ability action");
    BOSS_ASSERT(targetID != -1, "Target of ability %s is invalid. Target ID: %u", type.getName().c_str(), targetID);

    //!!!PROBLEM UNUSED short previousFrame = m_currentFrame;
    m_lastAction = type;

    fastForward(frame);

    // figure out when this action can be done and fast forward to it
    const TimeType timeWhenReady = whenCanCast(type, targetID);

    BOSS_ASSERT(timeWhenReady == m_currentFrame, "the ability should be castable at the current frame");

    if (m_race == Races::Protoss)
    {
        AbilityAction abilityAction(type, m_currentFrame, targetID, getUnit(targetID).getBuildID(), getUnit(targetID).getType(), getUnit(targetID).getBuildType());
        m_lastAbility = abilityAction;

        // cast chronoboost 
        if (getUnit(targetID).getMorphID() == -1)
        {
            getUnit(getBuilderID(type)).castAbility(type, getUnit(targetID), getUnit(getUnit(targetID).getBuildID()), getUnit(targetID));
        }
        // this unit is warping into something else (Gateway into WarpGate), so we need to change the time of the WarpGate as well
        else
        {
            getUnit(getBuilderID(type)).castAbility(type, getUnit(targetID), getUnit(getUnit(targetID).getBuildID()), getUnit(getUnit(targetID).getMorphID()));
        }


        m_chronoBoosts.push_back(abilityAction);

        // have to resort the list, because build time of unit(s) is changed.
        std::sort(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(),
            [this](int lhs, int rhs) { return m_units[lhs].getTimeUntilBuilt() > m_units[rhs].getTimeUntilBuilt(); });

        //resortUnitsBeingBuilt(getUnit(targetID).getBuildID(), getUnit(targetID).getMorphID());
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
        m_minerals              += timeElapsed * CONSTANTS::MPWPF * std::min((int)m_mineralWorkers, CONSTANTS::WorkersPerDepot * m_numDepots);
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
    m_minerals      += timeElapsed * CONSTANTS::MPWPF * std::min((int)m_mineralWorkers, CONSTANTS::WorkersPerDepot * m_numDepots);
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
    m_maxSupply = std::min(m_maxSupply, (NumUnits)200);
    m_inProgressSupply -= unit.getType().supplyProvided();
       
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
        m_inProgressRefineries--;
        m_numRefineries++;
        //std::cout << "we have " << m_numRefineries << " refineries, and " << (int)(m_numDepots + getNumInProgress(ActionTypes::GetResourceDepot(m_race))) << " bases." << std::endl;
        BOSS_ASSERT(m_numRefineries <= 2 * m_numDepots, "Shouldn't have more refineries than 2*depots, have %i refineries, %i depots", m_numRefineries, m_numDepots);
        int needGasWorkers = std::max(0, (CONSTANTS::WorkersPerRefinery*m_numRefineries - m_gasWorkers));
        BOSS_ASSERT(needGasWorkers < m_mineralWorkers, "Shouldn't need more gas workers than we have mineral workers. "
                                                       "%d required gas workers, %d mineral workers", needGasWorkers, m_mineralWorkers);
        m_mineralWorkers -= needGasWorkers;
        m_gasWorkers += needGasWorkers;
        //std::cout << "mineral workers after refinery build: " << m_mineralWorkers << std::endl;
    }
    else if (unit.getType().isDepot())
    {
        m_inProgressDepots--;
        m_numDepots++;
    }
    // if we have WarpGate research, all gateways turn into warpgates on their own when they are built
    //else if (unit.getType() == ActionTypes::GetActionType("Gateway") && timeUntilResearchDone(ActionTypes::GetWarpGateResearch()) == 0)
    //{
    //    addUnit(ActionTypes::GetActionType("WarpGate"), unit.getID());
    //    //std::cout << "changing gateway to warpgate since it finished building!" << std::endl;
    //}
    
    // a building that morphs from another building is constructed
    else if (unit.getType().isMorphed())
    {
        if (unit.getBuilderID() == -1)
        {
            for (auto & builder_unit : m_units)
            {
                if (builder_unit.getMorphID() == unit.getID())
                {
                    unit.setBuilderID(builder_unit.getID());
                    builder_unit.setMorphed(true);
                }
            }
        }
        else
        {
            getUnit(unit.getBuilderID()).setMorphed(true);
        }
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
    m_inProgressSupply += unit.getType().supplyProvided();

    if (m_unitTypes.size() == 0)
    {
        m_unitTypes = std::vector<std::vector<NumUnits>>(ActionTypes::GetRaceActionCount(m_race), std::vector<NumUnits>());
    }

    m_unitTypes[unit.getType().getRaceActionID()].push_back(unit.getID());

    if (type.isMorphed() && builderID != -1)
    {
        auto& subVector = m_unitTypes[getUnit(builderID).getType().getRaceActionID()];
        for (auto it = subVector.begin(); it != subVector.end(); ++it)
        {
            if (*it == builderID)
            {
                subVector.erase(it);
                break;
            }
        }
    }
    
    else
    {
        if (type.isDepot())
        {
            m_inProgressDepots++;
        }
        else if (type.isRefinery())
        {
            m_inProgressRefineries++;
        }
    }
    
    // if we have a valid builder for this object, add it to the Units being built
    if (builderID != -1)
    {
        // since WarpGates are built automatically, we need to adjust their built time to match our fast forwarding scheme
        if (type == ActionTypes::GetWarpgateAction())
        {
            //m_units.back().setTimeUntilBuilt(ActionTypes::GetActionType("WarpGate").buildTime() + 
            //                        std::max(m_currentFrame - m_previousFrame, getUnit(getUnit(builderID).getBuildID()).getTimeUntilBuilt()));
            m_units.back().setTimeUntilBuilt(ActionTypes::GetWarpgateAction().buildTime() +
                                        std::max(timeUntilResearchDone(ActionTypes::GetWarpGateResearch()),
                                            std::max(getUnit(builderID).getTimeUntilBuilt(), getUnit(builderID).getTimeUntilFree())));
            m_units.back().setTimeUntilFree(m_units.back().getTimeUntilBuilt());
        }

        int morphID = getUnit(builderID).getMorphID();

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

        // if the gateway is morphing into a warpgate, but the research isn't done yet,
        // we need to adjust the buildtime of the warpgate
        if (morphID != -1)
        {
            Unit & morphingUnit = getUnit(morphID);
            morphingUnit.setTimeUntilBuilt(std::max(getUnit(builderID).getTimeUntilFree() + ActionTypes::GetWarpgateAction().buildTime(), morphingUnit.getTimeUntilBuilt()));
            morphingUnit.setTimeUntilFree(morphingUnit.getTimeUntilBuilt());

            // need to resort
            for (int i = int(std::find(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(), morphID) - m_unitsBeingBuilt.begin()); i > 0; i--)
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

            /*for (int i : m_unitsBeingBuilt)
            {
               std::cout << m_units[i].getTimeUntilBuilt() << std::endl;
            }
            std::cout << std::endl;*/
        }
    }
    // if there's no builder, complete the unit now and skip the unit in progress step
    else
    {
        completeUnit(m_units[m_units.size() - 1]);
    }
}

std::vector<std::pair<int, int>> GameState::getAbilityTargetUnit(const std::pair<ActionType, AbilityAction> & action) const
{
    std::vector<std::pair<int, int>> targetIDs = std::vector<std::pair<int, int>>();
    ActionType type = action.first;
    ActionType targetType = action.second.targetType;
    ActionType targetProductionType = action.second.targetProductionType;
    for (int index = 0; index < getNumUnits(); ++index)
    {
        const auto unit = getUnit(index);
        if (unit.getType() == targetType && unit.getBuildType() == targetProductionType && whenCanCast(action.first, unit.getID()) != -1)
        {
            targetIDs.push_back(std::make_pair(unit.getID(), unit.getBuildID()));
            std::cout << "target for chronoboost: " << unit.getType().getName() << std::endl;
        }
    }

    return targetIDs;

    //std::cout << "target type: " << targetType.getName() << std::endl;
    //std::cout << "target production type: " << targetProductionType.getName() << std::endl;
    //BOSS_ASSERT(false, "Could not find the target for %s", action.first.getName().c_str());
}

int GameState::whenCanBuild(ActionType action, NumUnits targetID) const
{
    if (action.isAbility())
    {
        return whenCanCast(action, targetID);
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
    BOSS_ASSERT(getUnit(targetID).getTimeUntilBuilt() == 0, "Casting on %s which is not built yet", getUnit(targetID).getType().getName());

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
    NumUnits currentMineralWorkers  = std::min((int)m_mineralWorkers, CONSTANTS::WorkersPerDepot * m_numDepots);
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
        addedTime                  += std::max(mineralTimeNeeded, gasTimeNeeded);
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
    BOSS_ASSERT(m_currentSupply <= m_maxSupply, "current supply %i can't be higher than max supply %i", m_currentSupply, m_maxSupply);
    int supplyNeeded = action.supplyCost() + m_currentSupply - m_maxSupply;
    if (supplyNeeded <= 0) { return m_currentFrame; }

    // search the actions in progress in reverse for the first supply provider
    for (int i(0); i < m_unitsBeingBuilt.size(); ++i)
    {
        const Unit & unit = getUnit(m_unitsBeingBuilt[m_unitsBeingBuilt.size() - 1 - i]);   
        if (unit.getType().supplyProvided() >= supplyNeeded)
        {
            return m_currentFrame + unit.getTimeUntilBuilt();
        }
    }
    std::cout << "Max supply: " << m_maxSupply << std::endl;
    std::cout << "Supply in progress: " << m_inProgressSupply << std::endl;
    std::cout << "Current supply: " << m_currentSupply << std::endl;
    std::cout << "num depots: " << m_numDepots << ", in progress: " << m_inProgressDepots << std::endl;
    std::cout << "num pylons: " << getNumCompleted(ActionTypes::GetActionType("Pylon")) << ", in progress: " << getNumInProgress(ActionTypes::GetActionType("Pylon")) << std::endl;
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
        if (action.isBuilding() && !action.isRefinery() && !action.isSupplyProvider() && !action.isDepot())
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
        for (int unitID : m_unitTypes[req.getRaceActionID()])
        {
            const Unit& unit = getUnit(unitID);
            BOSS_ASSERT(unit.getType() == req, "Unit type %s must equal prereq type %s", unit.getType().getName().c_str(), req.getName().c_str());
            minReady = std::min(minReady, unit.getTimeUntilBuilt());
            if (unit.getTimeUntilBuilt() == 0) { break; }
        }
        // we can only build the type after the LAST of the prereqs are ready
        whenPrereqReady = std::max(whenPrereqReady, minReady);

        BOSS_ASSERT(whenPrereqReady != std::numeric_limits<TimeType>::max(), "Did not find a prerequisite required to build %s", action.getName().c_str());
    }
      
    return m_currentFrame + whenPrereqReady;
}

int GameState::timeUntilFirstPylonDone() const
{
    for (int pylonID : m_unitTypes[ActionTypes::GetSupplyProvider(m_race).getRaceActionID()])
    {
        BOSS_ASSERT(getUnit(pylonID).getType() == ActionTypes::GetSupplyProvider(m_race), "Change inside time until first pylon done causes error");
        return getUnit(pylonID).getTimeUntilBuilt();
    }

    return std::numeric_limits<int>::max();
}

int GameState::whenEnergyReady(ActionType action) const
{
    TimeType minWhenReady = std::numeric_limits<TimeType>::max();

    // look over all our units and get when the next builder type is free
    //for (auto & unit : m_units)
    for (int unitID : m_unitTypes[action.whatBuilds().getRaceActionID()])
    {
        const Unit& unit = m_units[unitID];
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
    //for (auto & unit : m_units)
    for (int unitID : m_unitTypes[action.whatBuilds().getRaceActionID()])
    {
        const Unit& unit = m_units[unitID];
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
    return m_unitTypes[type.whatBuilds().getRaceActionID()].size() > 0;
}

bool GameState::havePrerequisites(ActionType type) const
{
    for (const ActionType & req : type.required())
    {
        if (m_unitTypes[req.getRaceActionID()].size() == 0)
        {
            return false;
        }
    }

    return true;
}

int GameState::getNumInProgress(ActionType action) const
{
    return (int)std::count_if(m_unitsBeingBuilt.begin(), m_unitsBeingBuilt.end(),
           [this, &action](int id) { return action == this->getUnit(id).getType(); } );
}

int GameState::getNumCompleted(ActionType action) const
{
    int finished = 0;
    for (int unitID : m_unitTypes[action.getRaceActionID()])
    {
        if (getUnit(unitID).getTimeUntilBuilt() == 0)
        {
            finished++;
        }
    }
    return finished;
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
    return (int)m_unitTypes[action.getRaceActionID()].size();
}

bool GameState::haveType(ActionType action) const
{
    return m_unitTypes[action.getRaceActionID()].size() > 0;
}

int GameState::timeUntilResearchDone(ActionType action) const
{
    if (m_unitTypes[action.getRaceActionID()].size() == 0)
    {
        return std::numeric_limits<int>::max();
    }

    return getUnit(m_unitTypes[action.getRaceActionID()][0]).getTimeUntilBuilt();
}

void GameState::getSpecialAbilityTargets(ActionSetAbilities & actionSet, int index) const
{
    if (m_race == Races::Protoss)
    {
        storeChronoBoostTargets(actionSet, index);
        actionSet.remove(ActionTypes::GetSpecialAction(m_race), index); // remove placeholder 
    }
}

int GameState::storeChronoBoostTargets(ActionSetAbilities & actionSet, int index) const
{
    int numTargets = 0;
    for (auto & unit : m_units)
    {
        if (chronoBoostableTarget(unit))
        {
            //std::cout << "Chronoboost target: " << unit.getType().getName() << std::endl;
            actionSet.add(ActionTypes::GetSpecialAction(m_race), unit.getID(), index + 1);
            numTargets++;
        }
    }
    return numTargets;
}

// minimum criteria that a unit must meet in order to be Chronoboostable
bool GameState::chronoBoostableTarget(const Unit & unit) const
{
    // can't cast on a morphed unit as they are just a placeholder
    if (unit.isMorphed()) { return false; }

    // can only be used on buildings
    if (!unit.getType().isBuilding()) { return false; }

    // can't chrono boost refinery
    if (unit.getType().isRefinery()) { return false; }

    // can't chrono boost pylon
    if (unit.getType().isSupplyProvider()) { return false; }

    // the building must be finished
    if (unit.getTimeUntilBuilt() > 0) { return false; }

    // for now we don't allow chronoboost on a warpgate
    // TODO: implement this properly so it can be used with CommandCenter. Works properly on its own.
    if (unit.getType() == ActionTypes::GetWarpgateAction()) { return false; }

    // the unit must be producing something
    if (whenCanCast(ActionTypes::GetSpecialAction(m_race), unit.getID()) == -1) { return false; }

    // only allow chronoboost to be used on buildings that are producing a combat unit
    //if (unit.getBuildType().isWorker()) { return false; }

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
  ss << "Supply In Progress: " << m_inProgressSupply << "\n";
  ss << "Next Probe Finish: " << getNextFinishTime(ActionTypes::GetActionType("Probe")) << "\n";
  //printPath();

  return ss.str();
}

void GameState::printunitsbeingbuilt() const
{
    for (int index : m_unitsBeingBuilt)
    {
        std::cout << getUnit(index).getTimeUntilBuilt() << std::endl;
    }
    std::cout << std::endl;
}

void GameState::printUnits() const
{
    for (auto & unit : m_units)
    {
        std::cout << "id: " << unit.getID() << ". name: " << unit.getType().getName() << std::endl;
    }

    std::cout << std::endl;
}

#include "Eval.h"

std::pair<std::string, int> GameState::getStateData(const CombatSearchParameters & params, FracType currentValue, const std::vector<int>& chronoboostTargets) const
{    
    std::stringstream state;
    state.precision(4);

    std::vector<int> unitCount = std::vector<int>(ActionTypes::GetRaceActionCount(m_race), 0);
    int unitsWritten = 0;
    for (int index = 0; index < m_units.size(); ++index)
    {
        const auto& unit = m_units[index];
        ActionType type = unit.getType();
        if (unit.getTimeUntilBuilt() == 0 && (type.isWorker() || type.isRefinery() || type.isSupplyProvider() || type.isDepot() ||
            (!type.isBuilding() && !type.isSupplyProvider() && !type.isUpgrade() && !type.isAbility())))
        {
            ++unitCount[type.getRaceActionID()];
        }
        else
        {
            state << m_units[index].getData() << ",";
            ++unitsWritten;
        }
    }
    for (int i = 0; i < unitCount.size(); ++i)
    {
        state << unitCount[i] << ",";
    }

    /*const auto unitValues = Eval::GetUnitValuesVector();
    const auto unitWeights = Eval::GetUnitWeightVector();

    BOSS_ASSERT(unitValues.size() == unitWeights.size() && unitValues.size() == unitCount.size(), 
        "Sizes of these should be equal, but are %i, %i, %i", unitValues.size(), unitWeights.size(), unitCount.size());
    for (int i = 0; i < unitWeights.size(); ++i)
    {
        state << unitValues[i] + unitWeights[i] << ",";
    }*/

    state << Eval::getUnitWeightsString();

    //ss << int(m_race) << ",";
    state << m_minerals;
    state << ",";
    state << m_gas;
    state << ",";
    state << m_currentSupply;
    state << ",";
    state << m_maxSupply;
    state << ",";
    state << m_currentFrame;
    state << ",";
    //ss << m_previousFrame << ",";
    state << (params.getFrameTimeLimit() - m_currentFrame);
    state << ",";
    state << m_mineralWorkers;
    state << ",";
    state << m_gasWorkers;
    state << ",";
    state << m_buildingWorkers;
    state << ",";
    state << currentValue;
    state << ",";
    state << CONSTANTS::MPWPF;
    state << ",";
    state << CONSTANTS::GPWPF;
    state << ",";
    state << CONSTANTS::ERPF;

    // chronoboost targets
    for (int target : chronoboostTargets)
    {
        state << ",";
        state << target;
    }
    return std::make_pair(state.str(), unitsWritten);
}

json GameState::writeToJson(const CombatSearchParameters & params) const
{
    json j;
    //j["State"]["Units"] = json::array();
    for (int index = 0; index < m_units.size(); ++index)
    {
        j["Units"].push_back(m_units[index].writeToJson());
    }

    //j["State"]["UnitsBeingBuilt"] = json::array();
    for (int index = 0; index < m_unitsBeingBuilt.size(); ++index)
    {
        j["UnitsBeingBuilt"].push_back(m_unitsBeingBuilt[index]);
    }

    //j["State"]["UnitsSortedEndFrame"] = json::array();
    for (int index = 0; index < m_unitsSortedEndFrame.size(); ++index)
    {
        j["UnitsSortedEndFrame"].push_back(m_unitsSortedEndFrame[index]);
    }

    //j["State"]["ChronoBoosts"] = json::array();
    for (int index = 0; index < m_chronoBoosts.size(); ++index)
    {
        j["ChronoBoosts"].push_back(m_chronoBoosts[index].writeToJson());
    }

    j["Race"] = int(m_race);
    j["Minerals"] = m_minerals;
    j["Gas"] = m_gas;
    j["CurrentSupply"] = m_currentSupply;
    j["SupplyInProgress"] = m_inProgressSupply;
    j["MaxSupply"] = m_maxSupply;
    j["CurrentFrame"] = m_currentFrame;
    j["PreviousFrame"] = m_previousFrame;
    j["FrameTimeLimit"] = params.getFrameTimeLimit();
    j["MineralWorkers"] = m_mineralWorkers;
    j["GasWorkers"] = m_gasWorkers;
    j["BuildingWorkers"] = m_buildingWorkers;
    j["NumRefineries"] = m_numRefineries;
    j["RefinesInProgress"] = m_inProgressRefineries;
    j["NumDepots"] = m_numDepots;
    j["DepotsInProgress"] = m_inProgressDepots;
    j["LastAction"] = m_lastAction.getID();
    j["LastAbility"] = m_lastAbility.writeToJson();    

    return j;
}