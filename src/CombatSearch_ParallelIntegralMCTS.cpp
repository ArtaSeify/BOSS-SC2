#include "CombatSearch_ParallelIntegralMCTS.h"
#include "FileTools.h"

#include <random>
#include <future>

using namespace BOSS;

FracType CombatSearch_ParallelIntegralMCTS::exploration_parameter;
std::mt19937 CombatSearch_ParallelIntegralMCTS::rnggen;
gsl_rng* CombatSearch_ParallelIntegralMCTS::gsl_rng;

CombatSearch_ParallelIntegralMCTS::CombatSearch_ParallelIntegralMCTS(const CombatSearchParameters p, const std::string& dir,
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
    exploration_parameter = p.getExplorationValue();
    m_params = p;
    
    if (m_params.getChangingRoot())
    {
        m_simulationsPerRoot = p.getSimulationsPerStep();
    }

    m_dir = dir;
    m_prefix = prefix;
    m_name = name;

    std::random_device rd; // obtain a random number from hardware
    rnggen.seed(rd());
        
    const gsl_rng_type * T;
    gsl_rng_env_setup();

    T = gsl_rng_default;
    gsl_rng = gsl_rng_alloc(T);
}

CombatSearch_ParallelIntegralMCTS::~CombatSearch_ParallelIntegralMCTS()
{
    if (m_params.getSaveStates())
    {
        FileTools::MakeDirectory(CONSTANTS::ExecutablePath + "/SavedStates");
        //std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_prefix + "_" + std::to_string(m_filesWritten) + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
        std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
        #pragma omp critical (WriteState)
        fileStates << m_ssStates.rdbuf();
        m_ssStates.str(std::string());
    }
    gsl_rng_free(gsl_rng);
}

void CombatSearch_ParallelIntegralMCTS::recurse(const GameState& state, int depth)
{
    m_currentRoot = std::make_shared<Node>(state, *std::make_shared<Edge>());
    // no noise if we are evaluating. We are evaluating if the temperature change frame is 0
    if (m_params.getTemperatureChange() == 0)
    {
        m_currentRoot->createChildrenEdges(m_params, 0, false);
    }
    else
    {
        m_currentRoot->createChildrenEdges(m_params, 0, true);
    }

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
            threads[thread] = std::async(std::launch::async, &CombatSearch_ParallelIntegralMCTS::MCTSSearch, this, thread);
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
    CombatSearch_IntegralDataFinishedUnits finishedUnitsIntegral;
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
    CombatSearch_IntegralDataFinishedUnits usefulUnitsIntegral;
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

    m_currentRoot->cleanUp(m_params.getThreadsForMCTS());
}

void CombatSearch_ParallelIntegralMCTS::MCTSSearch(int threadID)
{
    while (!timeLimitReached() && m_numTotalSimulations < m_params.getNumberOfSimulations() && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        // change the root of the tree. Remove all the nodes and edges that are now irrelevant
        if (m_params.getChangingRoot() && m_numCurrentRootSimulations >= m_simulationsPerRoot)
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
                    break;
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

                    // reached a leaf node, we are done
                    if (m_currentRoot->isTerminal())
                    {
                        m_rootChanged = ThreadMessage::SearchFinished;
                        m_changeRootCompleted.notify_all();
                        break;
                    }

                    // error checking
                    int timesVisited = 0;
                    for (int i = 0; i < m_currentRoot->getNumEdges(); ++i)
                    {
                        BOSS_ASSERT(m_currentRoot->getEdge(i).virtualLoss() == 0, "Virtual loss should be 0 when changing root");
                        timesVisited += m_currentRoot->getEdge(i).realTimesVisited();
                    }

                    BOSS_ASSERT((!m_params.getChangingRootReset() && timesVisited >= m_simulationsPerRoot) || timesVisited == m_numCurrentRootSimulations,
                        "The number of times the edges are visited %i should equal to the number of simulations before changing the root %i", timesVisited, m_numCurrentRootSimulations.load());

                    // write state data
                    if (m_params.getSaveStates())
                    {
                        m_ssStates << m_currentRoot->getState().writeToSS(m_params,
                            m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue(), m_currentRoot->getChronoboostTargets());

                        std::vector<float> MCTSPolicy = std::vector<float>(ActionTypes::GetRaceActionCount(Races::Protoss), 0.f);
                        int totalVisits = 0;
                        FracType maxValue = 0;
                        // policy is edge_i visit count / all edges visit count
                        for (int i = 0; i < m_currentRoot->getNumEdges(); ++i)
                        {
                            const auto& edge = m_currentRoot->getEdge(i);
                            //std::cout << "edge: " << edge->getAction().first.getName() << ", id: " << edge->getAction().first.getRaceActionID() << std::endl;
                            //std::cout << "visits: " << edge->timesVisited() << std::endl;
                            MCTSPolicy[edge.getAction().first.getRaceActionID()] = static_cast<float>(edge.realTimesVisited());
                            totalVisits += edge.realTimesVisited();
                            maxValue = std::max(maxValue, edge.getMax());
                        }
                        //std::cout << std::endl;
                        // write the policy values
                        for (int i = 0; i < ActionTypes::GetRaceActionCount(Races::Protoss); ++i)
                        {
                            m_ssStates << "," << (MCTSPolicy[i] / totalVisits);
                        }
                        // write the value of state
                        //std::cout << Edge::MAX_EDGE_VALUE_EXPECTED << std::endl;
                        //std::cout << maxValue << std::endl;
                        //std::cout << maxValue / Edge::MAX_EDGE_VALUE_EXPECTED << std::endl;
                        m_ssStates << "," << maxValue / Edge::MAX_EDGE_VALUE_EXPECTED;

                        m_ssStates << "\n";
                    }

                    m_numCurrentRootSimulations = 0;
                    //m_simulationsPerStep = (int)round(m_simulationsPerStep * m_params.getSimulationsPerStepDecay());

                    std::shared_ptr<Edge> childEdge;
                    // take the highest value child, but if it has lower value than the best found, we take the
                    // action in the best found build order instead
                    if (!m_params.getChangingRootReset() || m_currentRoot->getState().getCurrentFrame() >= m_params.getTemperatureChange())
                    {
                        if (!m_params.getChangingRootReset())
                        {
                            
                            childEdge = m_currentRoot->getChild(m_bestResultFound.buildOrder[m_buildOrderIntegralChangedRoot.buildOrder.size()]).shared_from_this();
                        }
                        else
                        {
                            childEdge = m_currentRoot->getHighestValueChild(m_params).shared_from_this();
                            BOSS_ASSERT(childEdge->getMax() <= m_bestResultFound.integral.getCurrentStackValue(), "Value of a node can't be higher than the best build order found");
                        }
                    }

                    else
                    {
                        childEdge = m_currentRoot->getChildProportionalToVisitCount(m_params).shared_from_this();
                        if (childEdge->getAction() != m_bestResultFound.buildOrder[m_buildOrderIntegralChangedRoot.buildOrder.size()])
                        {
                            m_bestResultFound = m_buildOrderIntegralChangedRoot;
                        }
                    }

                    //std::cout << "taking action: " << childEdge->getAction().first.getName() << std::endl;

                    // create the child node if it doesn't exist
                    if (childEdge->getChild() == nullptr)
                    {
                        m_currentRoot->notExpandedChild(*childEdge, m_params, m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue(), true);
                        updateNodeVisits(true, m_currentRoot->isTerminal());
                    }
                    else
                    {
                        updateNodeVisits(false, m_currentRoot->isTerminal());
                    }
                    BOSS_ASSERT(childEdge->getChild() != nullptr, "currentRoot has become null");

                    // we have made a choice, so we need to update the integral and build order permanently
                    updateBOIntegral(*(childEdge->getChild()), childEdge->getAction(), m_buildOrderIntegralChangedRoot);

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
                            m_currentRoot->createChildrenEdges(m_params, m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue(), false);
                        }
                        else
                        {
                            m_currentRoot->createChildrenEdges(m_params, m_buildOrderIntegralChangedRoot.integral.getCurrentStackValue(), true);
                        }
                    }

                    // search is over
                    if (m_currentRoot->isTerminal() || m_numTotalSimulations >= m_params.getNumberOfSimulations()
                        || m_nodeVisits >= m_params.getNumberOfNodes())
                    {
                        m_rootChanged = ThreadMessage::SearchFinished;
                        m_changeRootCompleted.notify_all();
                        break;
                    }

                    // wake up the other threads
                    m_rootChanged = ThreadMessage::RootChanged;
                }
                m_changeRootCompleted.notify_all();
            }
        }
        

        if (m_numTotalSimulations < m_params.getNumberOfSimulations() && m_numCurrentRootSimulations < m_simulationsPerRoot)
        {
            // we increase the simulation count at the beginning to avoid doing extra simulations due to multithreading
            int thisThreadSims = ++m_numTotalSimulations;
            int thisThreadCRSims = ++m_numCurrentRootSimulations;

            // reached the limit, decrease the count
            if (thisThreadSims > m_params.getNumberOfSimulations() || thisThreadCRSims > m_simulationsPerRoot)
            {
                --m_numTotalSimulations;
                --m_numCurrentRootSimulations;
            }

            else
            {
                //std::cout << "have run : " << m_numTotalSimulations << " simulations thus far." << std::endl;
                
                BuildOrderIntegral buildOrderIntegral(m_buildOrderIntegralChangedRoot);

                std::shared_ptr<Node> promisingNode = getPromisingNode(*m_currentRoot, buildOrderIntegral);

                // we have reached node limit 
                if (m_nodeVisits > m_params.getNumberOfNodes())
                {
                    break;
                }

                randomPlayout(*promisingNode, buildOrderIntegral);

                // we have reached node limit 
                if (m_nodeVisits > m_params.getNumberOfNodes())
                {
                    break;
                }

                BOSS_ASSERT(thisThreadSims <= m_params.getNumberOfSimulations(), "Total simulations done: %i, limit: %i", thisThreadSims, m_params.getNumberOfSimulations());
                BOSS_ASSERT(thisThreadCRSims <= m_simulationsPerRoot, "Current root simulations done: %i, limit: %i", thisThreadCRSims, m_simulationsPerRoot);

                backPropogation(*promisingNode, buildOrderIntegral);
            }
        }
    }
}

void CombatSearch_ParallelIntegralMCTS::evaluatePolicyNetwork()
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

std::shared_ptr<Node> CombatSearch_ParallelIntegralMCTS::getPromisingNode(Node & node, BuildOrderIntegral & buildOrderIntegral)
{    
    std::shared_ptr<Node> currentNode = node.shared_from_this();

    while(!currentNode->isTerminal() && m_nodeVisits < m_params.getNumberOfNodes())
    {
        // select the edge with the highest UCT value
        Edge & edge = currentNode->selectChildEdge(m_params);

        // the node doesn't exist in memory
        if (edge.getChild() == nullptr)
        {
            currentNode = currentNode->notExpandedChild(edge, m_params, buildOrderIntegral.integral.getCurrentStackValue(), false);
            updateNodeVisits(edge.getChild() != nullptr, currentNode->isTerminal());
            updateBOIntegral(*currentNode, edge.getAction(), buildOrderIntegral);
            return currentNode;
        }

        // the node exists and is pointed to by the edge
        currentNode = edge.getChild();
        updateNodeVisits(false, currentNode->isTerminal());

        // update build order and integral
        updateBOIntegral(*currentNode, edge.getAction(), buildOrderIntegral);

        // create child edges if an expanded node doesn't have any edges. This happens when
        // we are not using a value network
        if (currentNode->getNumEdges() == 0 && !m_params.usePolicyValueNetwork())
        {
            currentNode->createChildrenEdgesSecondVisit(m_params, buildOrderIntegral.integral.getCurrentStackValue());
        }
    }

    BOSS_ASSERT(currentNode->isTerminal(), "This shouldn't happen if the node isn't terminal");
    return currentNode;
}

void CombatSearch_ParallelIntegralMCTS::randomPlayout(Node node, BuildOrderIntegral & buildOrderIntegral)
{
    // mixing value of 1 means we only use network prediction
    if (m_params.getMixingValue() == 1)
    {
        if (node.isTerminal())
        {
            GameState stateCopy(node.getState());
            stateCopy.fastForward(m_params.getFrameTimeLimit());
            buildOrderIntegral.integral.update(stateCopy, buildOrderIntegral.buildOrder, m_params, m_searchTimer, false);
            buildOrderIntegral.integral.setState(stateCopy);
        }
        else
        {
            buildOrderIntegral.integral.setState(node.getState());
        }
        return;
    }

    bool leafNode = node.isTerminal();
    // do a rollout
    while (!leafNode && m_nodeVisits < m_params.getNumberOfNodes())
    {
        leafNode = doRandomAction(node, buildOrderIntegral);
    }
}

bool CombatSearch_ParallelIntegralMCTS::doRandomAction(Node & currNode, BuildOrderIntegral& buildOrderIntegral)
{
    // generate possible actions
    ActionSetAbilities legalActions;
    generateLegalActions(currNode.getState(), legalActions, m_params);

    // do an action at random
    if (legalActions.size() > 0)
    {
        std::uniform_int_distribution<> distribution(0, legalActions.size() - 1);
        int index = distribution(rnggen);
        Action action = legalActions[index];
        currNode.doAction(action, m_params);

        if (action.first.isAbility())
        {
            updateBOIntegral(currNode, ActionAbilityPair(action.first, currNode.getState().getLastAbility()), buildOrderIntegral);
        }
        else
        {
            updateBOIntegral(currNode, ActionAbilityPair(action.first, AbilityAction()), buildOrderIntegral);
        }
        updateNodeVisits(false, false);
        return false;
    }

    updateIntegralTerminal(currNode, buildOrderIntegral);
    updateNodeVisits(false, true);
    return true;
}

bool CombatSearch_ParallelIntegralMCTS::timeLimitReached()
{
    if (m_params.getUseTotalTimeLimit())
    {
        return (m_params.getSearchTimeLimit() && (m_numTotalSimulations % 5 == 0) && boost::chrono::duration_cast<boost::chrono::duration<double, boost::milli>>(boost::chrono::thread_clock::now() - m_searchTimerCPU).count() > m_params.getSearchTimeLimit());
    }
    
    return (m_params.getSearchTimeLimit() && (m_numTotalSimulations % 5 == 0) && (m_searchTimer.getElapsedTimeInMilliSec() > m_params.getSearchTimeLimit()));
}

void CombatSearch_ParallelIntegralMCTS::updateIntegralTerminal(const Node & currNode, BuildOrderIntegral & buildOrderIntegral)
{
    GameState stateCopy(currNode.getState());
    stateCopy.fastForward(m_params.getFrameTimeLimit());
    buildOrderIntegral.integral.update(stateCopy, buildOrderIntegral.buildOrder, m_params, m_searchTimer, false);
    buildOrderIntegral.integral.setState(stateCopy);
}

void CombatSearch_ParallelIntegralMCTS::updateBOIntegral(const Node & currNode, const ActionAbilityPair & action, BuildOrderIntegral & buildOrderIntegral)
{
    // if the node is terminal, it means it doesn't have any children. we add the action
    // that took us to this node, fast forward to the frame limit, and update the integral
    if (currNode.isTerminal())
    {
        GameState stateCopy(currNode.getState());
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
        buildOrderIntegral.integral.update(currNode.getState(), buildOrderIntegral.buildOrder, m_params, m_searchTimer, false);
    }
}

void CombatSearch_ParallelIntegralMCTS::backPropogation(Node & node, const BuildOrderIntegral & buildOrderIntegral)
{
    std::shared_ptr<Edge> parentEdge = node.shared_from_this()->getParentEdge();
    FracType simulationValue = buildOrderIntegral.integral.getCurrentStackValue();
    FracType networkValue = parentEdge->getNetworkValue();

    // update edges
    while (parentEdge->getParent() != nullptr)
    {
        parentEdge->updateEdge(simulationValue, networkValue);
        parentEdge = parentEdge->getParent()->getParentEdge();
    }

    // write to log file one thread at a time
    m_resultFileMutex.lock();
    if (buildOrderIntegral.integral.getCurrentStackValue() > m_bestResultFound.integral.getCurrentStackValue() ||
        ((buildOrderIntegral.integral.getCurrentStackValue() == m_bestResultFound.integral.getCurrentStackValue())
            && Eval::StateBetter(buildOrderIntegral.integral.getState(), m_bestResultFound.integral.getState())))
    {
        m_bestResultFound = buildOrderIntegral;
        writeSummaryToQueue();
    }
    m_resultFileMutex.unlock();
}

void CombatSearch_ParallelIntegralMCTS::fixEdgeVisits(Node & node)
{
    std::shared_ptr<Edge> parentEdge = node.shared_from_this()->getParentEdge();

    while (parentEdge->getParent() != nullptr)
    {
        parentEdge->decVisits();
        parentEdge = parentEdge->getParent()->getParentEdge();
    }
}

void CombatSearch_ParallelIntegralMCTS::updateNodeVisits(bool nodeExpanded, bool isTerminal)
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

void CombatSearch_ParallelIntegralMCTS::writeSummaryToQueue()
{
    m_resultLog.push_back(ResultLog(m_nodesExpanded, m_nodeVisits, m_leafNodesExpanded,
        m_leafNodesVisited, m_results.searchTimer.getElapsedTimeInMilliSec() / 1000,
        m_numTotalSimulations, m_bestResultFound.buildOrder.getNameString(),
        m_bestResultFound.integral.getCurrentStackEval(),
        m_bestResultFound.integral.getCurrentStackValue()));
}

void CombatSearch_ParallelIntegralMCTS::printResults()
{
    m_bestResultFound.integral.print(m_bestResultFound.buildOrder);
    std::cout << "\nRan " << m_numTotalSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000 * double(m_numTotalSimulations / m_results.timeElapsed)) << " simulations/sec\n";
    std::cout << "Nodes expanded: " << m_results.nodesExpanded << ". Total nodes visited: " << m_results.nodeVisits << ", at a rate of " << (1000 * m_results.nodeVisits / m_results.timeElapsed) << " nodes/sec\n";
}

#include "BuildOrderPlotter.h"
void CombatSearch_ParallelIntegralMCTS::writeResultsFile(const std::string & dir, const std::string & filename)
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