/* -*- c-basic-offset: 4 -*- */

#include "CombatSearchParameters.h"
#include "ActionType.h"

using namespace BOSS;

// alternate constructor
CombatSearchParameters::CombatSearchParameters()
    : m_maxActions(ActionTypes::GetAllActionTypes().size(), -1)
    , m_useRepetitions(true)
    , m_repetitionValues(ActionTypes::GetAllActionTypes().size(), 1)
    , m_useIncreasingRepetitions(false)
    , m_repetitionThresholds(ActionTypes::GetAllActionTypes().size(), 0)
    , m_useWorkerCutoff(false)
    , m_workerCutoff(1)
    , m_useAlwaysMakeWorkers(false)
    , m_useSupplyBounding(false)
    , m_supplyBoundingThreshold(1)
    , m_useLandmarkLowerBoundHeuristic(false)
    , m_useResourceLowerBoundHeuristic(false)
    , m_searchTimeLimit(0)
    , m_frameLimit(0)
    , m_initialUpperBound(0)
    , m_initialState(GameState())
    , m_openingBuildOrder(BuildOrder())
    , m_enemyBuildOrder(BuildOrder())
    , m_printNewBest(false)
    , m_sortActions(false)
    , m_maximizeValue(false)
    , m_enemyUnits()
    , m_enemyRace(Races::None)
{
    
}

void CombatSearchParameters::setSearchTimeLimit(float timeLimitMS)
{
    m_searchTimeLimit = timeLimitMS;
}

float CombatSearchParameters::getSearchTimeLimit() const
{
    return m_searchTimeLimit;
}

void CombatSearchParameters::setRelevantActions(const ActionSet & set)
{
    m_relevantActions = set;
}

const ActionSet & CombatSearchParameters::getRelevantActions() const
{
    return m_relevantActions;
}

void CombatSearchParameters::setInitialState(const GameState & s)
{
    m_initialState = s;
}

const GameState & CombatSearchParameters::getInitialState() const
{
    return m_initialState;
}

void CombatSearchParameters::setEnemyInitialState(const GameState & s)
{
    m_enemyInitialState = s;
}

const GameState & CombatSearchParameters::getEnemyInitialState() const
{
    return m_enemyInitialState;
}

void CombatSearchParameters::setMaxActions(ActionType a, int max)
{
    m_maxActions[a.getID()] = max;
}

void CombatSearchParameters::setOpeningBuildOrder(const BuildOrder & buildOrder)
{
    m_openingBuildOrder = buildOrder;
}

const BuildOrder & CombatSearchParameters::getOpeningBuildOrder() const
{
    return m_openingBuildOrder;
}

void CombatSearchParameters::setEnemyBuildOrder(const BuildOrder & buildOrder)
{
    m_enemyBuildOrder = buildOrder;
}

const BuildOrder & CombatSearchParameters::getEnemyBuildOrder() const
{
    return m_enemyBuildOrder;
}

void CombatSearchParameters::setEnemyUnits(const std::vector<int> & units)
{
    m_enemyUnits = units;
}

const std::vector<int> & CombatSearchParameters::getEnemyUnits() const
{
    return m_enemyUnits;
}

void CombatSearchParameters::setEnemyRace(RaceID race)
{
    m_enemyRace = race;
}

RaceID CombatSearchParameters::getEnemyRace() const
{
    return m_enemyRace;
}

void CombatSearchParameters::setRepetitions(ActionType a,int repetitions)
{ 
    m_repetitionValues[a.getID()] = repetitions; 
}

int CombatSearchParameters::getMaxActions(ActionType a) const
{ 
    return m_maxActions[a.getID()]; 
}

int CombatSearchParameters::getRepetitions(ActionType a) const
{ 
    return m_repetitionValues[a.getID()]; 
}

void CombatSearchParameters::setFrameLimit(int limit)
{
    m_frameLimit = limit;
}

void CombatSearchParameters::setAlwaysMakeWorkers(bool flag)
{
    m_useAlwaysMakeWorkers = flag;
}

bool CombatSearchParameters::getAlwaysMakeWorkers() const
{
    return m_useAlwaysMakeWorkers;
}   

int CombatSearchParameters::getFrameLimit() const
{
    return m_frameLimit;
}

void CombatSearchParameters::setPrintNewBest(bool flag)
{
    m_printNewBest = flag;
}

bool CombatSearchParameters::getPrintNewBest() const
{
    return m_printNewBest;
}

void CombatSearchParameters::setSortActions(bool flag)
{
    m_sortActions = flag;
}

bool CombatSearchParameters::getSortActions() const
{
    return m_sortActions;
}



void CombatSearchParameters::setMaximizeValue(bool value)
{
    m_maximizeValue = value;
}

bool CombatSearchParameters::getMaximizeValue() const
{
    return m_maximizeValue;
}

void CombatSearchParameters::print()
{
    std::stringstream ss;
    ss << "\n\nSearch Parameter Information\n\n";

    /*std::stringstream ss;
    ss << "change root: " << m_params.getChangingRoot() << std::endl;
    ss << "rollouts per step: " << m_params.getSimulationsPerStep() << std::endl;
    ss << "total rollouts: " << m_params.getNumberOfSimulations() << std::endl;
    ss << "exploration value: " << m_params.getExplorationValue() << std::endl;
    ss << "use max: " << m_params.getUseMaxValue() << std::endl;

    std::cout << ss.str() << std::endl;*/

    /*printf("%s", m_useRepetitions ?                    "\tUSE      Repetitions\n" : "");
    printf("%s", m_useIncreasingRepetitions ?          "\tUSE      Increasing Repetitions\n" : "");
    printf("%s", m_useWorkerCutoff ?                   "\tUSE      Worker Cutoff\n" : "");
    printf("%s", m_useLandmarkLowerBoundHeuristic ?    "\tUSE      Landmark Lower Bound\n" : "");
    printf("%s", m_useResourceLowerBoundHeuristic ?    "\tUSE      Resource Lower Bound\n" : "");
    printf("%s", m_useAlwaysMakeWorkers ?              "\tUSE      Always Make Workers\n" : "");
    printf("%s", m_useSupplyBounding ?                 "\tUSE      Supply Bounding\n" : "");
    printf("\n");*/

    //for (int a = 0; a < ACTIONS.size(); ++a)
    //{
    //    if (repetitionValues[a] != 1)
    //    {
    //        printf("\tREP %7d %s\n", repetitionValues[a], ACTIONS[a].getName().c_str());
    //    }
    //}

    //for (int a = 0; a < ACTIONS.size(); ++a)
    //{
    //    if (repetitionThresholds[a] != 0)
    //    {
    //        printf("\tTHR %7d %s\n", repetitionThresholds[a], ACTIONS[a].getName().c_str());
    //    }
    //}

    //printf("\n\n");
}
