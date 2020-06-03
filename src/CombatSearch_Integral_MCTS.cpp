#include "CombatSearch_Integral_MCTS.h"
#include "FileTools.h"

#include <random>
#include <future>
#include <omp.h>

using namespace BOSS;

CombatSearch_Integral_MCTS::CombatSearch_Integral_MCTS(const CombatSearchParameters p, const std::string& dir,
    const std::string& prefix, const std::string& name)
    : m_bestResultFound()
    , m_resultLog()
    , m_numTotalSimulations(0)
    , m_numCurrentRootSimulations(0)
    , m_simulationsPerRoot(std::numeric_limits<int>::max())
    , m_threadsWaitingForChangeRoot(0)
    , m_buildOrderIntegralChangedRoot()
    , m_changeRootMutex()
    , m_changeRootCompleted()
    , m_rootChanged(ThreadMessage::RootNotChanged)
    , m_resultFileMutex()
    , m_nodesExpanded(0)
    , m_nodeVisits(0)
    , m_leafNodesExpanded(0)
    , m_leafNodesVisited(0)
{
    m_params = p;
    
    if (m_params.getChangingRoot())
    {
        m_simulationsPerRoot = p.getSimulationsPerStep();
    }

    m_dir = dir;
    m_prefix = prefix;
    m_name = name;

    std::random_device rd; // obtain a random number from hardware
    m_rnggen.seed(rd());
        
    const gsl_rng_type * T;
    gsl_rng_env_setup();

    T = gsl_rng_default;
    m_gsl_r = gsl_rng_alloc(T);
}

CombatSearch_Integral_MCTS::~CombatSearch_Integral_MCTS()
{
    gsl_rng_free(m_gsl_r);
}

void CombatSearch_Integral_MCTS::run(const GameState& state, int depth)
{
    m_currentRoot = std::make_shared<Node>(state, *std::make_shared<Edge>());
    // no noise if we are evaluating. We are evaluating if the temperature change frame is 0
    if (m_params.getTemperatureChange() == 0)
    {
        m_currentRoot->createChildrenEdges(m_params, 0);
    }
    else
    {
        m_currentRoot->createChildrenEdges(m_params, 0, true, m_gsl_r);
    }

    std::shared_ptr<Node> topRoot = m_currentRoot;

    if (m_params.getNumberOfSimulations() == -1)
    {
        evaluatePolicyNetwork();
    }
    else
    {
        int numThreads = m_params.getThreadsForMCTS();
        std::vector<std::future<void>> threads(numThreads);
        for (int thread = 0; thread < numThreads; ++thread)
        {
            threads[thread] = std::async(std::launch::async, &CombatSearch_Integral_MCTS::MCTSSearch, this, thread);
        }

        for (auto& thread : threads)
        {
            thread.wait();
        }
        //MCTSSearch(0);

        if (!m_params.getChangingRoot())
        {
            int simulations = m_numTotalSimulations;
            BOSS_ASSERT(simulations == m_params.getNumberOfSimulations(),
                "Simulations done %i must equal to the simulations limit %i", simulations, m_params.getNumberOfSimulations());
        }
    }

    // write state data to file
    if (m_params.getSaveStates())
    {
        m_ssStates.precision(4);

        FracType targetValueMix = m_params.getValueTargetMix();

        for (const auto& data : m_stateData)
        {
            m_ssStates << data.state.first << "," << data.policy << "," << 
                (targetValueMix * (data.highestFoundValue - data.stateValue)) + ((1.0 - targetValueMix) * (m_bestResultFound.integral.getCurrentStackValue() - data.stateValue))  << "\n";
        }

        CONSTANTS::SaveDataToFile.lock();
        {            
            FileTools::MakeDirectory(CONSTANTS::ExecutablePath + "/SavedStates");
            //std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/LatestData.csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
            std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
            fileStates << m_ssStates.rdbuf();
            fileStates.close();
        }
        CONSTANTS::SaveDataToFile.unlock();
        m_ssStates.str(std::string());
    }

    m_results.nodeVisits = m_nodeVisits;
    m_results.nodesExpanded = m_nodesExpanded;
    m_results.leafNodesVisited = m_leafNodesVisited;
    m_results.leafNodesExpanded = m_leafNodesExpanded;

    BuildOrderAbilities finishedUnitsBuildOrder = createFinishedUnitsBuildOrder(m_bestResultFound.buildOrder);
    m_results.buildOrder = m_bestResultFound.buildOrder;
    m_results.finishedUnitsBuildOrder = finishedUnitsBuildOrder;
    m_results.usefulBuildOrder = createUsefulBuildOrder(finishedUnitsBuildOrder);

    m_results.eval = m_integral.getCurrentStackValue();
    m_results.value = m_integral.getCurrentStackEval();
    m_results.numSimulations = m_numTotalSimulations;

    GameState finishedUnitsState(m_params.getInitialState());
    CombatSearch_IntegralData_FinishedUnits finishedUnitsIntegral;
    for (auto& action : m_results.finishedUnitsBuildOrder)
    {
        if (action.first.isAbility())
        {
            finishedUnitsState.doAbility(action.first, action.second.targetID);
        }
        else
        {
            finishedUnitsState.doAction(action.first);
        }
        finishedUnitsIntegral.update(finishedUnitsState, m_results.finishedUnitsBuildOrder, m_params, m_searchTimer, true);
        finishedUnitsIntegral.setState(finishedUnitsState);
    }
    finishedUnitsState.fastForward(m_params.getFrameTimeLimit());
    finishedUnitsIntegral.update(finishedUnitsState, m_results.finishedUnitsBuildOrder, m_params, m_searchTimer, true);
    finishedUnitsIntegral.setState(finishedUnitsState);
    m_results.finishedEval = finishedUnitsIntegral.getCurrentStackValue();
    m_results.finishedValue = finishedUnitsIntegral.getCurrentStackEval();

    GameState usefulUnitsState(m_params.getInitialState());
    CombatSearch_IntegralData_FinishedUnits usefulUnitsIntegral;
    for (auto& action : m_results.usefulBuildOrder)
    {
        if (action.first.isAbility())
        {
            usefulUnitsState.doAbility(action.first, action.second.targetID);
        }
        else
        {
            usefulUnitsState.doAction(action.first);
        }
        usefulUnitsIntegral.update(usefulUnitsState, m_results.usefulBuildOrder, m_params, m_searchTimer, true);
        usefulUnitsIntegral.setState(usefulUnitsState);
    }
    usefulUnitsState.fastForward(m_params.getFrameTimeLimit());
    usefulUnitsIntegral.update(usefulUnitsState, m_results.usefulBuildOrder, m_params, m_searchTimer, true);
    usefulUnitsIntegral.setState(usefulUnitsState);
    m_results.usefulEval = usefulUnitsIntegral.getCurrentStackValue();
    m_results.usefulValue = usefulUnitsIntegral.getCurrentStackEval();

    topRoot->cleanUp(m_params.getThreadsForMCTS());
}

void CombatSearch_Integral_MCTS::MCTSSearch(int threadID)
{
    while (!timeLimitReached() && m_numTotalSimulations < m_params.getNumberOfSimulations() 
        && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        // change the root of the tree. Remove all the nodes and edges that are now irrelevant
        if (m_params.getChangingRoot() && m_numCurrentRootSimulations >= m_simulationsPerRoot)
        {
            changeRoot(threadID);
        }

        // we increase the simulation count at the beginning to avoid doing extra simulations due to multithreading
        int thisThreadSims = ++m_numTotalSimulations;
        int thisThreadCRSims = ++m_numCurrentRootSimulations;

        // reached the limit, decrease the count
        if (thisThreadSims > m_params.getNumberOfSimulations() || thisThreadCRSims > m_simulationsPerRoot)
        {
            --m_numTotalSimulations;
            --m_numCurrentRootSimulations;
            continue;
        }

        else
        {
            //std::cout << "have run : " << m_numTotalSimulations << " simulations thus far." << std::endl;
                
            BuildOrderIntegral buildOrderIntegral(m_buildOrderIntegralChangedRoot);

            std::shared_ptr<Edge> promisingEdge = getPromisingEdge(m_currentRoot, buildOrderIntegral);

            // we have reached node limit 
            if (m_nodeVisits > m_params.getNumberOfNodes())
            {
                break;
            }

            // child node has not been expanded yet
            if (promisingEdge->getChild() == nullptr)
            {
                GameState state(promisingEdge->getParent()->getState());
                auto action = promisingEdge->getAction();
                if (action.first.isAbility())
                {
                    if (action.first.isAbility())
                    {
                        state.doAbility(action.first, action.second.targetID);
                    }
                    else
                    {
                        state.doAction(action.first);
                    }
                }

                if (promisingEdge->realTimesVisited() >= m_params.getNodeVisitsBeforeExpand())
                {
                    if (promisingEdge->setChild(std::make_shared<Node>(promisingEdge, state)))
                    {
                        ++m_results.nodesExpanded;
                    }
                }

                rollout(state, buildOrderIntegral);
                backPropagation(promisingEdge, buildOrderIntegral);
            }

            else
            {
                const std::shared_ptr<Node>& promisingNode = promisingEdge->getChild();

                if (promisingNode->isTerminal())
                {
                    ++m_leafNodesVisited;
                    //std::cout << "terminal node chosen" << std::endl;
                    // backpropogate results
                    result.value = promisingNode->getState().m_score;
                    backPropagation(promisingEdge, result);
                }

                else
                {
                    //std::cout << "creating children" << std::endl;
                    // create children and pick a child at random
                    promisingNode->createChildrenEdges(m_params);

                    if (promisingNode->isTerminal())
                    {
                        ++m_results.leafNodesExpanded;
                        ++m_results.leafNodesTraversed;
                        //std::cout << "terminal node just created" << std::endl;
                        // backpropogate results
                        result.value = promisingNode->getState().m_score;
                        backpropogate(promisingEdge, result);
                    }

                    else
                    {
                        // pick a newly created edge at random
                        promisingEdge = promisingNode->getRandomChild(*m_moveRNG);

                        State state(promisingNode->getState());
                        state.make_move(promisingEdge->getAction());
                        result.movesMade.push_back(promisingEdge->getAction());

                        // add child to tree if nodeVisitsBeforeExpand is 1
                        if (promisingEdge->timesVisited() >= m_params.nodeVisitsBeforeExpand)
                        {
                            if (promisingEdge->setChild(std::make_shared<Node>(m_params, promisingEdge, state)))
                            {
                                ++m_results.nodesExpanded;
                            }
                        }

                        ++m_results.nodesTraversed;

                        // rollout from child and backpropogate results
                        rollout(state, result);
                        backpropogate(promisingEdge, result);
                    }
                }
            }

            // we have reached node limit 
            if (m_nodeVisits > m_params.getNumberOfNodes())
            {
                break;
            }

            BOSS_ASSERT(thisThreadSims <= m_params.getNumberOfSimulations(), "Total simulations done: %i, limit: %i", thisThreadSims, m_params.getNumberOfSimulations());
            BOSS_ASSERT(thisThreadCRSims <= m_simulationsPerRoot, "Current root simulations done: %i, limit: %i", thisThreadCRSims, m_simulationsPerRoot);

            backPropagation(*promisingNode, buildOrderIntegral);
        }
    }
}

void CombatSearch_Integral_MCTS::evaluatePolicyNetwork()
{
    BOSS_ASSERT(m_params.usePolicyNetwork() || m_params.usePolicyValueNetwork(), "UsePolicyNetwork or UsePolicyValueNetwork must be set to true when evaluating policy network");
    
    std::shared_ptr<Node> currentNode = m_currentRoot;
    std::shared_ptr<Edge> bestAction;

    while (!currentNode->isTerminal())
    {
        bestAction = currentNode->getHighestPolicyValueChild().shared_from_this();
        currentNode = currentNode->notExpandedChild(*bestAction, m_params, m_bestResultFound.integral.getCurrentStackValue(), true);

        updateBOIntegral(*currentNode, bestAction->getAction(), m_bestResultFound);
        writeSummaryToQueue();
        updateNodeVisits(true, false);
    }

    // need to do one last update
    updateIntegralTerminal(*currentNode, m_bestResultFound);
    writeSummaryToQueue();
    updateNodeVisits(true, true);
}

std::shared_ptr<Edge> CombatSearch_Integral_MCTS::getPromisingEdge(const std::shared_ptr<Node> & node, BuildOrderIntegral & buildOrderIntegral)
{    
    Edge * promisingEdge = nullptr;
    Node * promisingNode = &*node;

    while(promisingNode != nullptr && m_nodeVisits < m_params.getNumberOfNodes()
        && promisingNode->numChildren() > 0)
    {
        // select the edge with the highest UCT value
        promisingEdge = &*promisingNode->getPromisingChild(m_params, buildOrderIntegral.integral.getCurrentStackValue());
        promisingNode = &*promisingEdge->getChild();

        // update build order and integral
        updateBOIntegral(promisingNode->getState(), promisingEdge->getAction(), buildOrderIntegral, promisingNode->isTerminal());

        ++m_nodeVisits;
    }
    return promisingEdge->shared_from_this();
}

void CombatSearch_Integral_MCTS::rollout(GameState & state, BuildOrderIntegral & buildOrderIntegral, bool terminal)
{
    // generate possible actions to check if state is terminal
    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);

    // terminal node if no action is possible
    if (legalActions.size() == 0)
    {
        state.fastForward(m_params.getFrameTimeLimit());
        buildOrderIntegral.integral.update(state, buildOrderIntegral.buildOrder, m_params, m_searchTimer, false);
        buildOrderIntegral.integral.setState(state);
        ++m_leafNodesVisited;
        return;
    }

    // mixing value of 1 means we only use network prediction
    if (m_params.getMixingValue() == 1)
    {
        buildOrderIntegral.integral.setState(state);
        return;
    }

    bool leafNode = terminal;
    // do a rollout
    while (!leafNode && m_nodeVisits < m_params.getNumberOfNodes())
    {
        leafNode = doRandomAction(state, buildOrderIntegral);
    }
}

bool CombatSearch_Integral_MCTS::doRandomAction(GameState& state, BuildOrderIntegral& buildOrderIntegral)
{
    // generate possible actions
    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);

    // do an action at random
    if (legalActions.size() > 0)
    {
        std::uniform_int_distribution<> distribution(0, legalActions.size() - 1);
        int index = distribution(m_rnggen);
        Action action = legalActions[index];

        if (action.first.isAbility())
        {
            state.doAbility(action.first, action.second);
            updateBOIntegral(state, ActionAbilityPair(action.first, state.getLastAbility()), buildOrderIntegral, false);
        }
        else
        {
            state.doAction(action.first);
            updateBOIntegral(state, ActionAbilityPair(action.first, AbilityAction()), buildOrderIntegral, false);
        }
        ++m_nodeVisits;
        return false;
    }

    updateIntegralTerminal(state, buildOrderIntegral);
    ++m_nodeVisits;
    ++m_leafNodesVisited;
    return true;
}

bool CombatSearch_Integral_MCTS::timeLimitReached()
{
    if (m_params.getUseTotalTimeLimit())
    {
        return (m_params.getSearchTimeLimit() && (m_numTotalSimulations % 5 == 0) && boost::chrono::duration_cast<boost::chrono::duration<double, boost::milli>>(boost::chrono::thread_clock::now() - m_searchTimerCPU).count() > m_params.getSearchTimeLimit());
    }
    
    return (m_params.getSearchTimeLimit() && (m_numTotalSimulations % 5 == 0) && (m_searchTimer.getElapsedTimeInMilliSec() > m_params.getSearchTimeLimit()));
}

void CombatSearch_Integral_MCTS::updateIntegralTerminal(const GameState & state, BuildOrderIntegral & buildOrderIntegral)
{
    GameState stateCopy(state);
    stateCopy.fastForward(m_params.getFrameTimeLimit());
    buildOrderIntegral.integral.update(stateCopy, buildOrderIntegral.buildOrder, m_params, m_searchTimer, false);
    buildOrderIntegral.integral.setState(stateCopy);
}

void CombatSearch_Integral_MCTS::updateBOIntegral(const GameState & state, const ActionAbilityPair & action, BuildOrderIntegral & buildOrderIntegral, bool terminal)
{
    // if the node is terminal, it means it doesn't have any children. we add the action
    // that took us to this node, fast forward to the frame limit, and update the integral
    if (terminal)
    {
        GameState stateCopy(state);
        // fast forward to the end of the timelimit
        stateCopy.fastForward(m_params.getFrameTimeLimit());

        buildOrderIntegral.buildOrder.add(action);
        buildOrderIntegral.integral.update(stateCopy, buildOrderIntegral.buildOrder, m_params, m_searchTimer, false);
        buildOrderIntegral.integral.setState(stateCopy);
    }

    // add the action and calculate the integral
    else
    {
        buildOrderIntegral.buildOrder.add(action);
        buildOrderIntegral.integral.update(state, buildOrderIntegral.buildOrder, m_params, m_searchTimer, false);
    }
}

void CombatSearch_Integral_MCTS::backPropagation(const std::shared_ptr<Edge> & edge, const BuildOrderIntegral & buildOrderIntegral)
{
    Edge* nextEdge = &*edge;
    Node* nextNode = &*edge->getParent();

    FracType simulationValue = buildOrderIntegral.integral.getCurrentStackValue();
    FracType networkValue = edge->getNetworkValue();

    // update edges
    while (nextNode != nullptr)
    {
        nextEdge->updateEdge(simulationValue, networkValue);
        nextEdge = &*nextNode->getParent();
        nextNode = &*nextEdge->getParent();
    }

    // write to log file one thread at a time
    if (buildOrderIntegral.integral.getCurrentStackValue() > m_bestResultFound.integral.getCurrentStackValue() ||
        ((buildOrderIntegral.integral.getCurrentStackValue() == m_bestResultFound.integral.getCurrentStackValue())
            && Eval::StateBetter(buildOrderIntegral.integral.getState(), m_bestResultFound.integral.getState())))
    {
        std::scoped_lock sl(m_resultFileMutex);
        if (buildOrderIntegral.integral.getCurrentStackValue() > m_bestResultFound.integral.getCurrentStackValue() ||
            ((buildOrderIntegral.integral.getCurrentStackValue() == m_bestResultFound.integral.getCurrentStackValue())
                && Eval::StateBetter(buildOrderIntegral.integral.getState(), m_bestResultFound.integral.getState())))
        {
            m_bestResultFound = buildOrderIntegral;
            writeSummaryToQueue();
            FracType val = (Edge::MIXING_VALUE * networkValue) + ((1 - Edge::MIXING_VALUE) * buildOrderIntegral.integral.getCurrentStackValue());
            if (val > Edge::HIGHEST_VALUE_FOUND)
            {
                Edge::HIGHEST_VALUE_FOUND = val;
            }
        }
    }
}

bool CombatSearch_Integral_MCTS::changeRoot(int threadID)
{
    // All threads wait for thread 0 to change the root
    if (threadID != 0)
    {
        std::unique_lock<std::mutex> ul(m_changeRootMutex);

        ++m_threadsWaitingForChangeRoot;
        m_changeRootCompleted.wait(ul, [this] {return m_rootChanged != ThreadMessage::RootNotChanged; });
        --m_threadsWaitingForChangeRoot;

        // reset bool so threads wait to change root next time as well
        if (m_threadsWaitingForChangeRoot == 0 && m_rootChanged == ThreadMessage::RootChanged)
        {
            m_rootChanged = ThreadMessage::RootNotChanged;
        }
        //ul.unlock();
        //m_changeRootCompleted.notify_one();

        // search is finished, so the thread needs to exit the loop
        if (m_rootChanged == ThreadMessage::SearchFinished)
        {
            return false;
        }
    }

    else
    {
        // wait until all the threads finish fixing the values for extra simulations
        while (m_threadsWaitingForChangeRoot != m_params.getThreadsForMCTS() - 1);
        //std::cout << m_threadsWaitingForChangeRoot << std::endl;
        {
            std::lock_guard<std::mutex> lg(m_changeRootMutex);
            BOSS_ASSERT(m_numCurrentRootSimulations == m_simulationsPerRoot,
                "Number of simulations for a root %i must equal to the limit of simulations per root %i", m_numCurrentRootSimulations.load(), m_simulationsPerRoot);

            //std::cout << "simulations before root change: " << simulations << std::endl;

            BOSS_ASSERT(!m_currentRoot->isTerminal(), "Root should never be terminal when reaching this point");

            // error checking
            m_currentRoot->changeRootCheck(m_params, m_numCurrentRootSimulations);
            
            // write state data
            if (m_params.getSaveStates())
            {
                writeRootData(m_buildOrderIntegralChangedRoot);
            }

            m_numCurrentRootSimulations = 0;
            //m_simulationsPerStep = (int)round(m_simulationsPerStep * m_params.getSimulationsPerStepDecay());

            bool bestResultFoundChange = false;
            std::shared_ptr<Edge> childEdge;
            // take the highest value child, but if it has lower value than the best found, we take the
            // action in the best found build order instead
            if (m_currentRoot->getState().getCurrentFrame() >= m_params.getTemperatureChange())
            {
                if (m_bestResultFound.buildOrder.size() > m_buildOrderIntegralChangedRoot.buildOrder.size())
                {
                    childEdge = m_currentRoot->getChild(m_bestResultFound.buildOrder[m_buildOrderIntegralChangedRoot.buildOrder.size()]);
                }
                else
                {
                    childEdge = m_currentRoot->getHighestValueChild(m_params);
                    bestResultFoundChange = true;
                }
                if (m_params.getMixingValue() < 1)
                {
                    BOSS_ASSERT(childEdge->getMax() <= m_bestResultFound.integral.getCurrentStackValue(), "Value of a node can't be higher than the best build order found");
                }
            }

            else
            {
                childEdge = m_currentRoot->getChildProportionalToVisitCount(m_params, m_rnggen);
                if (m_bestResultFound.buildOrder.size() > m_buildOrderIntegralChangedRoot.buildOrder.size())
                {
                    if (childEdge->getAction() != m_bestResultFound.buildOrder[m_buildOrderIntegralChangedRoot.buildOrder.size()])
                    {
                        bestResultFoundChange = true;
                    }
                }
                else
                {
                    bestResultFoundChange = true;
                }
            }

            //std::cout << "taking action: " << childEdge->getAction().first.getName() << std::endl;

            // create the child node if it doesn't exist
            if (childEdge->getChild() == nullptr)
            {
                std::shared_ptr<Node> temp = m_currentRoot->notExpandedChild(*childEdge, m_params, m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue(), true);
                updateNodeVisits(true, m_currentRoot->isTerminal());
            }
            else
            {
                updateNodeVisits(false, m_currentRoot->isTerminal());
            }
            BOSS_ASSERT(childEdge->getChild() != nullptr, "currentRoot has become null");

            // we have made a choice, so we need to update the integral and build order permanently
            updateBOIntegral(*(childEdge->getChild()), childEdge->getAction(), m_buildOrderIntegralChangedRoot);
            if (bestResultFoundChange)
            {
                m_bestResultFound = m_buildOrderIntegralChangedRoot;
            }

            // remove nodes and edges that are unreachable. reset the parent edge of the new root
            m_currentRoot->removeEdges(*childEdge);
            m_currentRoot = childEdge->getChild();
            if (m_params.getChangingRootReset())
            {
                m_currentRoot->cleanUp(m_params.getThreadsForMCTS());
                m_currentRoot->getParentEdge()->reset();
                // no noise if we are evaluating. We are evaluating if the temperature change frame is 0
                if (m_params.getTemperatureChange() == 0)
                {
                    m_currentRoot->createChildrenEdges(m_params, m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue());
                }
                else
                {
                    m_currentRoot->createChildrenEdges(m_params, m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue(), true, m_gsl_r);
                }
            }
            else if (m_currentRoot->getNumEdges() > 0)
            {
                m_currentRoot->addNoise(m_gsl_r);
            }

            // there might be no edges for this new root
            if (m_currentRoot->getNumEdges() == 0)
            {
                // no noise if we are evaluating. We are evaluating if the temperature change frame is 0
                if (m_params.getTemperatureChange() == 0)
                {
                    m_currentRoot->createChildrenEdges(m_params, m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue());
                }
                else
                {
                    m_currentRoot->createChildrenEdges(m_params, m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue(), true, m_gsl_r);
                }
            }

            // search is over
            if (m_currentRoot->isTerminal() || m_numTotalSimulations >= m_params.getNumberOfSimulations()
                || m_nodeVisits >= m_params.getNumberOfNodes())
            {
                // creating the children showed us this state is terminal, so we need to do another update
                if (m_currentRoot->isTerminal())
                {
                    updateIntegralTerminal(*m_currentRoot, m_buildOrderIntegralChangedRoot);
                    m_bestResultFound = m_buildOrderIntegralChangedRoot;
                    // write state data of terminal node
                    if (m_params.getSaveStates())
                    {
                        writeRootData(m_buildOrderIntegralChangedRoot);
                    }
                }
                m_rootChanged = ThreadMessage::SearchFinished;
                m_changeRootCompleted.notify_all();
                return false;
            }

            // wake up the other threads
            m_rootChanged = ThreadMessage::RootChanged;
        }
        m_changeRootCompleted.notify_all();
    }

    return true;
}

void CombatSearch_Integral_MCTS::updateNodeVisits(bool nodeExpanded, bool isTerminal)
{
    ++m_nodeVisits;
    if (nodeExpanded)
    {
        ++m_nodesExpanded;
    }  
    if (isTerminal)
    {
        ++m_leafNodesVisited;
    }
    if (nodeExpanded && isTerminal)
    {
        ++m_leafNodesExpanded;
    }
}

void CombatSearch_Integral_MCTS::writeSummaryToQueue()
{
    m_resultLog.push_back(ResultLog(m_nodesExpanded, m_nodeVisits, m_leafNodesExpanded,
        m_leafNodesVisited, m_results.searchTimer.getElapsedTimeInMilliSec() / 1000,
        m_numTotalSimulations, m_bestResultFound.buildOrder.getNameString(),
        m_bestResultFound.integral.getCurrentStackEval(),
        m_bestResultFound.integral.getCurrentStackValue()));
}

void CombatSearch_Integral_MCTS::writeRootData(const BuildOrderIntegral & rootIntegral)
{
    BOSS_ASSERT(!m_currentRoot->isTerminal() || m_currentRoot->numChildren() == 0, "Number of edges of the root must be greater than 0 if it's not a terminal node");

    /*std::cout << "integral value: " << m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue() << std::endl;
    std::cout << "children:" << std::endl;
    m_currentRoot->printChildren();
    std::cout << std::endl;*/

    StateData stateData;
    stateData.state = m_currentRoot->getState().getStateData(m_params,
        m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue(), m_currentRoot->getChronoboostTargets());

    std::vector<float> MCTSPolicy = std::vector<float>(ActionTypes::GetRaceActionCount(Races::Protoss), 0.f);
    FracType maxValue = -1;

    if (m_currentRoot->getNumEdges() > 0)
    {
        if (m_bestResultFound.buildOrder.size() > m_buildOrderIntegralChangedRoot.buildOrder.size())
        {
            const auto& edge = m_currentRoot->getChild(m_bestResultFound.buildOrder[m_buildOrderIntegralChangedRoot.buildOrder.size()]);
            if (m_params.useSimulationValueOnly())
            {
                maxValue = edge.getMax();
            }
            else
            {
                maxValue = edge.getValue();
            }
            MCTSPolicy[edge.getAction().first.getRaceActionID()] = 1;
        }
        else
        {
            const auto& edge = m_currentRoot->getHighestValueChild(m_params);
            if (m_params.useSimulationValueOnly())
            {
                maxValue = edge.getMax();
            }
            else
            {
                maxValue = edge.getValue();
            }
            MCTSPolicy[edge.getAction().first.getRaceActionID()] = 1;
        }
    }

    // terminal node, so we give probability 1 to the None action for policy and value of the node
    // is the value of the build order
    else
    {
        //maxIndices.push_back(0);
        MCTSPolicy[0] = 1.0f;
        maxValue = m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue();
    }

    // AlphaZero Target normalize values
    /*for (int i = 0; i < m_currentRoot->getNumEdges(); ++i)
    {
        MCTSPolicy[m_currentRoot->getEdge(i).getAction().first.getRaceActionID()] /= totalVisits;
    }*/

    // policy
    std::stringstream policy;
    policy.precision(4);
    for (int i = 0; i < ActionTypes::GetRaceActionCount(Races::Protoss); ++i)
    {
        policy << MCTSPolicy[i];

        if (i != ActionTypes::GetRaceActionCount(Races::Protoss) - 1)
        {
            policy << ",";
        }
    }
    stateData.policy = policy.str();

    //std::cout << policy.str() << std::endl;

    // write the value of state
    stateData.stateValue = rootIntegral.integral.getCurrentStackValue();
    stateData.highestFoundValue = maxValue;

    m_stateData.push_back(stateData);
}

void CombatSearch_Integral_MCTS::printResults()
{
    m_bestResultFound.integral.print(m_bestResultFound.buildOrder);
    std::cout << "\nRan " << m_numTotalSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000 * double(m_numTotalSimulations / m_results.timeElapsed)) << " simulations/sec\n";
    std::cout << "Nodes expanded: " << m_results.nodesExpanded << ". Total nodes visited: " << m_results.nodeVisits << ", at a rate of " << (1000 * m_results.nodeVisits / m_results.timeElapsed) << " nodes/sec\n";
}

#include "BuildOrderPlotter.h"
void CombatSearch_Integral_MCTS::writeResultsFile(const std::string & dir, const std::string & filename)
{
    /*BuildOrderPlotter plot;
    plot.setOutputDir(dir);
    plot.addPlot(filename, m_params.getInitialState(), m_bestBuildOrderFound);
    plot.doPlots();
*/
    m_bestResultFound.integral.writeToFile(dir, filename);

    std::ofstream boFile(dir + "/" + filename + "_BuildOrder.txt", std::ofstream::out | std::ofstream::app);
    boFile << "\nRan " << m_numTotalSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000 * m_numTotalSimulations / m_results.timeElapsed) << " simulations/sec" << std::endl;
    boFile << "Nodes expanded: " << m_results.nodesExpanded << ". Total nodes visited: " << m_results.nodeVisits << ", at a rate of " << (1000 * m_results.nodeVisits / m_results.timeElapsed) << " nodes/sec\n";

    // Write search data to file 
    std::stringstream ss;
    for (auto & log : m_resultLog)
    {
        log.writeToSS(ss);
    }
    std::ofstream file(m_dir + "/" + m_prefix + "_Results.csv", std::ofstream::out | std::ofstream::app);
    file << ss.rdbuf();
    ss.str(std::string());
    file.close();

    std::ofstream searchData(m_dir + "/" + m_prefix + "_SearchData.txt", std::ofstream::out | std::ofstream::app);
    searchData << "Max eval found: " << m_bestResultFound.integral.getCurrentStackValue() << "\n";
    searchData << "Finished build order eval: " << m_results.finishedEval << "\n";
    searchData << "Useful build order eval: " << m_results.usefulEval << "\n";
    searchData << "Max value found: " << m_bestResultFound.integral.getCurrentStackEval() << "\n";
    searchData << "Finished build order value: " << m_results.finishedValue << "\n";
    searchData << "Useful build order value: " << m_results.usefulValue << "\n";
    searchData << "Best build order (all): " << m_bestResultFound.buildOrder.getNameString() << std::endl;
    searchData << "Best build order (finished): " << m_results.finishedUnitsBuildOrder.getNameString(0, -1, true) << std::endl;
    searchData << "Best build order (useful): " << m_results.usefulBuildOrder.getNameString(0, -1, true) << std::endl;
    searchData << "Nodes expanded: " << m_results.nodesExpanded << "\n";
    searchData << "Nodes traversed: " << m_results.nodeVisits << "\n";
    searchData << "Leaf nodes expanded: " << m_results.leafNodesExpanded << "\n";
    searchData << "Leaf nodes traversed: " << m_results.leafNodesVisited << "\n";
    searchData << "Search real time in ms: " << m_results.timeElapsed << "\n";
    searchData << "Search CPU time in ms: " << m_results.timeElapsedCPU << "\n";
    searchData << "Simulations: " << m_numTotalSimulations;
    searchData.close();
}