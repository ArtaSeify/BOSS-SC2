/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch.h"
#include "Tools.h"

using namespace BOSS;


// function which is called to do the actual search
void CombatSearch::search()
{
    m_searchTimer.start();

    // apply the opening build order to the initial state
    //std::vector<Unit> units;
    /*units.push_back(Unit(ActionTypes::GetActionType("Probe"), 0, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 1, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 2, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 3, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 4, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 5, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 6, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 7, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 8, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Nexus"), 9, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 10, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 11, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 12, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 13, -1, 1));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 14, -1, 273));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 15, -1, 318));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 16, -1, 545));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 17, -1, 817));
    units.push_back(Unit(ActionTypes::GetActionType("Gateway"), 18, -1, 817));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 19, -1, 965));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 20, -1, 1089));
    units.push_back(Unit(ActionTypes::GetActionType("Assimilator"), 21, -1, 1173));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 22, -1, 1271));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 23, -1, 1857));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 24, -1, 1857));
    units.push_back(Unit(ActionTypes::GetActionType("CyberneticsCore"), 25, -1, 1857));
    units.push_back(Unit(ActionTypes::GetActionType("Assimilator"), 26, -1, 1868));
    units.push_back(Unit(ActionTypes::GetActionType("Stargate"), 27, -1, 2657));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 28, -1, 2657));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 29, -1, 2742));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 30, -1, 3617));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 31, -1, 3617));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 32, -1, 3617));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 33, -1, 3617));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 34, -1, 3617));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 35, -1, 3782));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 36, -1, 4417));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 37, -1, 4417));
    units.push_back(Unit(ActionTypes::GetActionType("Gateway"), 38, -1, 4559));
    units.push_back(Unit(ActionTypes::GetActionType("Stargate"), 39, -1, 4807));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 40, -1, 5007));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 41, -1, 5408));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 42, -1, 5608));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 43, -1, 6075));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 44, -1, 6260));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 45, -1, 6445));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 46, -1, 6772));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 47, -1, 6772));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 48, -1, 7172));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 49, -1, 7172));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 50, -1, 7491));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 51, -1, 8065));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 52, -1, 8065));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 53, -1, 8213));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 54, -1, 8413));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 55, -1, 8673));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 56, -1, 8773));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 57, -1, 9085));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 58, -1, 9374));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 59, -1, 9575));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 60, -1, 9975));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 61, -1, 10376));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 62, -1, 10536));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 63, -1, 10937));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 64, -1, 11097));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 65, -1, 11257));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 66, -1, 11458));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 67, -1, 11858));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 68, -1, 12059));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 69, -1, 12219));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 70, -1, 12379));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 71, 39, 12780));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 72, 18, 12980));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 73, 38, 13140));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 74, 0, 13301));*/


    /*units.push_back(Unit(ActionTypes::GetActionType("Probe"), 0, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 1, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Gateway"), 2, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 3, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 4, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 5, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 6, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 7, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 8, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 9, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 10, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stargate"), 11, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 12, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 13, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 14, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 15, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 16, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 17, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Nexus"), 18, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 19, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 20, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 21, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 22, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Assimilator"), 23, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 24, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 25, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 26, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 27, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 28, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 29, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 30, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 31, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 32, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 33, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 34, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 35, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 36, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 37, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Assimilator"), 38, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 39, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 40, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 41, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 42, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 43, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 44, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 45, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 46, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Probe"), 47, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stargate"), 48, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 49, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 50, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 51, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 52, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 53, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 54, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 55, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 56, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("CyberneticsCore"), 57, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 58, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 59, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Gateway"), 60, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 61, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 62, -1, 0));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 63, -1, 632));
    units.push_back(Unit(ActionTypes::GetActionType("Pylon"), 64, 0, 932));
    units.push_back(Unit(ActionTypes::GetActionType("Stalker"), 65, 2, 932));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 66, 11, 932));
    units.push_back(Unit(ActionTypes::GetActionType("VoidRay"), 67, 48, 932));
    units.push_back(Unit(ActionTypes::GetActionType("Zealot"), 68, 60, 932));

    GameState s(units, BOSS::Races::Protoss, 5355, 5172, 93, 95, 14, 6, 1, 932, 2, 1);
    m_params.setInitialState(s);*/
    
    GameState initialState(m_params.getInitialState());
    m_buildOrder = m_params.getOpeningBuildOrder();
    Tools::DoBuildOrder(initialState, m_buildOrder);
    Eval::CalculateUnitWeightVector(initialState, m_params);

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
}

// This function generates the legal actions from a GameState based on the input search parameters
void CombatSearch::generateLegalActions(const GameState & state, ActionSetAbilities & legalActions, const CombatSearchParameters & params)
{
    // prune actions we have too many of already
    const ActionSetAbilities & allActions = params.getRelevantActions();
    for (auto it = allActions.begin(); it != allActions.end(); ++it)
    {
        ActionType action = it->first;

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

        if (action.isAbility())
        {
            state.getSpecialAbilityTargets(legalActions, legalActions.size()-1);
        }
    }
    
    //std::cout << legalActions.toString() << std::endl;

    // if we enabled the always make workers flag, and workers are legal
    ActionType worker = ActionTypes::GetWorker(state.getRace());
    ActionSetAbilities illegalActions;
    if (m_params.getAlwaysMakeWorkers() && legalActions.contains(worker))
    {
        bool actionLegalBeforeWorker = false;

        // when can we make a worker
        int workerReady = state.whenCanBuild(worker);

        if (workerReady > params.getFrameTimeLimit())
        {
            illegalActions.add(worker);
        }
        // if we can make a worker in the next couple of frames, do it
        else if (workerReady <= state.getCurrentFrame() + 2)
        {
            legalActions.clear();
            legalActions.add(worker);
            return;
        }

        // figure out if anything can be made before a worker
        for (auto it = legalActions.begin(); it != legalActions.end(); ++it)
        {
            ActionType actionType = it->first;

            int whenCanPerformAction = state.whenCanBuild(actionType, it->second);

            // if action goes past the time limit, it is illegal
            if (whenCanPerformAction > params.getFrameTimeLimit())
            {
                illegalActions.add(actionType);
            }

            if (!actionType.isAbility() && whenCanPerformAction < workerReady)
            {
                actionLegalBeforeWorker = true;
            }
        }

        // no legal action
        if (illegalActions.size() == legalActions.size())
        {
            legalActions.clear();
            return;
        }

        // if something can be made before a worker, then don't consider workers
        if (actionLegalBeforeWorker)
        {
            // remove illegal actions, which now includes worker
            illegalActions.add(worker);
            legalActions.remove(illegalActions);
        }
        // otherwise we can make a worker next so don't consider anything else
        else
        {
            legalActions.clear();
            if (workerReady <= params.getFrameTimeLimit())
            {
                legalActions.add(worker);
            }
        }
    }

    else
    {
        // figure out if any action goes past the time limit
        for (auto it = legalActions.begin(); it != legalActions.end(); ++it)
        {
            ActionType actionType = it->first;
            int whenCanPerformAction = state.whenCanBuild(actionType, it->second);

            // if action goes past the time limit, it is illegal
            if (whenCanPerformAction > params.getFrameTimeLimit())
            {
                illegalActions.add(actionType);
            }
        }

        // remove illegal actions
        legalActions.remove(illegalActions);
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
    return (m_params.getSearchTimeLimit() && (m_results.nodeVisits % 100 == 0) && (m_searchTimer.getElapsedTimeInMilliSec() > m_params.getSearchTimeLimit()));
}

void CombatSearch::finishSearch()
{
    m_params.setSearchTimeLimit(1);
}

bool CombatSearch::isTerminalNode(const GameState & s, int /*!!! PROBLEM NOT USED depth */)
{
  //!!! IMPROVEMENT: just say return s.getCurrent....
    return s.getCurrentFrame() > m_params.getFrameTimeLimit();
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
