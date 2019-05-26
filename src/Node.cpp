#define BOOST_PYTHON_STATIC_LIB
#include <boost/python/list.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/object_attributes.hpp>
#include <boost/python/import.hpp>
#include "Node.h"
#include "Eval.h"

using namespace BOSS;
namespace python = boost::python;

Node::Node(const GameState & state)
    : m_parentEdge()
    , m_state(state)
    , m_edges()
    , isTerminalNode(false)
{

}

Node::Node(const GameState & state, std::shared_ptr<Edge> parentEdge)
    : m_parentEdge(parentEdge)
    , m_state(state)
    , m_edges()
    , isTerminalNode(false)
{

}

void Node::cleanUp()
{
    for (auto & edge : m_edges)
    {
        edge->cleanUp();
        //std::cout << "removing ownership of edge" << std::endl;
        //std::cout << "edge use count: " << edge.use_count() << std::endl;
        edge.reset();
    }
    //std::cout << "all edges of this node cleaned up!" << std::endl;
}

void Node::createChildrenEdges(ActionSetAbilities & legalActions, const CombatSearchParameters & params)
{
    // if we already know it's a terminal node, just return
    if (isTerminalNode)
    {
        return;
    }

    std::shared_ptr<Node> thisNode = shared_from_this();
    for (int index = 0; index < legalActions.size(); ++index)
    {
        auto action = legalActions[index];

        // the placeholder Chronoboost is expanded into a Chronoboost for each target
        if (action.first.isAbility() && action.second == -1)
        {
            m_state.getSpecialAbilityTargets(legalActions, index);

            // Chronoboost is no longer a legal action
            if (legalActions[index].second == -1)
            {
                continue;
            }
        }
        
        // test the action to see if it's valid. if it's valid, we create the edge.
        // if it's not valid, we don't create the edge
        action = legalActions[index];
        if (action.first.isAbility())
        {
            GameState testState(m_state);
            testState.doAbility(action.first, action.second);
            if (testState.getCurrentFrame() <= params.getFrameTimeLimit())
            {
                m_edges.push_back(std::make_shared<Edge>(ActionAbilityPair(action.first, testState.getLastAbility()), thisNode));
            }
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
        isTerminalNode = true;
    }

    if (params.usePolicyNetwork())
    {
        std::stringstream ss;
        m_state.writeToSS(ss, params);

        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        PyObject* policyValues = PyEval_CallObject(CONSTANTS::Predictor, Py_BuildValue("(s)", ss.str().c_str()));
        PyGILState_Release(gstate);
        
        for (auto& edge : m_edges)
        {
            //std::cout << "edge action: " << edge->getAction().first.getRaceActionID() << ", value: " 
            //    << python::extract<FracType>(policyValues[edge->getAction().first.getRaceActionID()]) << std::endl;
            // update the edge values
            edge->setPolicyValue(static_cast<FracType>(PyFloat_AsDouble(PyList_GetItem(policyValues, edge->getAction().first.getRaceActionID()))));
        }
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
}

void Node::removeEdges(std::shared_ptr<Edge> edge)
{
    for (auto it = m_edges.begin(); it != m_edges.end();)
    {
        // check if action are the same
        if ((*it)->getAction().first != edge->getAction().first)
        {
            (*it)->cleanUp();
            it = m_edges.erase(it);
        }
        // check if targets of ability are the same
        else if ((*it)->getAction().second.targetID != edge->getAction().second.targetID)
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
    removeEdges(std::make_shared<Edge>());
}

bool Node::doAction(std::shared_ptr<Edge> edge, const CombatSearchParameters & params, bool makeNode)
{
    const ActionAbilityPair & action = edge->getAction();

    if (action.first.isAbility())
    {
        m_state.doAbility(action.first, action.second.targetID);
    }
    else
    {
        m_state.doAction(action.first);
    }

    // if we go over the frame time limit, this node is invalid
    if (m_state.getCurrentFrame() > params.getFrameTimeLimit())
    {
        BOSS_ASSERT(false, "invalid action given to doAction()");
        return false;
    }

    if (edge->timesVisited() == Edge::NODE_VISITS_BEFORE_EXPAND - 1 || makeNode)
    {
        //std::shared_ptr<Node> newNode = std::make_shared<Node>(m_state, edge);
        //edge->setChild(newNode);
        edge->setChild(shared_from_this());

        //// evaluate the newly created state using the network and store the value in the edge
        //std::stringstream ss;
        //if (params.useNetworkPrediction())
        //{
        //    m_state.writeToSS(ss, params);

        //    // evaluate the states. the results will be returned as string
        //    python::object value = CONSTANTS::Predictor.attr("predict")(ss.str());

        //    // update the edge values
        //    //std::cout << python::extract<FracType>(value[0]) << std::endl;
        //    edge->setNetworkValue(python::extract<FracType>(value[0]));
        //}
    }

    return true;
}

bool Node::doAction(const Action & action, const CombatSearchParameters & params)
{
    ActionType actionType = action.first;
    NumUnits actionTarget = action.second;

    // Chronoboost place holder action. Need to find all valid targets, then we pick a target at random
    if (actionType == ActionTypes::GetSpecialAction(m_state.getRace()) && actionTarget == -1)
    {
        BOSS_ASSERT(false, "Non targetted ability action should not be passed to doAction");
    }
    
    if (actionType.isAbility())
    {
        m_state.doAbility(actionType, actionTarget);
    }
    else
    {
        m_state.doAction(actionType);
    }

    return true;
}

void Node::printChildren() const
{
    for (auto & edge : m_edges)
    {
        std::cout << "action: " << edge->getAction().first.getName();
        if (edge->getChild() != nullptr)
        {
            std::cout << ", state frame: " << edge->getChild()->getState().getCurrentFrame() << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
}

std::shared_ptr<Edge> Node::selectChildEdge(FracType exploration_param, std::mt19937 & rnggen, const CombatSearchParameters & params) const
{
    BOSS_ASSERT(m_edges.size() > 0, "selectChildEdge called when there are no edges.");

    // uniform policy
    //float policyValue = 1.f / m_edges.size();

    std::vector<int> unvisitedEdges;
    int totalChildVisits = 0;
    for (int index = 0; index < m_edges.size(); ++index )
    {
        const auto& edge = m_edges[index];

        int edgeTimesVisited = edge->timesVisited();
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
        return m_edges[unvisitedEdges[distribution(rnggen)]];
    }

    float UCBValue = exploration_param *
        static_cast<FracType>(std::sqrt(totalChildVisits));

    /*float UCBValue = exploration_param *
            static_cast<FracType>(std::sqrt(totalChildVisits));*/

    float maxActionValue = 0;
    int maxIndex = 0;
    int index = 0;

    for (auto & edge : m_edges)
    {
        // calculate UCB value and get the total value of action
        // Q(s, a) + u(s, a)
        // we normalize the action value to a range of [0, 1] using the highest
        // value of the search thus far. 
        float childUCBValue = (UCBValue * edge->getPolicyValue()) / (1 + edge->timesVisited());
        float actionValue = edge->getValue() / Edge::CURRENT_HIGHEST_VALUE;
        BOSS_ASSERT(actionValue <= 1, "value of an action must be less than or equal to 1, but is %f", actionValue);

        float UCTValue = actionValue + childUCBValue;

        /*std::cout << "UCB Value: " << childUCBValue << std::endl;
        std::cout << "action value: " << actionValue << std::endl;
        std::cout << "UCT value: " << UCTValue << std::endl;*/

        //std::cout << "edge value: " << edge->getValue() / Edge::CURRENT_HIGHEST_VALUE << ". UCT value: " << childUCBValue << std::endl;

        //std::cout << "times visited: " << child.timesVisited() << std::endl;

        // store the index of this action
        if (maxActionValue < UCTValue)
        {
            maxActionValue = UCTValue;
            maxIndex = index;
        }

        ++index;
    }

    return m_edges[maxIndex];
}

void Node::printPValues(FracType exploration_param, std::mt19937& rnggen, const CombatSearchParameters& params) const
{
    if (m_edges.size() == 0)
    {
        return;
    }

    std::vector<int> unvisitedEdges;
    int totalChildVisits = 0;
    for (int index = 0; index < m_edges.size(); ++index)
    {
        const auto& edge = m_edges[index];

        int edgeTimesVisited = edge->timesVisited();
        // all unvisited edges are taken as an action first 
        if (edgeTimesVisited == 0)
        {
            unvisitedEdges.push_back(index);
        }

        totalChildVisits += edgeTimesVisited;
    }

    // pick an unvisited edge at uniformly random

    float UCBValue = exploration_param *
        static_cast<FracType>(std::sqrt(totalChildVisits));

    float maxActionValue = 0;
    int maxIndex = 0;
    int index = 0;

    for (int index = 0; index < m_edges.size(); ++index)
    {
        const auto& edge = m_edges[index];
        // calculate UCB value and get the total value of action
        // Q(s, a) + u(s, a)
        // we normalize the action value to a range of [0, 1] using the highest
        // value of the search thus far. 
        float childUCBValue = UCBValue / (1 + edge->timesVisited());

        float UCBWithoutParam = static_cast<FracType>(std::sqrt(totalChildVisits)) / (1 + edge->timesVisited());
        float p = 1 / (exp(UCBWithoutParam * UCBWithoutParam * 2 * edge->timesVisited()));
        std::cout << "edge: " << edge->getAction().first.getName() << " has UCB value: " << UCBWithoutParam << ". has p value: " << p << std::endl;
    }
    std::cout << std::endl;
}

std::shared_ptr<Node> Node::notExpandedChild(std::shared_ptr<Edge> edge, const CombatSearchParameters & params, bool makeNode) const
{
    // create a temporary node
    std::shared_ptr<Node> node = std::make_shared<Node>(m_state, edge);
    BOSS_ASSERT(node->doAction(edge, params, makeNode), "notExpandedChild should only be called with legal edge");
    return node;
}

std::shared_ptr<Edge> Node::getHighestValueChild(const CombatSearchParameters & params) const
{
    // get the node with the highest action value
    auto edge = std::max_element(m_edges.begin(), m_edges.end(),
        [this, &params](const std::shared_ptr<Edge> & lhs, const std::shared_ptr<Edge> & rhs)
        { 
            if (lhs->getValue() == rhs->getValue())
            {
                if (lhs->getParent()->getNumEdges() == rhs->getParent()->getNumEdges())
                {
                    std::shared_ptr<Node> lhsNode(lhs->getChild());
                    std::shared_ptr<Node> rhsNode(rhs->getChild());
                    if (lhsNode == nullptr)
                    {
                        lhsNode = notExpandedChild(lhs, params);
                    }
                    if (rhs->getChild() == nullptr)
                    {
                        rhsNode = notExpandedChild(rhs, params);
                    }
                    return Eval::StateBetter(rhsNode->getState(), lhsNode->getState());
                }

                return lhs->getParent()->getNumEdges() < rhs->getParent()->getNumEdges();
            }
        
            return lhs->getValue() < rhs->getValue();
        });

    return *edge;
}

std::shared_ptr<Edge> Node::getHighestVisitedChild() const
{
    auto edge = std::max_element(m_edges.begin(), m_edges.end(),
        [](const std::shared_ptr<Edge> & lhs, const std::shared_ptr<Edge> & rhs)
        {
            return lhs->timesVisited() < rhs->timesVisited();
        });

    return *edge;
}

std::shared_ptr<Edge> Node::getRandomEdge()
{
    return m_edges[std::rand() % m_edges.size()];
}

std::shared_ptr<Edge> Node::getChild(const ActionAbilityPair & action)
{
    BOSS_ASSERT(m_edges.size() > 0, "Number of edges is %i", m_edges.size());

    for (auto & edge : m_edges)
    {
        if (edge->getAction().first == action.first && edge->getAction().second == action.second)
        {
            return edge;
        }
    }

    BOSS_ASSERT(false, "Tried to get edge with action %s, but it doesn't exist", action.first.getName().c_str());
    return std::make_shared<Edge>(ActionAbilityPair(ActionTypes::None, AbilityAction()), nullptr);
}