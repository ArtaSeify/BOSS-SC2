/* -*- c-basic-offset: 4 -*- */
#ifdef __linux__
#include <sys/resource.h>
#include <stdio.h> 
#include <sys/time.h>   
#include <sys/resource.h> 
#endif

#include "CombatSearch.h"
#include "Tools.h"

#include <thread>

using namespace BOSS;


// function which is called to do the actual search
void CombatSearch::search()
{
    m_searchTimer.start();
    m_searchTimerCPU = boost::chrono::thread_clock::now();
    
#ifdef __linux__
    struct rusage buf;
    getrusage(RUSAGE_THREAD, &buf);
    double startCPUTime = ((buf.ru_utime.tv_sec) + (buf.ru_utime.tv_usec / 1000000.f) + (buf.ru_stime.tv_sec) + (buf.ru_stime.tv_usec / 1000000.f))*1000.0;
#endif
    /*FILETIME creation, exit, kernel, user;
    auto th = GetCurrentThread();
    BOSS_ASSERT(GetThreadTimes(th, &creation, &exit, &kernel, &user), "failed");*/

    // apply the opening build order to the initial state    
    GameState initialState(m_params.getInitialState());
    m_buildOrder = m_params.getOpeningBuildOrder();
    Tools::DoBuildOrder(initialState, m_buildOrder);
    Eval::CalculateUnitValues(initialState);
    Eval::SetUnitWeightVector(Eval::CalculateUnitWeightVector(initialState, m_params.getEnemyUnits()));

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
    m_results.timeElapsedCPU = boost::chrono::duration_cast<boost::chrono::duration<double, boost::milli>>(boost::chrono::thread_clock::now() - m_searchTimerCPU).count();
    
    #ifdef __linux__
        struct rusage bufEnd;
        getrusage(RUSAGE_THREAD, &bufEnd);
        double endCPUTime = ((bufEnd.ru_utime.tv_sec) + (bufEnd.ru_utime.tv_usec / 1000000.f) + (bufEnd.ru_stime.tv_sec) + (bufEnd.ru_stime.tv_usec / 1000000.f))*1000.0;
    
        //std::cout << "CPU time according to boost: " << m_results.timeElapsedCPU << std::endl;
        //std::cout << "CPU time accoding to Linux: " << endCPUTime - startCPUTime << std::endl;
    #endif


    /*FILETIME creationEnd, exitEnd, kernelEnd, userEnd;
    BOSS_ASSERT(GetThreadTimes(th, &creationEnd, &exitEnd, &kernelEnd, &userEnd), "failed");

    SYSTEMTIME stKernel, stKernelEnd, stUser, stUserEnd;
    BOSS_ASSERT(FileTimeToSystemTime(&kernel, &stKernel), "failed");
    BOSS_ASSERT(FileTimeToSystemTime(&user, &stUser), "failed");
    BOSS_ASSERT(FileTimeToSystemTime(&kernelEnd, &stKernelEnd), "failed");
    BOSS_ASSERT(FileTimeToSystemTime(&userEnd, &stUserEnd), "failed");

    double totalStart = stKernel.wMilliseconds + stKernel.wSecond * 1000 + stKernel.wMinute * 60 * 1000 + stKernel.wHour * 60 * 60 * 1000 +
        stUser.wMilliseconds + stUser.wSecond * 1000 + stUser.wMinute * 60 * 1000 + stUser.wHour * 60 * 60 * 1000;
    double totalEnd = stKernelEnd.wMilliseconds + stKernelEnd.wSecond * 1000 + stKernelEnd.wMinute * 60 * 1000 + stKernelEnd.wHour * 60 * 60 * 1000 +
        stUserEnd.wMilliseconds + stUserEnd.wSecond * 1000 + stUserEnd.wMinute * 60 * 1000 + stUserEnd.wHour * 60 * 60 * 1000;
    
    std::cout << "thread time through windows: " << totalEnd - totalStart << std::endl;
    std::cout << "thread time through boost: " << m_results.timeElapsedCPU << std::endl;
    std::cout << "kernel: " << stKernel.wMilliseconds + stKernel.wSecond * 1000 + stKernel.wMinute * 60 * 1000 + stKernel.wHour * 60 * 60 * 1000 << " "
        << stKernelEnd.wMilliseconds + stKernelEnd.wSecond * 1000 + stKernelEnd.wMinute * 60 * 1000 + stKernelEnd.wHour * 60 * 60 * 1000 << std::endl;
    std::cout << "user: " << stUser.wMilliseconds + stUser.wSecond * 1000 + stUser.wMinute * 60 * 1000 + stUser.wHour * 60 * 60 * 1000 << " "
        << stUserEnd.wMilliseconds + stUserEnd.wSecond * 1000 + stUserEnd.wMinute * 60 * 1000 + stUserEnd.wHour * 60 * 60 * 1000 << std::endl;*/
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
