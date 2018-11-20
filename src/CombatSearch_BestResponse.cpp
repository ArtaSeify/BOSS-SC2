/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch_BestResponse.h"

using namespace BOSS;

CombatSearch_BestResponse::CombatSearch_BestResponse(const CombatSearchParameters p)
    : m_bestResponseData(p.getEnemyInitialState(), p.getEnemyBuildOrder())
{
    m_params = p;

    BOSS_ASSERT(m_params.getInitialState().getRace() != Races::None, "Combat search initial state is invalid");
}

// Recursive DFS search through all possible build orders given the parameters
void CombatSearch_BestResponse::recurse(const GameState & state, int depth)
{
    if (timeLimitReached())
    {
        throw BOSS_COMBATSEARCH_TIMEOUT;
    }

    m_bestResponseData.update(m_params.getInitialState(), state, m_buildOrder);
    updateResults(state);

    if (isTerminalNode(state, depth))
    {
        return;
    }

    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);
    
    for (int a(0); a < legalActions.size(); ++a)
    {
        int ri = (int)legalActions.size() - 1 - a;

        GameState child(state);
        child.doAction(legalActions[ri].first);
        m_buildOrder.add(legalActions[ri].first);
        
        recurse(child,depth+1);

        m_buildOrder.pop_back();
    }
}

void CombatSearch_BestResponse::printResults()
{

}

#include "BuildOrderPlotter.h"
void CombatSearch_BestResponse::writeResultsFile(const std::string & dir, const std::string & /*!!! PROBLEM NOT USED filename */)
{
    BuildOrderPlotter plot;
    plot.setOutputDir(dir);
    plot.addPlot("BestResponseSelf", m_params.getInitialState(), m_bestResponseData.getBestBuildOrder());
    plot.doPlots();

    BuildOrderPlotter plot2;
    plot2.setOutputDir(dir);
    plot2.addPlot("BestResponseEnemy", m_params.getEnemyInitialState(), m_params.getEnemyBuildOrder());
    plot2.doPlots();
}
