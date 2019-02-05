#include "Node.h"
#include "Eval.h"

using namespace BOSS;

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
        GameState testState(m_state);
        if (action.first.isAbility())
        {
            testState.doAbility(action.first, action.second);
        }

        else
        {
            testState.doAction(action.first);
        }

        if (testState.getCurrentFrame() > params.getFrameTimeLimit())
        {
            continue;
        }

        // action is valid, so create an edge
        m_edges.push_back(std::make_shared<Edge>(action, thisNode));
    }

    // no edges were created, so this is a terminal node
    if (m_edges.size() == 0)
    {
        isTerminalNode = true;
    }
}

bool Node::doAction(std::shared_ptr<Edge> edge, const CombatSearchParameters & params)
{
    const Action & action = edge->getAction();
    ActionType actionType = action.first;
    NumUnits actionTarget = action.second;

    if (actionType.isAbility())
    {
        m_state.doAbility(actionType, actionTarget);
    }
    else
    {
        m_state.doAction(actionType);
    }

    // if we go over the frame time limit, this node is invalid
    if (m_state.getCurrentFrame() > params.getFrameTimeLimit())
    {
        BOSS_ASSERT(false, "invalid action given to doAction()");
        return false;
    }

    if (edge->timesVisited() == Edge::NODE_VISITS_BEFORE_EXPAND - 1)
    {
        //std::shared_ptr<Node> newNode = std::make_shared<Node>(m_state, edge);
        //edge->setChild(newNode);
        edge->setChild(shared_from_this());
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
        ActionSetAbilities legalActions;
        legalActions.add(action.first, action.second);
        m_state.getSpecialAbilityTargets(legalActions, 0);
        // Chronoboost is no longer a legal action
        if (legalActions[0].second == -1)
        {
            return false;
        }

        // if only one target, set that to be the target
        if (legalActions.size() == 1)
        {
            actionTarget = legalActions[0].second;
        }

        // otherwise randomly choose between the targets
        else
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, legalActions.size() - 1);
            actionTarget = legalActions[dis(gen)].second;
        }
    }
    
    if (actionType.isAbility())
    {
        m_state.doAbility(actionType, actionTarget);
    }
    else
    {
        m_state.doAction(actionType);
    }

    // if we go over the frame time limit, this node is invalid
    if (m_state.getCurrentFrame() > params.getFrameTimeLimit())
    {
        isTerminalNode = true;
        return false;
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

std::shared_ptr<Edge> Node::selectChildEdge(FracType exploration_param, const CombatSearchParameters & params) const
{
    BOSS_ASSERT(m_edges.size() > 0, "selectChildEdge called when there are no edges.");

    // uniform policy
    float policyValue = 1.f / m_edges.size();

    int totalChildVisits = 0;
    for (auto & edge : m_edges)
    {
        totalChildVisits += edge->timesVisited();
    }

    float UCBValue = exploration_param * policyValue *
        static_cast<float>(std::sqrt(totalChildVisits));

    float maxActionValue = 0;
    int maxIndex = 0;
    int index = 0;

    for (auto & edge : m_edges)
    {
        // calculate UCB value and get the total value of action
        // Q(s, a) + u(s, a)
        // we normalize the action value to a range of [0, 1] using the highest
        // value of the search thus far. 
        float childUCBValue = UCBValue / (1 + edge->timesVisited());
        float actionValue = (edge->getValue() / Edge::CURRENT_HIGHEST_VALUE) + childUCBValue;

        //std::cout << "edge value: " << edge->getValue() / Edge::CURRENT_HIGHEST_VALUE << ". UCT value: " << childUCBValue << std::endl;

        //std::cout << "times visited: " << child.timesVisited() << std::endl;

        // store the index of this action
        if (maxActionValue < actionValue)
        {
            maxActionValue = actionValue;
            maxIndex = index;
        }

        ++index;
    }

    return m_edges[maxIndex];
}

std::shared_ptr<Node> Node::notExpandedChild(std::shared_ptr<Edge> edge, const CombatSearchParameters & params) const
{
    // create a temporary node
    std::shared_ptr<Node> node = std::make_shared<Node>(m_state, edge);
    node->doAction(edge, params);
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

std::shared_ptr<Edge> Node::getChild(const ActionType & action)
{
    BOSS_ASSERT(m_edges.size() > 0, "Number of edges is %i", m_edges.size());

    for (auto & edge : m_edges)
    {
        if (edge->getAction().first == action)
        {
            return edge;
        }
    }

    BOSS_ASSERT(false, "Tried to get edge with action %s, but it doesn't exist", action.getName().c_str());
    return std::make_shared<Edge>();
}