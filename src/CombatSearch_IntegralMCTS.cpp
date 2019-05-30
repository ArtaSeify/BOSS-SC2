#include "CombatSearch_IntegralMCTS.h"
#include "FileTools.h"
#include <random>

using namespace BOSS;

CombatSearch_IntegralMCTS::CombatSearch_IntegralMCTS(const CombatSearchParameters p, const std::string & dir, 
                                                        const std::string & prefix, const std::string & name)
    : m_exploration_parameter (p.getExplorationValue())
    , m_promisingNodeIntegral(CombatSearch_IntegralDataFinishedUnits())
    , m_promisingNodeBuildOrder(BuildOrderAbilities())
    , m_bestIntegralFound(CombatSearch_IntegralDataFinishedUnits())
    , m_bestBuildOrderFound(BuildOrderAbilities())
    , m_rootRewards()
    , m_numTotalSimulations(0)
    , m_numCurrentRootSimulations(0)
    , m_resultsStream()
    , m_needToWriteBestValue(false)
{
    m_params = p;
    Edge::USE_MAX_VALUE = m_params.getUseMaxValue();
    m_simulationsPerStep = m_params.getSimulationsPerStep();

    m_writeEveryKSimulations = 1;
    m_dir = dir;
    m_prefix = prefix;
    m_name = name;
    m_resultsStream << "0,0,0,0,0,0, ,0,0\n";

    std::random_device rd; // obtain a random number from hardware
    m_rnggen.seed(rd());

    /*if (m_params.useNetworkPrediction())
    {
        Edge::MIXING_PARAMETER = 0.5f;
    }
    else
    {
        Edge::MIXING_PARAMETER = 0.0f;
    }*/
}

CombatSearch_IntegralMCTS::~CombatSearch_IntegralMCTS()
{
    if (m_params.getSaveStates())
    {
        FileTools::MakeDirectory(CONSTANTS::ExecutablePath + "/SavedStates");
        //std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_prefix + "_" + std::to_string(m_filesWritten) + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
        std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
        #pragma omp critical
        fileStates << m_ssStates.rdbuf();
        m_ssStates.str(std::string());
    }
}

void CombatSearch_IntegralMCTS::recurse(const GameState& state, int depth)
{
    m_numTotalSimulations = 0;
    m_numCurrentRootSimulations = 0;
    int simulationsWritten = 0;
    int rootDepth = 0;

    std::shared_ptr<Node> root = std::make_shared<Node>(state);
    std::shared_ptr<Edge> root_parent = std::make_shared<Edge>();
    root->setParentEdge(root_parent);
    std::shared_ptr<Node> currentRoot = root;

    while (!timeLimitReached() && m_numTotalSimulations < m_params.getNumberOfSimulations() && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        // change the root of the tree. Remove all the nodes and edges that are now irrelevant
        if (m_params.getChangingRoot() && m_numTotalSimulations > 0 && m_numCurrentRootSimulations == m_simulationsPerStep)
        {
            //std::cout << "simulations before root change: " << m_numCurrentRootSimulations << std::endl;
            // reached a leaf node, we are done
            if (currentRoot->getNumEdges() == 0)
            {
                break;
            }

            // write state data
            if (m_params.getSaveStates())
            {
                currentRoot->getState().writeToSS(m_ssStates, m_params);
                std::vector<float> MCTSPolicy = std::vector<float>(ActionTypes::GetRaceActionCount(Races::Protoss), 0.f);
                int totalVisits = 0;
                // policy is edge_i visit count / all edges visit count
                for (int i = 0; i < currentRoot->getNumEdges(); ++i)
                {
                    const auto& edge = currentRoot->getEdge(i);
                    MCTSPolicy[edge->getAction().first.getRaceActionID()] = static_cast<float>(edge->timesVisited());
                    totalVisits += edge->timesVisited();
                }
                for (int i = 0; i < ActionTypes::GetRaceActionCount(Races::Protoss); ++i)
                {
                    m_ssStates << "," << (MCTSPolicy[i] / totalVisits);
                }
                m_ssStates << "\n";
            }

            m_numCurrentRootSimulations = 0;

            //m_simulationsPerStep = (int)round(m_simulationsPerStep * m_params.getSimulationsPerStepDecay());

            std::shared_ptr<Edge> childEdge;
            // take the highest value child, but if it has lower value than the best found, we take the
            // action in the best found build order instead
            if (currentRoot->getState().getCurrentFrame() >= m_params.getTemperatureChange())
            {
                childEdge = currentRoot->getHighestValueChild(m_params);
                BOSS_ASSERT(childEdge->getValue() <= m_bestIntegralFound.getCurrentStackValue(), "Value of a node can't be higher than the best build order found");
                if (!m_params.getChangingRootReset() && childEdge->getValue() <= m_bestIntegralFound.getCurrentStackValue())
                {
                    childEdge = currentRoot->getChild(m_bestBuildOrderFound[m_buildOrder.size()]);
                }
            }

            else
            {
                childEdge = currentRoot->getChildProportionalToVisitCount(m_rnggen, m_params);
            }

            // create the child node if it doesn't exist
            if (childEdge->getChild() == nullptr)
            {
                currentRoot->notExpandedChild(childEdge, m_params, true);
            }
            BOSS_ASSERT(childEdge->getChild() != nullptr, "currentRoot has become null");

            // we have made a choice, so we need to update the integral and build order permanently
            updateBOIntegral(*(childEdge->getChild()), childEdge->getAction(), currentRoot->getState(), true);

            currentRoot->removeEdges(childEdge);
            currentRoot = childEdge->getChild();
            if (m_params.getChangingRootReset())
            {
                currentRoot->removeEdges();
                currentRoot->getParentEdge()->reset();
            }
            updateNodeVisits(false, isTerminalNode(*currentRoot));

            ++rootDepth;

            // search is over
            if (isTerminalNode(*currentRoot) || m_numTotalSimulations >= m_params.getNumberOfSimulations())
            {
                break;
            }
        }
        /*if ((m_numTotalSimulations % 1000) == 0)
        {
            std::cout << "have run : " << m_numTotalSimulations << " simulations thus far." << std::endl;
        }*/
        auto nodePair = getPromisingNode(currentRoot);
        // we have reached node limit 
        if (m_results.nodeVisits >= m_params.getNumberOfNodes())
        {
            break;
        }
        std::shared_ptr<Node> promisingNode = nodePair.first;

        // a node that isn't part of the graph yet. We just simulate from this point
        if (nodePair.second)
        {
            randomPlayout(*promisingNode);
        }

        // the node is part of the graph, so we create its edges and pick one at random
        else
        {
            if (!isTerminalNode(*promisingNode))
            {
                ActionSetAbilities legalActions;
                generateLegalActions(promisingNode->getState(), legalActions, m_params);
                promisingNode->createChildrenEdges(legalActions, m_params);

                // there might be no action possible, so createChildrenEdges creates 0 edges
                if (isTerminalNode(*promisingNode))
                {
                    m_results.leafNodesExpanded++;
                    m_results.leafNodesVisited++;
                }
                else
                {
                    //// get a child based on highest network value
                    //if (m_params.useNetworkPrediction())
                    //{
                    //    const GameState& prevNodeState = promisingNode->getState();
                    //    std::shared_ptr<Edge> action = promisingNode->getHighestValueChild(m_params);
                    //    promisingNode = promisingNode->notExpandedChild(action, m_params);
                    //    updateBOIntegral(*promisingNode, action->getAction(), prevNodeState, false);
                    //}
                    //// pick a child at random
                    //else
                    //{
                        const GameState& prevNodeState = promisingNode->getState();
                        std::shared_ptr<Edge> action = promisingNode->getRandomEdge();
                        promisingNode = promisingNode->notExpandedChild(action, m_params);
                        updateBOIntegral(*promisingNode, action->getAction(), prevNodeState, false);
                    //}

                    updateNodeVisits(Edge::NODE_VISITS_BEFORE_EXPAND == 1, isTerminalNode(*promisingNode));
                }
                randomPlayout(*promisingNode);
            }
        }

        if (m_results.nodeVisits < m_params.getNumberOfNodes())
        {
            backPropogation(promisingNode);

            ++m_numTotalSimulations;
            ++m_numCurrentRootSimulations;

            if (m_needToWriteBestValue)
            {
                writeResultsToFile(currentRoot);
            }
        }
    }

    m_integral = m_bestIntegralFound;
    m_buildOrder = m_bestBuildOrderFound;

    BuildOrderAbilities finishedUnitsBuildOrder = createFinishedUnitsBuildOrder(m_bestBuildOrderFound);
    m_results.buildOrder = m_bestBuildOrderFound;
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

    //// write state data
    //if (m_params.getSaveStates())
    //{
    //    std::shared_ptr<Node> currentNode = root;

    //    while (currentNode != currentRoot)
    //    {
    //        currentNode->getState().writeToSS(m_dataStream, m_params);
    //        m_dataStream << "," << m_integral.getCurrentStackValue() << "\n";

    //        currentNode = currentNode->getHighestValueChild(m_params)->getChild();
    //    }
    //    currentNode->getState().writeToSS(m_dataStream, m_params);
    //    m_dataStream << "," << m_integral.getCurrentStackValue() << "\n";
    //}

    // some sanity checks to make sure the result is as expected
    /*if (!timeLimitReached() && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        auto buildOrderAndIntegral = pickBestBuildOrder(root, false);
        BuildOrderAbilities bestBuildOrder = buildOrderAndIntegral.first;
        BOSS_ASSERT(buildOrderAndIntegral.second.getCurrentStackValue() == m_bestIntegralFound.getCurrentStackValue(), "Value of best build order in tree %f must match the value of the best build order found %f", buildOrderAndIntegral.second.getCurrentStackValue(), m_bestIntegralFound.getCurrentStackValue());
        BOSS_ASSERT(m_bestBuildOrderFound.size() == bestBuildOrder.size(), "Best build order in tree must match the best build order found when using max");
        for (int index = 0; index < m_bestBuildOrderFound.size(); ++index)
        {
            BOSS_ASSERT(m_bestBuildOrderFound[index].first == bestBuildOrder[index].first, "Best build order in tree must match the best build order found when using max");
            BOSS_ASSERT(m_bestBuildOrderFound[index].second == bestBuildOrder[index].second, "Best build order in tree must match the best build order found when using max");
        }
    }*/

    for (const auto& pair : m_rootRewards)
    {
        std::ofstream f("../bin/StateDist/" + pair.first + ".txt", std::ofstream::out | std::ofstream::trunc);
        for (auto reward : pair.second)
        {
            f << reward << "\n";
        }
    }

    /*for (int i = 0; i < root->getNumEdges(); ++i)
    {
        auto action = root->getEdge(i);
        std::cout << action->getAction().first.getName() << ": MEAN " << action->getMean() << ", SD: " << action->getSD()
            << ", MAX: " << action->getValue() << ", CONST: " << (action->getValue() - action->getMean()) / action->getSD()
            << " s/sqrt(n): " << action->getSD() / sqrt(action->timesVisited())
            << std::endl;
    }
    std::cout << std::endl;*/

    root->cleanUp();
}

void CombatSearch_IntegralMCTS::test(const GameState & state)
{
//    m_numTotalSimulations = 0;
//    m_numCurrentRootSimulations = 0;
//    int simulationsWritten = 0;
//
//    std::shared_ptr<Node> root = std::make_shared<Node>(state);
//    ActionSetAbilities legalActions;
//    generateLegalActions(root->getState(), legalActions, m_params);
//    root->createChildrenEdges(legalActions, m_params);
//
//    for (int index = 0; index < root->getNumEdges(); ++index)
//    {
//        std::shared_ptr<Edge> action = root->getEdge(index);
//        root->notExpandedChild(action, m_params, true);
//    }
//
//    for (int ind = 0; ind < root->getNumEdges(); ++ind)
//    {
//        std::shared_ptr<Edge> action = root->getEdge(ind);
//        std::shared_ptr<Node> promisingNode = action->getChild();
//
//        std::cout << action->getAction().first.getName() << std::endl;
//
//        for (int sim = 0; sim < 20000; ++sim)
//        {
//            updateBOIntegral(*promisingNode, action->getAction(), root->getState(), false);
//            
//            randomPlayout(*promisingNode);
//            backPropogation(promisingNode);
//
//            m_promisingNodeBuildOrder = m_buildOrder;
//            m_promisingNodeIntegral = m_integral;
//        }
//    }
//    
//    for (const auto& pair : m_rootRewards)
//    {
//        std::ofstream f("../bin/StateDist/" + pair.first + ".txt", std::ofstream::out | std::ofstream::trunc);
//        for (auto reward : pair.second)
//        {
//            f << reward << "\n";
//        }
//    }
//
//    for (int i = 0; i < root->getNumEdges(); ++i)
//    {
//        auto action = root->getEdge(i);
//        std::cout << action->getAction().first.getName() << ": MEAN " << action->getMean() << ", SD: " << action->getSD() 
//            << ", MAX: " << action->getValue() << ", CONST: " << (action->getValue() - action->getMean()) / action->getSD()
//            << " s/sqrt(n): " << action->getSD() / sqrt(action->timesVisited())
//            << std::endl;
//    }
//    std::cout << std::endl;
}

void CombatSearch_IntegralMCTS::test2(const GameState & state)
{
    //std::shared_ptr<Node> root = std::make_shared<Node>(state);

    //ActionSetAbilities legalActions;
    //generateLegalActions(state, legalActions, m_params);
    //root->createChildrenEdges(legalActions, m_params);
    //root->printChildren();

    //std::cout << "root use count: " << root.use_count() << std::endl;

    ////std::shared_ptr<Node> child1 = std::make_shared<Node>(state);
    ////child1->doAction(root->getChild(ActionTypes::GetActionType("Probe")), m_params);

    ////root->cleanUp();
}

std::pair<std::shared_ptr<Node>, bool> CombatSearch_IntegralMCTS::getPromisingNode(std::shared_ptr<Node> node) 
{
    // create copies of integral and the build order
    m_promisingNodeBuildOrder = m_buildOrder;
    m_promisingNodeIntegral = m_integral;

    std::shared_ptr<Node> returnNode = node;
    std::shared_ptr<Edge> edge;

    while(returnNode->getNumEdges() > 0 && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        const GameState & prevStateNode = returnNode->getState();

        // select the edge with the highest UCT value
        edge = returnNode->selectChildEdge(m_exploration_parameter, m_rnggen, m_params);

        // the node doesn't exist in memory
        if (edge->getChild() == nullptr)
        {
            returnNode = returnNode->notExpandedChild(edge, m_params);
            updateNodeVisits(edge->getChild() != nullptr, isTerminalNode(*returnNode));
            updateBOIntegral(*returnNode, edge->getAction(), prevStateNode, false);
            return std::pair<std::shared_ptr<Node>, bool>(returnNode, true);
        }

        // the node exists and is pointed to by the edge
        // get the next child
        returnNode = edge->getChild();
        updateNodeVisits(false, isTerminalNode(*returnNode));

        // update build order and integral
        updateBOIntegral(*returnNode, edge->getAction(), prevStateNode, false);
    }

    return std::pair<std::shared_ptr<Node>, bool>(returnNode, false);
}

bool CombatSearch_IntegralMCTS::isTerminalNode(const Node & node) const
{
    return node.isTerminal();
}

void CombatSearch_IntegralMCTS::randomPlayout(Node node)
{
    // do a rollout until we reach a terminal state
    while (!isTerminalNode(node) && m_results.nodeVisits < m_params.getNumberOfNodes())
    {
        GameState prevGameState(node.getState());
        doRandomAction(node, prevGameState);
    }
}

void CombatSearch_IntegralMCTS::doRandomAction(Node & node, const GameState & prevGameState)
{
    // generate possible actions
    ActionSetAbilities legalActions;
    generateLegalActions(node.getState(), legalActions, m_params);

    // the placeholder Chronoboost is expanded into a Chronoboost for each target.
    // this gives each chrono boostable target a fair chance at getting picked
    //getChronoBoostTargets(node, legalActions);

    // do an action at random
    if (legalActions.size() > 0)
    {
        std::uniform_int_distribution<> distribution(0, legalActions.size() - 1);
        int index = distribution(m_rnggen);
        Action action = legalActions[index];

        while (!node.doAction(action, m_params))
        {
            legalActions.remove(action.first, index);
            if (legalActions.size() == 0)
            {
                updateIntegralTerminal(node, prevGameState);
                node.setTerminal();
                m_results.leafNodesVisited++;
                return;
            }
            else if (legalActions.size() == 1)
            {
                index = 0;
                action = legalActions[index];
            }
            else
            {
                std::uniform_int_distribution<> distribution(0, legalActions.size() - 1);
                index = distribution(m_rnggen);
                action = legalActions[index];
            }
        }
        if (action.first.isAbility())
        {
            updateBOIntegral(node, ActionAbilityPair(action.first, node.getState().getLastAbility()), prevGameState, false);
        }
        else
        {
            updateBOIntegral(node, ActionAbilityPair(action.first, AbilityAction()), prevGameState, false);
        }
    }
    else
    {
        updateIntegralTerminal(node, prevGameState);
        node.setTerminal();
    }

    updateNodeVisits(false, isTerminalNode(node));
}

bool CombatSearch_IntegralMCTS::timeLimitReached()
{
    if (m_params.getUseTotalTimeLimit())
    {
        return (m_params.getSearchTimeLimit() && (m_numTotalSimulations % 5 == 0) && boost::chrono::duration_cast<boost::chrono::duration<double, boost::milli>>(boost::chrono::thread_clock::now() - m_searchTimerCPU).count() > m_params.getSearchTimeLimit());
    }
    
    return (m_params.getSearchTimeLimit() && (m_numTotalSimulations % 5 == 0) && (m_searchTimer.getElapsedTimeInMilliSec() > m_params.getSearchTimeLimit()));
}

//void CombatSearch_IntegralMCTS::getChronoBoostTargets(const Node & node, ActionSetAbilities & legalActions)
//{
//    int chronoBoostIndex = -1;
//    for (auto it = legalActions.begin(); it != legalActions.end(); ++it)
//    {
//        if (it->first.isAbility())
//        {
//            chronoBoostIndex = int(it - legalActions.begin());
//            node.getState().getSpecialAbilityTargets(legalActions, chronoBoostIndex);
//            break;
//        }
//
//    }
//    // chronoboost is an invalid action
//    if (chronoBoostIndex != -1 && legalActions[chronoBoostIndex].second == -1)
//    {
//        legalActions.remove(legalActions[chronoBoostIndex].first, chronoBoostIndex);
//    }
//}

void CombatSearch_IntegralMCTS::updateIntegralTerminal(const Node & node, const GameState & prevGameState)
{
    GameState stateCopy(prevGameState);
    stateCopy.fastForward(m_params.getFrameTimeLimit());
    m_promisingNodeIntegral.update(stateCopy, m_promisingNodeBuildOrder, m_params, m_searchTimer, false);
    m_promisingNodeIntegral.setState(stateCopy);
}

void CombatSearch_IntegralMCTS::updateBOIntegral(const Node & node, const ActionAbilityPair & action, const GameState & prevGameState, bool permanantUpdate)
{
    // if the node is terminal, it means it doesn't have any children. we add the action
    // that took us to this node, fast forward to the frame limit, and update the integral
    if (isTerminalNode(node))
    {
        GameState stateCopy(prevGameState);
        // fast forward to the end of the timelimit
        stateCopy.fastForward(m_params.getFrameTimeLimit());

        // update the integral. since no action is done, we don't need to update the build order
        if (permanantUpdate)
        {
            m_buildOrder.add(action);
            m_integral.update(stateCopy, m_buildOrder, m_params, m_searchTimer, false);
            m_integral.setState(stateCopy);
        }
        else
        {
            m_promisingNodeBuildOrder.add(action);
            m_promisingNodeIntegral.update(stateCopy, m_promisingNodeBuildOrder, m_params, m_searchTimer, false);
            m_promisingNodeIntegral.setState(stateCopy);
        }
    }

    // add the action and calculate the integral
    else
    {
        if (permanantUpdate)
        {
            m_buildOrder.add(action);
            m_integral.update(node.getState(), m_buildOrder, m_params, m_searchTimer, false);
        }
        else
        {
            m_promisingNodeBuildOrder.add(action);
            m_promisingNodeIntegral.update(node.getState(), m_promisingNodeBuildOrder, m_params, m_searchTimer, false);
        }
    }
}

void CombatSearch_IntegralMCTS::backPropogation(std::shared_ptr<Node> node)
{
    std::shared_ptr<Node> current_node = node;
    std::shared_ptr<Edge> parent_edge = current_node->getParentEdge();

    if (m_promisingNodeIntegral.getCurrentStackValue() > m_bestIntegralFound.getCurrentStackValue() || 
        ((m_promisingNodeIntegral.getCurrentStackValue() == m_bestIntegralFound.getCurrentStackValue()) && Eval::StateBetter(m_promisingNodeIntegral.getState(), m_bestIntegralFound.getState())))
    {
        m_bestIntegralFound = m_promisingNodeIntegral;
        m_bestBuildOrderFound = m_promisingNodeBuildOrder;
        m_needToWriteBestValue = true;
        //std::cout << "best value found so far: " << m_bestIntegralFound.getCurrentStackEval() << std::endl;
    }

    //std::cout << "Simulation: " << m_numTotalSimulations << ". Value of search: " << m_promisingNodeIntegral.getCurrentStackValue() << std::endl;

    while (true)
    {
        parent_edge->updateEdge(m_promisingNodeIntegral.getCurrentStackValue());

        //std::cout << "value of " << current_node->getAction().first.getName() << " changed to: " << current_node->getValue() << std::endl;
        //std::cout << std::endl;

        if (parent_edge != nullptr && parent_edge->getParent() != nullptr && parent_edge->getParent()->getParentEdge() == nullptr)
        {
            if (m_rootRewards.find(parent_edge->getAction().first.getName()) == m_rootRewards.end())
            {
                m_rootRewards[parent_edge->getAction().first.getName()] = std::vector<FracType>();
            }
            m_rootRewards.at(parent_edge->getAction().first.getName()).push_back(m_promisingNodeIntegral.getBestStackValue());
        }

        if (parent_edge->getParent() == nullptr)
        {
            break;
        }

        current_node = parent_edge->getParent();
        parent_edge = current_node->getParentEdge();
    }
}

std::pair<BuildOrderAbilities, CombatSearch_IntegralDataFinishedUnits> CombatSearch_IntegralMCTS::pickBestBuildOrder(std::shared_ptr<Node> root,  bool useVisitCount)
{
    BuildOrderAbilities buildOrder;
    CombatSearch_IntegralDataFinishedUnits integral;

    std::shared_ptr<Node> bestNode = root;
    std::shared_ptr<Edge> bestEdge;

    // pick the child node with the highest action value until we reach a leaf node
    while (bestNode->getNumEdges() > 0)
    {
        if (useVisitCount)
        {
            bestEdge = bestNode->getHighestVisitedChild();
        }
        else
        {
            bestEdge = bestNode->getHighestValueChild(m_params);
        }
        if (bestEdge->getChild() == nullptr)
        {
            break;
        }
        bestNode = bestEdge->getChild();

        //std::cout << "edge value: " << bestEdge->getValue() << std::endl;

        buildOrder.add(bestEdge->getAction());
        integral.update(bestNode->getState(), buildOrder, m_params, m_searchTimer, false);
    }

    // there are no more actions, but we still need to fast forward to the time
    // limit to properly calculate the integral
    GameState finalState(bestNode->getState());
    finalState.fastForward(m_params.getFrameTimeLimit());
    integral.update(finalState, buildOrder, m_params, m_searchTimer, false);

    return std::make_pair(buildOrder, integral);
}

void CombatSearch_IntegralMCTS::updateNodeVisits(bool nodeExpanded, bool isTerminal)
{
    m_results.nodeVisits++;
    if (nodeExpanded)
    {
        m_results.nodesExpanded++;
    }  
    if (isTerminal)
    {
        m_results.leafNodesVisited++;
    }
}

void CombatSearch_IntegralMCTS::writeResultsToFile(std::shared_ptr<Node> root)
{
    //pickBestBuildOrder(root, false);
    //FracType highestValueRoute = m_promisingNodeIntegral.getCurrentStackValue();

    m_resultsStream << m_results.nodesExpanded << "," << m_results.nodeVisits << ","
        << m_results.leafNodesExpanded << "," << m_results.leafNodesVisited << ","
        << m_results.searchTimer.getElapsedTimeInMilliSec() / 1000 << "," << m_numTotalSimulations << ","
        << m_bestBuildOrderFound.getNameString() << "," << m_bestIntegralFound.getCurrentStackEval() << ","
        << m_bestIntegralFound.getCurrentStackValue() << "\n";
    
    m_needToWriteBestValue = false;
}

void CombatSearch_IntegralMCTS::printResults()
{
    m_bestIntegralFound.print(m_bestBuildOrderFound);
    std::cout << "\nRan " << m_numTotalSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000* m_numTotalSimulations / m_results.timeElapsed) << " simulations/sec\n";
    std::cout << "Nodes expanded: " << m_results.nodesExpanded << ". Total nodes visited: " << m_results.nodeVisits << ", at a rate of " << (1000 * m_results.nodeVisits / m_results.timeElapsed) << " nodes/sec\n";
}

#include "BuildOrderPlotter.h"
void CombatSearch_IntegralMCTS::writeResultsFile(const std::string & dir, const std::string & filename)
{
    /*BuildOrderPlotter plot;
    plot.setOutputDir(dir);
    plot.addPlot(filename, m_params.getInitialState(), m_bestBuildOrderFound);
    plot.doPlots();
*/
    m_bestIntegralFound.writeToFile(dir, filename);

    std::ofstream boFile(dir + "/" + filename + "_BuildOrder.txt", std::ofstream::out | std::ofstream::app);
    boFile << "\nRan " << m_numTotalSimulations << " simulations in " << m_results.timeElapsed << "ms @ " << (1000 * m_numTotalSimulations / m_results.timeElapsed) << " simulations/sec" << std::endl;
    boFile << "Nodes expanded: " << m_results.nodesExpanded << ". Total nodes visited: " << m_results.nodeVisits << ", at a rate of " << (1000 * m_results.nodeVisits / m_results.timeElapsed) << " nodes/sec\n";

    // Write search data to file 
    std::ofstream file(m_dir + "/" + m_prefix + "_Results.csv", std::ofstream::out | std::ofstream::app);
    file << m_resultsStream.rdbuf();
    file.close();
    m_resultsStream.str(std::string());


    std::ofstream searchData(m_dir + "/" + m_prefix + "_SearchData.txt", std::ofstream::out | std::ofstream::app);
    searchData << "Max eval found: " << m_bestIntegralFound.getCurrentStackValue() << "\n";
    searchData << "Finished build order eval: " << m_results.finishedEval << "\n";
    searchData << "Useful build order eval: " << m_results.usefulEval << "\n";
    searchData << "Max value found: " << m_bestIntegralFound.getCurrentStackEval() << "\n";
    searchData << "Finished build order value: " << m_results.finishedValue << "\n";
    searchData << "Useful build order value: " << m_results.usefulValue << "\n";
    searchData << "Best build order (all): " << m_bestBuildOrderFound.getNameString() << std::endl;
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