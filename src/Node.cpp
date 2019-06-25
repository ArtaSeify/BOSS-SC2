#include "Node.h"
#include "Eval.h"
#include "GPUQueue.h"

using namespace BOSS;

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

void Node::createChildrenEdges(const CombatSearchParameters & params, FracType currentValue)
{
    //std::scoped_lock sl(m_mutex);
    //m_mutex.lock();
    // if we already know it's a terminal node, just return
    if (m_isTerminalNode)
    {
        //m_mutex.unlock();
        return;
    }

    // function already called by another thread
    if (m_edges.size() > 0)
    {
        //m_mutex.unlock();
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
        //m_mutex.unlock();
        return;
    }

    if (params.usePolicyValueNetwork() || params.usePolicyNetwork())
    {
        std::stringstream ss;
        m_state.writeToSS(ss, params, currentValue, getChronoboostTargets());

        // push state into queue and wait until predictions are made
        int predictionIndex = GPUQueue::getInstance().push_back(ss.str());
        //m_mutex.unlock();
        GPUQueue::getInstance().wait();
        //m_mutex.lock();
        PyObject* values = GPUQueue::getInstance()[predictionIndex];
        //std::cout << "grabbing value" << std::endl;
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
            m_parentEdge->setNetworkValue(static_cast<FracType>(PyFloat_AsDouble(PyList_GetItem(nodeValue, 0))) * Edge::MAX_EDGE_VALUE_EXPECTED);
            //std::cout << "parent network value: " << m_parentEdge->getNetworkValue() << std::endl;
        }
        GPUQueue::getInstance().decPredictionReference();
        //std::cout << "finished with network in node" << std::endl;
    }
    // use uniform probability if we're not using a policy network
    else
    {
        float uniformVal = 1.f / m_edges.size();
        for (auto& edge : m_edges)
        {
            edge->setPolicyValue(uniformVal);
        }
    }
    //std::cout << "node finished and unlocking" << std::endl;
    //m_mutex.unlock();
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
    if (edge.getChild() == nullptr && ((edge.realTimesVisited() == Edge::NODE_VISITS_BEFORE_EXPAND) || makeNode))
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

Edge & Node::selectChildEdge(FracType exploration_param, std::mt19937 & rnggen, const CombatSearchParameters & params)
{
    std::scoped_lock sl(m_mutex);

    BOSS_ASSERT(m_edges.size() > 0, "selectChildEdge called when there are no edges.");
    // uniform policy
    //float policyValue = 1.f / m_edges.size();

    std::vector<int> unvisitedEdges;
    int totalChildVisits = 0;
    for (int index = 0; index < m_edges.size(); ++index )
    {
        Edge & edge = *m_edges[index];

        int edgeTimesVisited = edge.realTimesVisited();
        // all unvisited edges are taken as an action first 
        if (edgeTimesVisited == 0)
        {
            unvisitedEdges.push_back(index);
        }

        totalChildVisits += edgeTimesVisited;
    }

    // pick an unvisited edge at uniformly random
    if (unvisitedEdges.size() > 0)
    {
        std::uniform_int_distribution<> distribution(0, int(unvisitedEdges.size()) - 1);
        Edge & edge = *m_edges[unvisitedEdges[distribution(rnggen)]];
        edge.visited();
        return edge;
    }

    float UCBValue = exploration_param *
        static_cast<FracType>(std::sqrt(totalChildVisits));

    float maxActionValue = 0;
    int maxIndex = 0;
    int index = 0;
    FracType currentHighestValue = Edge::CURRENT_HIGHEST_VALUE;
    for (auto & edge : m_edges)
    {
        // calculate UCB value and get the total value of action
        // Q(s, a) + u(s, a)
        // we normalize the action value to a range of [0, 1] using the highest
        // value of the search thus far. 
        //BOSS_ASSERT(edge->totalTimesVisited() == edge->realTimesVisited(), "Should be equal");
        float childUCBValue = (UCBValue * edge->getPolicyValue()) / (1 + edge->totalTimesVisited());
        float actionValue = edge->getValue() / currentHighestValue;
        BOSS_ASSERT(actionValue <= 1, "value of an action must be less than or equal to 1, but is %f", actionValue);

        float UCTValue = actionValue + childUCBValue;

        //std::cout << edge->getAction().first.getName() << " " << actionValue << ", " << childUCBValue << std::endl;

        // store the index of this action
        if (maxActionValue < UCTValue)
        {
            maxActionValue = UCTValue;
            maxIndex = index;
        }

        ++index;
    }
    //std::cout << std::endl;
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

    // if the node is expanded, we create the edges of the node
    if (node->doAction(edge, params, makeNode))
    {
        node->createChildrenEdges(params, currentValue);
    }

    return node;
}

Edge & Node::getHighestValueChild(const CombatSearchParameters & params) const
{
    std::scoped_lock sl(m_mutex);

    // get the node with the highest action value
    auto edge = std::max_element(m_edges.begin(), m_edges.end(),
        [this, &params](const std::shared_ptr<Edge> & lhs, const std::shared_ptr<Edge> & rhs)
        { 
            if (lhs->getValue() == rhs->getValue())
            {
                std::unique_ptr<Node> lhsNode = std::make_unique<Node>(*lhs->getParent());
                std::unique_ptr<Node> rhsNode = std::make_unique<Node>(*rhs->getParent());
                if (lhs->getChild() == nullptr)
                {
                    lhsNode->doAction(*lhs, params);
                }
                if (rhs->getChild() == nullptr)
                {
                    rhsNode->doAction(*rhs, params);
                }
                return Eval::StateBetter(rhsNode->getState(), lhsNode->getState());;
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

Edge & Node::getChildProportionalToVisitCount(std::mt19937& rnggen, const CombatSearchParameters& params) const
{
    //std::scoped_lock sl(m_mutex);

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