#include "CombatSearchParameters_MCTS.h"

using namespace BOSS;

CombatSearchParameters_MCTS::CombatSearchParameters_MCTS()
    : m_explorationValue(1)
    , m_changingRoot()
    , m_numberOfSimulations(std::numeric_limits<int>::max())
    , m_nodeLimit(std::numeric_limits<uint8>::max())
    , m_threads(1)
    , m_useTotalTimeLimit(false)
    , m_nodeVisitsBeforeExpand(0)
{

}

void CombatSearchParameters_MCTS::setExplorationValue(FracType value)
{
    m_explorationValue = value;
}

FracType CombatSearchParameters_MCTS::getExplorationValue() const
{
    return m_explorationValue;
}

void CombatSearchParameters_MCTS::setChangingRoot(bool value)
{
    m_changingRoot.changingRoot = value;
}

bool CombatSearchParameters_MCTS::getChangingRoot() const
{
    return m_changingRoot.changingRoot;
}

void CombatSearchParameters_MCTS::setChangingRootReset(bool value)
{
    m_changingRoot.changingRootReset = value;
}

bool CombatSearchParameters_MCTS::getChangingRootReset() const
{
    return m_changingRoot.changingRootReset;
}

void CombatSearchParameters_MCTS::setNumberOfSimulations(int value)
{
    m_numberOfSimulations = value;
}

int CombatSearchParameters_MCTS::getNumberOfSimulations() const
{
    return m_numberOfSimulations;
}

void CombatSearchParameters_MCTS::setNodeLimit(uint8 value)
{
    m_nodeLimit = value;
}
uint8 CombatSearchParameters_MCTS::getNodeLimit() const
{
    return m_nodeLimit;
}

void CombatSearchParameters_MCTS::setSimulationsPerStep(int value)
{
    m_changingRoot.simulationsPerStep = value;
}

int CombatSearchParameters_MCTS::getSimulationsPerStep() const
{
    return m_changingRoot.simulationsPerStep;
}

void CombatSearchParameters_MCTS::setThreads(int threads)
{
    m_threads = threads;
}

int CombatSearchParameters_MCTS::getThreads() const
{
    return m_threads;
}

void CombatSearchParameters_MCTS::setUseTotalTimeLimit(bool value)
{
    m_useTotalTimeLimit = value;
}

bool CombatSearchParameters_MCTS::getUseTotalTimeLimit() const
{
    return m_useTotalTimeLimit;
}

void CombatSearchParameters_MCTS::setNodeVisitsBeforeExpand(int value)
{
    m_nodeVisitsBeforeExpand = value;
}

int CombatSearchParameters_MCTS::getNodeVisitsBeforeExpand() const
{
    return m_nodeVisitsBeforeExpand;
}