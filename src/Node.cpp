#include "Node.h"
#include "Eval.h"
#include "GPUQueue.h"
#include "CombatSearch_ParallelIntegralMCTS.h"

using namespace BOSS;

const std::array<double, 70> Node::alphas = { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };

Node::Node()
    : m_parentEdge()
    , m_state()
    , m_edges()
    , m_isTerminalNode(false)
    , m_mutex()
{

}

Node::Node(const GameState & state)
    : m_parentEdge()
    , m_state(state)
    , m_edges()
    , m_isTerminalNode(false)
    , m_mutex()
{

}

Node::Node(const GameState & state, Edge & parentEdge)
    : m_parentEdge(parentEdge.shared_from_this())
    , m_state(state)
    , m_edges()
    , m_isTerminalNode(false)
    , m_mutex()
{

}

Node::Node(const Node& node)
    : m_mutex()
{
    std::scoped_lock sl(node.m_mutex);
    m_parentEdge = node.m_parentEdge;
    m_state = node.m_state;
    m_edges = node.m_edges;
    m_isTerminalNode = node.m_isTerminalNode;
}

Node& Node::operator=(const Node& node)
{
    std::scoped_lock sl(node.m_mutex);
    m_parentEdge = node.m_parentEdge;
    m_state = node.m_state;
    m_edges = node.m_edges;
    m_isTerminalNode = node.m_isTerminalNode;
    return *this;
}

void Node::cleanUp(int threads)
{
    #pragma omp parallel for num_threads(threads)
    for (int i = 0; i < m_edges.size(); ++i)
    {
        auto & edge = m_edges[i];
        edge->cleanUp();
        //std::cout << "removing ownership of edge" << std::endl;
        //std::cout << "edge use count: " << edge.use_count() << std::endl;
        edge.reset();
    }
    m_edges.clear();
    //std::cout << "all edges of this node cleaned up!" << std::endl;
}

void Node::createChildrenEdges(const CombatSearchParameters & params, FracType currentValue, bool rootNode, gsl_rng * gsl_r)
{
    // if we already know it's a terminal node, just return
    if (m_isTerminalNode)
    {
        return;
    }

    // function already called by another thread
    if (m_edges.size() > 0)
    {
        return;
    }

    ActionSetAbilities legalActions;
    generateLegalActions(m_state, legalActions, params);

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
            m_edges.push_back(std::make_shared<Edge>(ActionAbilityPair(action.first, testState.getLastAbility()), thisNode));
        }

        // action is valid, so create an edge
        else
        {
            m_edges.push_back(std::make_shared<Edge>(ActionAbilityPair(action.first, AbilityAction()), thisNode));
        }
    }

    // no edges were created, so this is a terminal node
    if (m_edges.size() == 0)
    {
        m_isTerminalNode = true;
    }

    if (params.usePolicyValueNetwork() || params.usePolicyNetwork())
    {
        networkPrediction(params, currentValue);

        // add dirichlet noise to root node edges
        if (rootNode)
        {
            static const FracType epsilon = 0.25f;
            
            std::array<double, 70> dirichlet_noise;
            gsl_ran_dirichlet(gsl_r, m_edges.size(), alphas.data(), dirichlet_noise.data());

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
            for (int index = 0; index < m_edges.size(); ++index)
            {
                auto edge = m_edges[index];
                edge->setPolicyValue(FracType(((1 - epsilon) * edge->getPolicyValue()) + (epsilon * dirichlet_noise[index])));
                //std::cout << edge->getPolicyValue() << " ";
            }
            //std::cout << std::endl;
        }
    }
    // use uniform probability if we're not using a policy network
    else if (!m_isTerminalNode)
    {
        float uniformVal = 1.f / m_edges.size();
        for (auto& edge : m_edges)
        {
            edge->setPolicyValue(uniformVal);
        }
    }
    //std::cout << "node finished and unlocking" << std::endl;
}

void Node::createChildrenEdgesSecondVisit(const CombatSearchParameters& params, FracType currentValue)
{
    BOSS_ASSERT(!params.usePolicyValueNetwork(), "This function should not be called when we are using a value network");

    std::scoped_lock sl(m_mutex);
    createChildrenEdges(params, currentValue);
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
    std::pair<std::string, int> state = m_state.getStateData(params, currentValue, getChronoboostTargets());

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

    // Set policy values
    for (auto& edge : m_edges)
    {
        //std::cout << "edge action: " << edge->getAction().first.getRaceActionID() << ", value: " 
        //    << python::extract<FracType>(policyValues[edge->getAction().first.getRaceActionID()]) << std::endl;
        // update the edge values
        edge->setPolicyValue(static_cast<FracType>(PyFloat_AsDouble(PyList_GetItem(policyValues, edge->getAction().first.getRaceActionID()))));
    }

    // set node value
    if (params.usePolicyValueNetwork())
    {
        m_parentEdge->setNetworkValue(static_cast<FracType>(PyFloat_AsDouble(PyList_GetItem(nodeValue, 0))));
        //std::cout << "edge: " << m_parentEdge->getAction().first.getName() << " edge value: " << m_parentEdge->getNetworkValue() << std::endl;
    }
    GPUQueue::getInstance().decPredictionReference();
    //std::cout << "finished with network in node" << std::endl;
}

void Node::removeEdges(const Edge & edge)
{
    for (auto it = m_edges.begin(); it != m_edges.end();)
    {
        // check if action are the same
        if ((*it)->getAction().first != edge.getAction().first)
        {
            (*it)->cleanUp();
            it = m_edges.erase(it);
        }
        // check if targets of ability are the same
        else if ((*it)->getAction().second.targetID != edge.getAction().second.targetID)
        {
            (*it)->cleanUp();
            it = m_edges.erase(it);
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

bool Node::doAction(Edge & edge, const CombatSearchParameters & params, bool makeNode)
{
    BOSS_ASSERT(edge.shared_from_this()->getChild() == nullptr, "Child node somehow exists?");

    const ActionAbilityPair & action = edge.getAction();

    if (action.first.isAbility())
    {
        m_state.doAbility(action.first, action.second.targetID);
    }
    else
    {
        m_state.doAction(action.first);
    }
    
    // expand node
    if (edge.getChild() == nullptr && ((edge.realTimesVisited() >= Edge::NODE_VISITS_BEFORE_EXPAND) || makeNode))
    {
        edge.setChild(shared_from_this());
        return true;
    }

    return false;
}

void Node::doAction(const Action & action, const CombatSearchParameters & params)
{
    ActionType actionType = action.first;
    NumUnits actionTarget = action.second;

    BOSS_ASSERT(!(actionType == ActionTypes::GetSpecialAction(m_state.getRace()) && actionTarget == -1), "Non targetted ability action should not be passed to doAction");
    
    if (actionType.isAbility())
    {
        m_state.doAbility(actionType, actionTarget);
    }
    else
    {
        m_state.doAction(actionType);
    }
}

void Node::printChildren() const
{
    for (auto & edge : m_edges)
    {
        edge->printValues();
    }

    std::cout << std::endl;
}

Edge & Node::selectChildEdge(const CombatSearchParameters & params, std::mt19937 & rnggen)
{
    {
        std::scoped_lock sl(m_mutex);
    }
    
    BOSS_ASSERT(m_edges.size() > 0, "selectChildEdge called when there are no edges.");
    
    std::vector<int> unvisitedEdges;
    int totalChildVisits = 0;
    bool visitEveryEdge = !params.usePolicyNetwork() && !params.usePolicyValueNetwork();
    FracType currentHighestValue = 0;
    for (int index = 0; index < m_edges.size(); ++index )
    {
        Edge & edge = *m_edges[index];
        edge.lock();

        // all unvisited edges are taken as an action first 
        if (visitEveryEdge && edge.realTimesVisitedNoLock() == 0)
        {
            unvisitedEdges.push_back(index);
        }

        if (unvisitedEdges.size() == 0)
        {
            totalChildVisits += edge.totalTimesVisitedNoLock();
            currentHighestValue = std::max(edge.getValueNoLock(), currentHighestValue);
        }

        edge.unlock();
    }

    if (currentHighestValue == 0.f)
    {
        currentHighestValue = 1;
    }
    
    // pick an unvisited edge at uniformly random. This is only done 
    // in MCTS without network
    if (visitEveryEdge)
    {
        if (unvisitedEdges.size() > 0)
        {
            std::uniform_int_distribution<> distribution(0, int(unvisitedEdges.size()) - 1);
            Edge& edge = *m_edges[unvisitedEdges[distribution(rnggen)]];
            edge.visited();
            return edge;
        }
    }

    float UCBValue = params.getExplorationValue() *
        static_cast<FracType>(std::sqrt(totalChildVisits));

    int maxIndex = 0;
    float maxActionValue = 0;
    while (true)
    {
        maxIndex = 0;
        maxActionValue = 0;
        float actionValue = 0;

        for (int index = 0; index < m_edges.size(); ++index)
        {
            const auto edge = m_edges[index];
            edge->lock();

            // calculate UCB value and get the total value of action
            // Q(s, a) + u(s, a)
            // we normalize the action value to a range of [0, 1] using the highest
            // value of the search thus far. 
            float childUCBValue = (UCBValue * edge->getPolicyValueNoLock()) / (1 + edge->totalTimesVisitedNoLock());
            actionValue = edge->getValueNoLock() / currentHighestValue;

            edge->unlock();
            // an edge was updated while we were picking a move, need to restart
            if (actionValue > 1)
            {
                break;
            }
            //BOSS_ASSERT(actionValue <= 1, "value of an action must be less than or equal to 1, but is %f", actionValue);

            float UCTValue = actionValue + childUCBValue;

            // store the index of this action
            if (maxActionValue < UCTValue)
            {
                maxActionValue = UCTValue;
                maxIndex = index;
            }
        }
        // an edge value was changed to be higher while we were deciding on an edge, so
        // we have to redo it
        if (actionValue > 1)
        {
            for (const auto & edge : m_edges)
            {
                currentHighestValue = std::max(edge->getValue(), currentHighestValue);
            }
        }
        else
        {
            break;
        }
    }
    //BOSS_ASSERT(maxActionValue <= 1, "value of an action must be less than or equal to 1, but is %f", maxActionValue);
    Edge& edge = *m_edges[maxIndex];
    edge.visited();
    return edge;
}

std::shared_ptr<Node> Node::notExpandedChild(Edge& edge, const CombatSearchParameters& params, FracType currentValue, bool makeNode)
{
    std::scoped_lock sl(m_mutex);

    // child already created by another thread
    if (edge.getChild() != nullptr)
    {
        return edge.getChild();
    }
    // create a temporary node
    std::shared_ptr<Node> node = std::make_shared<Node>(m_state, edge);

    std::scoped_lock newNodeLock(node->m_mutex);

    // if the node is expanded, we create the edges of the node and do network evaluation
    // if our network outputs a value. if it doesn't output a value, we create the child edges
    // the second time we come to this node for efficiency reasons
    if (node->doAction(edge, params, makeNode) && (params.usePolicyValueNetwork() || makeNode))
    {
        node->createChildrenEdges(params, currentValue);
    }

    return node;
}

Edge & Node::getHighestValueChild(const CombatSearchParameters & params) const
{
    //std::scoped_lock sl(m_mutex);

    // get the node with the highest action value
    auto edge = std::max_element(m_edges.begin(), m_edges.end(),
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

    return **edge;
}

Edge & Node::getHighestVisitedChild() const
{
    //std::scoped_lock sl(m_mutex);
    auto edge = std::max_element(m_edges.begin(), m_edges.end(),
        [](const std::shared_ptr<Edge> & lhs, const std::shared_ptr<Edge> & rhs)
        {
            return lhs->realTimesVisited() < rhs->realTimesVisited();
        });

    return **edge;
}

Edge & Node::getHighestPolicyValueChild() const
{
    //std::scoped_lock sl(m_mutex);
    auto edge = std::max_element(m_edges.begin(), m_edges.end(),
        [](const std::shared_ptr<Edge> & lhs, const std::shared_ptr<Edge> & rhs)
        {
            return lhs->getPolicyValue() < rhs->getPolicyValue();
        });

    return **edge;
}

Edge & Node::getChildProportionalToVisitCount(const CombatSearchParameters& params, std::mt19937 & rnggen) const
{
    //std::scoped_lock sl(m_mutex);

    /*std::cout << "actions: " << std::endl;
    printChildren();
    std::cout << std::endl;*/

    int totalVisits = 0;
    for (const auto& edge : m_edges)
    {
        totalVisits += edge->realTimesVisited();
    }

    std::uniform_int_distribution<> distribution(1, totalVisits);
    int randomVisitCount = distribution(rnggen);
    
    for (const auto& edge : m_edges)
    {
        int edgeVisits = edge->realTimesVisited();
        if (edgeVisits >= randomVisitCount)
        {
            return *edge;
        }
        randomVisitCount -= edgeVisits;
    }

    BOSS_ASSERT(false, "Couldn't find an edge given the distribution");
    return *std::make_shared<Edge>();
}

Edge & Node::getRandomEdge()
{
    //std::scoped_lock sl(m_mutex);
    return *m_edges[std::rand() % m_edges.size()];
}

Edge & Node::getChild(const ActionAbilityPair & action)
{
    //std::scoped_lock sl(m_mutex);
    BOSS_ASSERT(m_edges.size() > 0, "Number of edges is %i", m_edges.size());

    for (auto & edge : m_edges)
    {
        if (edge->getAction().first == action.first && edge->getAction().second == action.second)
        {
            return *edge;
        }
    }

    BOSS_ASSERT(false, "Tried to get edge with action %s, but it doesn't exist", action.first.getName().c_str());
    return *std::make_shared<Edge>();
}

std::vector<int> Node::getChronoboostTargets() const
{
    std::vector<int> targets;
    for (const auto& edge : m_edges)
    {
        if (edge->getAction().first == ActionTypes::GetSpecialAction(Races::Protoss))
        {
            targets.push_back(edge->getAction().second.targetID);
        }
    }
    return targets;
}