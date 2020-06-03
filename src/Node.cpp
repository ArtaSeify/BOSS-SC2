#include "Node.h"
#include "Eval.h"
#include "GPUQueue.h"
#include "CombatSearch_Integral_MCTS.h"

using namespace BOSS;

const std::array<double, 70> Node::alphas = { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };

Node::Node(const std::shared_ptr<Edge> & parent)
    : m_state()
    , m_children()
    , m_parent(parent)
    , m_lock()
    , m_leaf(false)
    , m_timesVisited(0)
    , m_policySet(false)
{

}
Node::Node(const std::shared_ptr<Edge>& parent, const GameState& state)
    : m_state(state)
    , m_children()
    , m_parent(parent)
    , m_lock()
    , m_leaf(false)
    , m_timesVisited(0)
    , m_policySet(false)
{

}

void Node::cleanUp(int threads)
{
    #pragma omp parallel for num_threads(threads)
    for (int i = 0; i < m_children.size(); ++i)
    {
        auto & edge = m_children[i];
        edge->cleanUp();
        //std::cout << "removing ownership of edge" << std::endl;
        //std::cout << "edge use count: " << edge.use_count() << std::endl;
        edge.reset();
    }
    m_children.clear();
    //std::cout << "all edges of this node cleaned up!" << std::endl;
}

void Node::changeRootCheck(const CombatSearchParameters & params, int currentRootSimulations) const
{
    int timesVisited = 0;
    for (int i = 0; i < m_children.size(); ++i)
    {
        BOSS_ASSERT(m_children[i]->virtualLoss() == 0, "Virtual loss should be 0 when changing root, but is %i", m_children[i]->virtualLoss());
        timesVisited += m_children[i]->realTimesVisited();
    }

    BOSS_ASSERT((!params.getChangingRootReset() && timesVisited >= params.getSimulationsPerStep()) || timesVisited == currentRootSimulations,
        "The number of times the edges are visited %i should equal to the number of simulations before changing the root %i", timesVisited, currentRootSimulations);

}

void Node::createChildrenEdges(const CombatSearchParameters & params, FracType currentValue, bool rootNode, gsl_rng * gsl_r)
{
    std::scoped_lock sl(m_lock);

    // if we already know it's a terminal node, just return
    if (m_leaf)
    {
        return;
    }

    // function already called by another thread
    if (m_children.size() > 0)
    {
        return;
    }

    assert(m_children.size() == 0);
    assert(!m_leaf);

    ActionSetAbilities legalActions;
    generateLegalActions(m_state, legalActions, params);

    // terminal node
    if (legalActions.size() == 0)
    {
        m_leaf = true;
        return;
    }

    std::shared_ptr<Node> thisNode = shared_from_this();
    for (int index = 0; index < legalActions.size(); ++index)
    {
        auto action = legalActions[index];

        // test the action to see if it's valid. if it's valid, we create the edge.
        // if it's not valid, we don't create the edge
        action = legalActions[index];
        if (action.first.isAbility())
        {
            GameState testState(m_state);
            testState.doAbility(action.first, action.second);
            m_children.push_back(std::make_shared<Edge>(ActionAbilityPair(action.first, testState.getLastAbility()), thisNode));
        }

        // action is valid, so create an edge
        else
        {
            m_children.push_back(std::make_shared<Edge>(ActionAbilityPair(action.first, AbilityAction()), thisNode));
        }
    }

    if (params.usePolicyValueNetwork())
    {
        networkPrediction(params, currentValue);
    }
    
    // use uniform probability if we're not using a policy network
    else if (!params.usePolicyNetwork())
    {
        float uniformVal = 1.f / m_children.size();
        for (auto& edge : m_children)
        {
            edge->setPolicy(uniformVal);
        }
        m_policySet = true;
    }

    // add dirichlet noise to root node edges
    if (rootNode)
    {
        addNoise(gsl_r);
    }

    //std::cout << "node finished and unlocking" << std::endl;
}

void Node::addNoise(gsl_rng* gsl_r)
{
    static const FracType epsilon = 0.25f;

    std::vector<double> dirichlet_noise;
    gsl_ran_dirichlet(gsl_r, m_children.size(), alphas.data(), dirichlet_noise.data());

    /*std::cout << "noise values:" << std::endl;
    for (int i = 0; i < m_edges.size(); ++i)
    {
        std::cout << dirichlet_noise[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "before noise:" << std::endl;
    for (auto& edge : m_edges)
    {
        std::cout << edge->getPolicyValue() << " ";
    }
    std::cout << std::endl;
    std::cout << "after noise:" << std::endl;*/
    for (int index = 0; index < m_children.size(); ++index)
    {
        auto edge = m_children[index];
        edge->setPolicy(FracType(((1 - epsilon) * edge->getPolicyValue()) + (epsilon * dirichlet_noise[index])));
        //std::cout << edge->getPolicyValue() << " ";
    }
    //std::cout << std::endl;
}

void Node::generateLegalActions(const GameState& state, ActionSetAbilities& legalActions, const CombatSearchParameters& params)
{
    // prune actions we have too many of already
    const ActionSetAbilities& allActions = params.getRelevantActions();
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
            state.getSpecialAbilityTargets(legalActions, legalActions.size() - 1);
        }
    }

    //std::cout << legalActions.toString() << std::endl;

    // if we enabled the always make workers flag, and workers are legal
    ActionType worker = ActionTypes::GetWorker(state.getRace());
    ActionSetAbilities illegalActions;
    if (params.getAlwaysMakeWorkers() && legalActions.contains(worker))
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
        legalActions.sort(state, params);
    }
}

void Node::networkPrediction(const CombatSearchParameters & params, FracType currentValue) const
{
    std::pair<std::string, int> state = m_state.getStateData(params);

    // push state into queue and wait until predictions are made
    int predictionIndex = GPUQueue::getInstance().push_back(state);
    GPUQueue::getInstance().wait();
    PyObject* values = GPUQueue::getInstance()[predictionIndex];

    PyObject* policyValues;
    PyObject* nodeValue;
    if (params.usePolicyValueNetwork())
    {
        policyValues = PyList_GetItem(values, 0);
        nodeValue = PyList_GetItem(values, 1);
    }
    else
    {
        policyValues = values;
    }

    // renormalize policy values
    FracType totalPolicyValues = 0;
    FracType uniformPolicy = 1.f / m_children.size();
    for (auto& edge : m_children)
    {
        totalPolicyValues += static_cast<FracType>(PyFloat_AsDouble(PyList_GetItem(policyValues, edge->getAction().first.getRaceActionID())));
    }

    // Set policy values
    for (auto& edge : m_children)
    {
        if (totalPolicyValues == 0)
        {
            edge->setPolicy(uniformPolicy);
        }
        else
        {
            edge->setPolicy(static_cast<FracType>(PyFloat_AsDouble(PyList_GetItem(policyValues, edge->getAction().first.getRaceActionID()))) / totalPolicyValues);
        }
    }

    // set node value
    if (params.usePolicyValueNetwork())
    {
        m_parent->setNetworkValue(static_cast<FracType>(PyFloat_AsDouble(PyList_GetItem(nodeValue, 0))) + currentValue);
        //std::cout << "edge: " << m_parentEdge->getAction().first.getName() << " edge value: " << m_parentEdge->getNetworkValue() << std::endl;
    }
    GPUQueue::getInstance().decPredictionReference();
    //std::cout << "finished with network in node" << std::endl;

    m_policySet = true;
}

void Node::removeEdges(const Edge & edge)
{
    for (auto it = m_children.begin(); it != m_children.end();)
    {
        // check if action are the same
        if ((*it)->getAction().first != edge.getAction().first)
        {
            (*it)->cleanUp();
            it = m_children.erase(it);
        }
        // check if targets of ability are the same
        else if ((*it)->getAction().second.targetID != edge.getAction().second.targetID)
        {
            (*it)->cleanUp();
            it = m_children.erase(it);
        }
        // it equals edge, so we skip it
        else
        {
            ++it;
        }
    }
}

void Node::removeEdges()
{
    removeEdges(Edge());
}

//bool Node::doAction(Edge & edge, const CombatSearchParameters & params, bool makeNode)
//{
//    BOSS_ASSERT(edge.shared_from_this()->getChild() == nullptr, "Child node somehow exists?");
//
//    const ActionAbilityPair & action = edge.getAction();
//
//    if (action.first.isAbility())
//    {
//        m_state.doAbility(action.first, action.second.targetID);
//    }
//    else
//    {
//        m_state.doAction(action.first);
//    }
//    
//    // expand node
//    if (edge.getChild() == nullptr && ((edge.realTimesVisited() >= Edge::NODE_VISITS_BEFORE_EXPAND) || makeNode))
//    {
//        edge.setChild(shared_from_this());
//        return true;
//    }
//
//    return false;
//}

//void Node::doAction(const Action & action, const CombatSearchParameters & params)
//{
//    ActionType actionType = action.first;
//    NumUnits actionTarget = action.second;
//
//    BOSS_ASSERT(!(actionType == ActionTypes::GetSpecialAction(m_state.getRace()) && actionTarget == -1), "Non targetted ability action should not be passed to doAction");
//    
//    if (actionType.isAbility())
//    {
//        m_state.doAbility(actionType, actionTarget);
//    }
//    else
//    {
//        m_state.doAction(actionType);
//    }
//}

void Node::printChildren() const
{
    for (auto & edge : m_children)
    {
        edge->printValues();
    }

    std::cout << std::endl;
}

std::shared_ptr<Edge> Node::getPromisingChild(const CombatSearchParameters & params, FracType currentValue)
{
    // makes sure the children aren't being created at the same time
    {
        std::scoped_lock sl(m_lock);

        assert(m_children.size() > 0);

        // if we are using a policy network and have not done a network
        // prediction yet, do that first
        if (!m_policySet)
        {
            networkPrediction(params, currentValue);
        }
    }

    FracType sqrtVisits = (FracType)std::sqrt(m_timesVisited.load());
    int normalization = Edge::HIGHEST_VALUE_FOUND;

    int highestIndex = -1;
    FracType highestValue = std::numeric_limits<FracType>::lowest();
    bool reset = true;

    //m_state.write(true);
    while (reset)
    {
        reset = false;
        for (int index = 0; index < m_children.size(); ++index)
        {
            const std::shared_ptr<Edge>& edge = m_children[index];

            int currentHighestFound = Edge::HIGHEST_VALUE_FOUND;
            // new highest value found, need to redo all calculations
            if (normalization != currentHighestFound)
            {
                normalization = currentHighestFound;
                reset = true;
                break;
            }

            FracType value = edge->calculatePUCT(params.getExplorationValue(), sqrtVisits, normalization);

            // normalization failed, reset
            if (value == std::numeric_limits<FracType>::lowest())
            {
                reset = true;
                break;
            }

            //std::cout << "action: " << edge->getAction().x << "," << edge->getAction().y << ". Value: " << value << std::endl;

            // new best
            if (value > highestValue)
            {
                highestValue = value;
                highestIndex = index;
            }
        }
    }

    m_timesVisited += 1 + Edge::VIRTUAL_LOSS;
    m_children[highestIndex]->visited();

    return m_children[highestIndex];
}

//std::shared_ptr<Node> Node::notExpandedChild(Edge& edge, const CombatSearchParameters& params, FracType currentValue, bool makeNode)
//{
//    std::scoped_lock sl(m_mutex);
//
//    // child already created by another thread
//    if (edge.getChild() != nullptr)
//    {
//        return edge.getChild();
//    }
//    // create a temporary node
//    std::shared_ptr<Node> node = std::make_shared<Node>(m_state, edge);
//
//    std::scoped_lock newNodeLock(node->m_mutex);
//
//    // if the node is expanded, we create the edges of the node and do network evaluation
//    // if our network outputs a value. if it doesn't output a value, we create the child edges
//    // the second time we come to this node for efficiency reasons
//    if (node->doAction(edge, params, makeNode) && (params.usePolicyValueNetwork() || makeNode))
//    {
//        node->createChildrenEdges(params, currentValue);
//    }
//
//    return node;
//}

std::shared_ptr<Edge> Node::getHighestValueChild(const CombatSearchParameters & params) const
{
    //std::scoped_lock sl(m_mutex);

    // get the node with the highest action value
    auto edge = std::max_element(m_children.begin(), m_children.end(),
        [this, &params](const std::shared_ptr<Edge> & lhs, const std::shared_ptr<Edge> & rhs)
        { 
            if (lhs->getValue() == rhs->getValue())
            {
                std::unique_ptr<GameState> lhsState;
                std::unique_ptr<GameState> rhsState;
                if (lhs->getChild() == nullptr)
                {
                    const auto action = lhs->getAction();
                    lhsState = std::make_unique<GameState>(m_state);
                    if (action.first.isAbility())
                    {
                        lhsState->doAbility(action.first, action.second.targetID);
                    }
                    else
                    {
                        lhsState->doAction(action.first);
                    }
                }
                else
                {
                    lhsState = std::make_unique<GameState>(lhs->getChild()->getState());
                }

                if (rhs->getChild() == nullptr)
                {
                    const auto action = rhs->getAction();
                    rhsState = std::make_unique<GameState>(m_state);
                    if (action.first.isAbility())
                    {
                        rhsState->doAbility(action.first, action.second.targetID);
                    }
                    else
                    {
                        rhsState->doAction(action.first);
                    }
                }
                else
                {
                    rhsState = std::make_unique<GameState>(rhs->getChild()->getState());
                }
                return Eval::StateBetter(*rhsState, *lhsState);
            }
        
            return lhs->getValue() < rhs->getValue();
        });

    return *edge;
}

std::shared_ptr<Edge> Node::getHighestVisitedChild() const
{
    //std::scoped_lock sl(m_mutex);
    auto edge = std::max_element(m_children.begin(), m_children.end(),
        [](const std::shared_ptr<Edge> & lhs, const std::shared_ptr<Edge> & rhs)
        {
            return lhs->realTimesVisited() < rhs->realTimesVisited();
        });

    return *edge;
}

std::shared_ptr<Edge> Node::getHighestPolicyValueChild() const
{
    //std::scoped_lock sl(m_mutex);
    auto edge = std::max_element(m_children.begin(), m_children.end(),
        [](const std::shared_ptr<Edge> & lhs, const std::shared_ptr<Edge> & rhs)
        {
            return lhs->getPolicyValue() < rhs->getPolicyValue();
        });

    return *edge;
}

std::shared_ptr<Edge> Node::getChildProportionalToVisitCount(const CombatSearchParameters& params, std::mt19937 & rnggen) const
{
    //std::scoped_lock sl(m_mutex);

    /*std::cout << "actions: " << std::endl;
    printChildren();
    std::cout << std::endl;*/

    int totalVisits = 0;
    for (const auto& edge : m_children)
    {
        totalVisits += edge->realTimesVisited();
    }

    std::uniform_int_distribution<> distribution(1, totalVisits);
    int randomVisitCount = distribution(rnggen);
    
    for (const auto& edge : m_children)
    {
        int edgeVisits = edge->realTimesVisited();
        if (edgeVisits >= randomVisitCount)
        {
            return edge;
        }
        randomVisitCount -= edgeVisits;
    }

    BOSS_ASSERT(false, "Couldn't find an edge given the distribution");
    return std::make_shared<Edge>();
}

std::shared_ptr<Edge> Node::getRandomChild(RNG & rng)
{
    // makes sure the children aren't being created at the same time
    {
        std::scoped_lock sl(m_lock);
    }

    assert(m_children.size() > 0);

    std::uniform_int_distribution<int> dist(0, (int)m_children.size() - 1);
    std::shared_ptr<Edge> edge = m_children[dist(rng)];

    m_timesVisited += 1 + Edge::VIRTUAL_LOSS;
    edge->visited();

    return edge;
}


std::shared_ptr<Edge> Node::getChild(const ActionAbilityPair & action)
{
    //std::scoped_lock sl(m_mutex);
    BOSS_ASSERT(m_children.size() > 0, "Number of edges is %i", m_children.size());

    for (auto & edge : m_children)
    {
        if (edge->getAction().first == action.first && edge->getAction().second == action.second)
        {
            return edge;
        }
    }

    BOSS_ASSERT(false, "Tried to get edge with action %s, but it doesn't exist", action.first.getName().c_str());
    return std::make_shared<Edge>();
}

//std::vector<int> Node::getChronoboostTargets() const
//{
//    std::vector<int> targets;
//    for (const auto& edge : m_edges)
//    {
//        if (edge->getAction().first == ActionTypes::GetSpecialAction(Races::Protoss))
//        {
//            targets.push_back(edge->getAction().second.targetID);
//        }
//    }
//    return targets;
//}