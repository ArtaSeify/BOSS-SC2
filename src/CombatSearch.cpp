/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch.h"
#include "Tools.h"

using namespace BOSS;


// function which is called to do the actual search
void CombatSearch::search()
{
    std::cout << "Search started!" << std::endl;
    m_searchTimer.start();

    // apply the opening build order to the initial state
    GameState initialState(m_params.getInitialState());
    m_buildOrder = m_params.getOpeningBuildOrder();
    Tools::DoBuildOrder(initialState, m_buildOrder);

    try
    {
        recurse(initialState, 0);
    
        m_results.solved = true;
    }
    catch (int e)
    {
        if (e == BOSS_COMBATSEARCH_TIMEOUT)
        {
            m_results.timedOut = true;
        }
    }

    m_results.timeElapsed = m_searchTimer.getElapsedTimeInMilliSec();
    setBestBuildOrder();
}

void CombatSearch::continueSearch()
{

}

void CombatSearch::setBestBuildOrder()
{

}

// This function generates the legal actions from a GameState based on the input search parameters
void CombatSearch::generateLegalActions(const GameState & state, ActionSetAbilities & legalActions, const CombatSearchParameters & params)
{
    // prune actions we have too many of already
    const ActionSetAbilities & allActions = params.getRelevantActions();
    for (const auto & actionAndTarget : allActions)
    {
        ActionType action = actionAndTarget.first;

        bool isLegal = state.isLegal(action);

        if (!isLegal)
        {
            continue;
        }

        // prune the action if we have too many of them already
        if ((params.getMaxActions(action) != -1) && ((int)state.getNumTotal(action) >= params.getMaxActions(action)))
        {
            continue;
        }

        legalActions.add(action);
    }

    // if we enabled the always make workers flag, and workers are legal
    ActionType worker = ActionTypes::GetWorker(state.getRace());
    if (m_params.getAlwaysMakeWorkers() && legalActions.contains(worker))
    {
        bool actionLegalBeforeWorker = false;

        // when can we make a worker
        int workerReady = state.whenCanBuild(worker);

        // if we can make a worker in the next couple of frames, do it
        if (workerReady <= state.getCurrentFrame() + 2)
        {
            legalActions.clear();
            legalActions.add(worker);
            return;
        }

        // figure out if anything can be made before a worker
        for (const auto & actionAndTarget : legalActions)
        {
            ActionType actionType = actionAndTarget.first;
            // considering abilities will break this heuristic
            if (!actionType.isAbility())
            {
                int whenCanPerformAction = state.whenCanBuild(actionType);

                if (whenCanPerformAction < workerReady)
                {
                    actionLegalBeforeWorker = true;
                    break;
                }
            }
        }

        // if something can be made before a worker, then don't consider workers
        if (actionLegalBeforeWorker)
        {
            legalActions.remove(worker);
        }
        // otherwise we can make a worker next so don't consider anything else
        else
        {
            legalActions.clear();
            legalActions.add(worker);
            //legalActions.add(ActionTypes::GetSpecialAction(state.getRace()));
        }
    }

    // sort the actions
    if (params.getSortActions())
    {
        legalActions.sort(state, m_params);
    }    
}

const CombatSearchResults & CombatSearch::getResults() const
{
    return m_results;
}

bool CombatSearch::timeLimitReached()
{
    return (m_params.getSearchTimeLimit() && (m_results.nodesExpanded % 100 == 0) && (m_searchTimer.getElapsedTimeInMilliSec() > m_params.getSearchTimeLimit()));
}

void CombatSearch::finishSearch()
{
    m_params.setSearchTimeLimit(1);
}

bool CombatSearch::isTerminalNode(const GameState & s, int /*!!! PROBLEM NOT USED depth */)
{
  //!!! IMPROVEMENT: just say return s.getCurrent....
    return s.getCurrentFrame() >= m_params.getFrameTimeLimit();
}

void CombatSearch::recurse(const GameState & /*!!! PROBLEM NOT USED state*/, int /*!!! PROBLEM NOT USED depth*/)
{
    // This base class function should never be called, leaving the code
    // here as a basis to form child classes
    BOSS_ASSERT(false, "Base CombatSearch recurse() should never be called");

    //if (timeLimitReached())
    //{
    //    throw BOSS_COMBATSEARCH_TIMEOUT;
    //}

    //updateResults(state);

    //if (isTerminalNode(state, depth))
    //{
    //    return;
    //}

    //ActionSet legalActions;
    //generateLegalActions(state, legalActions, _params);
    //
    //for (size_t a(0); a < legalActions.size(); ++a)
    //{
    //    GameState child(state);
    //    child.doAction(legalActions[a]);
    //    _buildOrder.add(legalActions[a]);
    //    
    //    doSearch(child,depth+1);

    //    _buildOrder.pop_back();
    //}
}

void CombatSearch::updateResults(const GameState & /*!!! PROBLEM NOT USED state */)
{
    m_results.nodesExpanded++;
}

void CombatSearch::printResults()
{
    std::cout << "Printing base class CombatSearch results!\n\n";
}

void CombatSearch::writeResultsFile(const std::string & /*!!! PROBLEM NOT USED dir*/, const std::string & /*!!! PROBLEM NOT USED prefix*/)
{
    std::cout << "Writing base class CombatSearch results!\n\n";
}
