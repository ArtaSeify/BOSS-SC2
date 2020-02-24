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
    , m_frameTimeLimit(0)
    , m_initialUpperBound(0)
    , m_initialState(GameState())
    , m_openingBuildOrder(BuildOrderAbilities())
    , m_enemyBuildOrder(BuildOrderAbilities())
    , m_printNewBest(false)
    , m_sortActions(false)
    , m_saveStates(false)
    , m_explorationValue(1)
    , m_changingRoot(false)
    , m_changingRootReset(false)
    , m_useMaxValue(false)
    , m_maximizeValue(false)
    , m_numberOfSimulations(std::numeric_limits<int>::max())
    , m_numberOfNodes(std::numeric_limits<uint8>::max())
    , m_simulationsPerStep(std::numeric_limits<int>::max())
    , m_simulationsPerStepDecay(1)
    , m_usePolicyNetwork(0)
    , m_usePolicyValueNetwork(0)
    , m_mixingValue(0)
    , m_threadsForMCTS(1)
    , m_numPlayouts(0)
    , m_level(0)
    , m_enemyUnits()
    , m_enemyRace(Races::None)
    , m_useTotalTimeLimit(false)
    , m_temperatureChange(0)
    , m_nodeVisitsBeforeExpand(0)
    , m_valueNormalization(1)
    , m_valueTargetMix(0)
    , m_useSimulationValueOnly(false)
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

void CombatSearchParameters::setRelevantActions(const ActionSetAbilities & set)
{
    m_relevantActions = set;
}

const ActionSetAbilities & CombatSearchParameters::getRelevantActions() const
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

void CombatSearchParameters::setOpeningBuildOrder(const BuildOrderAbilities & buildOrder)
{
    m_openingBuildOrder = buildOrder;
}

const BuildOrderAbilities & CombatSearchParameters::getOpeningBuildOrder() const
{
    return m_openingBuildOrder;
}

void CombatSearchParameters::setEnemyBuildOrder(const BuildOrderAbilities & buildOrder)
{
    m_enemyBuildOrder = buildOrder;
}

const BuildOrderAbilities & CombatSearchParameters::getEnemyBuildOrder() const
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

void CombatSearchParameters::setFrameTimeLimit(int limit)
{
    m_frameTimeLimit = limit;
}

void CombatSearchParameters::setAlwaysMakeWorkers(bool flag)
{
    m_useAlwaysMakeWorkers = flag;
}

bool CombatSearchParameters::getAlwaysMakeWorkers() const
{
    return m_useAlwaysMakeWorkers;
}   

int CombatSearchParameters::getFrameTimeLimit() const
{
    return m_frameTimeLimit;
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

void CombatSearchParameters::setSaveStates(bool flag)
{
    m_saveStates = flag;
}

bool CombatSearchParameters::getSaveStates() const
{
    return m_saveStates;
}

void CombatSearchParameters::setExplorationValue(FracType value)
{
    m_explorationValue = value;
}

FracType CombatSearchParameters::getExplorationValue() const
{
    return m_explorationValue;
}

void CombatSearchParameters::setChangingRoot(bool value)
{
    m_changingRoot = value;
}

bool CombatSearchParameters::getChangingRoot() const
{
    return m_changingRoot;
}

void CombatSearchParameters::setChangingRootReset(bool value)
{
    m_changingRootReset = value;
}

bool CombatSearchParameters::getChangingRootReset() const
{
    return m_changingRootReset;
}

void CombatSearchParameters::setUseMaxValue(bool value)
{
    m_useMaxValue = value;
}

bool CombatSearchParameters::getUseMaxValue() const
{
    return m_useMaxValue;
}

void CombatSearchParameters::setMaximizeValue(bool value)
{
    m_maximizeValue = value;
}

bool CombatSearchParameters::getMaximizeValue() const
{
    return m_maximizeValue;
}

void CombatSearchParameters::setNumberOfSimulations(int value)
{
    m_numberOfSimulations = value;
}

int CombatSearchParameters::getNumberOfSimulations() const
{
    return m_numberOfSimulations;
}

void CombatSearchParameters::setNumberOfNodes(uint8 value)
{
    m_numberOfNodes = value;
}
uint8 CombatSearchParameters::getNumberOfNodes() const
{
    return m_numberOfNodes;
}

void CombatSearchParameters::setSimulationsPerStep(int value)
{
    m_simulationsPerStep = value;
}

int CombatSearchParameters::getSimulationsPerStep() const
{
    return m_simulationsPerStep;
}

void CombatSearchParameters::setSimulationsPerStepDecay(FracType value)
{
    m_simulationsPerStepDecay = value;
}

FracType CombatSearchParameters::getSimulationsPerStepDecay() const
{
    return m_simulationsPerStepDecay;
}

void CombatSearchParameters::setUsePolicyNetwork(bool value)
{
    m_usePolicyNetwork = value;
}

bool CombatSearchParameters::usePolicyNetwork() const
{
    return m_usePolicyNetwork;
}

void CombatSearchParameters::setUsePolicyValueNetwork(bool value)
{
    m_usePolicyValueNetwork = value;
}

bool CombatSearchParameters::usePolicyValueNetwork() const
{
    return m_usePolicyValueNetwork;
}

void CombatSearchParameters::setMixingValue(FracType value)
{
    BOSS_ASSERT(value >= 0 && value <= 1, "Mixing value must be between 0 and 1");
    m_mixingValue = value;
}

FracType CombatSearchParameters::getMixingValue() const
{
    return m_mixingValue;
}

void CombatSearchParameters::setThreadsForMCTS(int threads)
{
    m_threadsForMCTS = threads;
}

int CombatSearchParameters::getThreadsForMCTS() const
{
    return m_threadsForMCTS;
}

void CombatSearchParameters::setNumPlayouts(int value)
{
    m_numPlayouts = value;
}
int CombatSearchParameters::getNumPlayouts() const
{
    return m_numPlayouts;
}

void CombatSearchParameters::setLevel(int value)
{
    m_level = value;
}
int CombatSearchParameters::getLevel() const
{
    return m_level;
}

void CombatSearchParameters::setUseTotalTimeLimit(bool value)
{
    m_useTotalTimeLimit = value;
}

bool CombatSearchParameters::getUseTotalTimeLimit() const
{
    return m_useTotalTimeLimit;
}

void CombatSearchParameters::setTemperatureChange(int value)
{
    m_temperatureChange = value;
}

int CombatSearchParameters::getTemperatureChange() const
{
    return m_temperatureChange;
}

void CombatSearchParameters::setValueNormalization(int value)
{
    m_valueNormalization = value;
}

int CombatSearchParameters::getValueNormalization() const
{
    return m_valueNormalization;
}

void CombatSearchParameters::setNodeVisitsBeforeExpand(int value)
{
    m_nodeVisitsBeforeExpand = value;
}

int CombatSearchParameters::getNodeVisitsBeforeExpand() const
{
    return m_nodeVisitsBeforeExpand;
}

void CombatSearchParameters::setValueTargetMix(FracType value)
{
    m_valueTargetMix = value;
}

FracType CombatSearchParameters::getValueTargetMix() const
{
    return m_valueTargetMix;
}

void CombatSearchParameters::setUseSimulationValueOnly(bool value)
{
    m_useSimulationValueOnly = value;
}

bool CombatSearchParameters::useSimulationValueOnly() const
{
    return m_useSimulationValueOnly;
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
