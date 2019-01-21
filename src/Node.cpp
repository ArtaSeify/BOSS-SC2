#include "Node.h"
#include "Eval.h"

using namespace BOSS;

FracType Node::CURRENT_HIGHEST_VALUE = 1.f;

Node::Node(const GameState & state)
    : m_parentEdge()
    , m_state(state)
    , m_edges()
{

}

void Node::createChildrenEdges(ActionSetAbilities & legalActions, const CombatSearchParameters & params)
{
    for (auto & action : legalActions)
    {
        std::unique_ptr<Edge> edge(new Edge(action, this, nullptr));
        m_edges.push_back(edge);
    }
}

bool Node::doAction(std::unique_ptr<Edge> & edge, const CombatSearchParameters & params)
{
    const Action & action = edge->getAction();
    ActionType actionType = action.first;
    NumUnits actionTarget = action.second;

    // if it's the plain CB without a target, we need to get the targets for the ability
    // we create a new edge for each target
    if (actionType == ActionTypes::GetSpecialAction(m_state.getRace()) && actionTarget == -1)
    {
        ActionSetAbilities actions;
        actions.add(ActionTypes::GetSpecialAction(m_state.getRace()), -1);
        m_state.getSpecialAbilityTargets(actions, 0);

        // the ability is no longer valid, skip
        if (actions[0].second == -1)
        {
            return false;
        }

        // store the first new target as the edge's action. 
        // for each new target, create a new edge
        edge->setAction(actions[0]);
        actionTarget = actions[0].second;
        for (int i = 1; i < actions.size(); ++i)
        {
            std::unique_ptr<Edge> newEdge(new Edge(actions[i], this, nullptr));
            m_edges.push_back(newEdge);
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
        return false;
    }

    edge->incrementVisited();

    return true;
}

void Node::printChildren() const
{
    for (auto & edge : m_edges)
    {
        std::cout << "action: " << edge->getAction().first.getName() << std::endl;
        std::cout << "child state frame: " << edge->getChild()->getState().getCurrentFrame() << std::endl;
    }
}

std::unique_ptr<Node> Node::selectChild(int exploration_param, const CombatSearchParameters & params)
{
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
        float actionValue = (edge->getValue() / CURRENT_HIGHEST_VALUE) + childUCBValue;

        //std::cout << "times visited: " << child.timesVisited() << std::endl;

        // store the index of this action
        if (maxActionValue < actionValue)
        {
            maxActionValue = actionValue;
            maxIndex = index;
        }

        ++index;
    }

    if (m_edges[maxIndex]->getChild() == nullptr)
    {
        std::unique_ptr<Node> chosenNode(new Node(m_state));
        chosenNode->doAction(m_edges[maxIndex], params);
        return chosenNode;
    }

    return std::move(m_edges[maxIndex]->getChild());
}

std::unique_ptr<Node> Node::getHighestValueChild(const CombatSearchParameters & params)
{
    // get the node with the highest action value
    auto edge = std::max_element(m_edges.begin(), m_edges.end(),
        [this](const CombatSearchParameters & params, std::unique_ptr<Edge> & lhs, std::unique_ptr<Edge> & rhs)
        { 
            if (lhs->getValue() == rhs->getValue())
            {
                std::unique_ptr<Node> lhsNode;
                std::unique_ptr<Node> rhsNode;
                if (lhs->getChild() == nullptr)
                {
                    lhsNode = std::unique_ptr<Node>(new Node(lhs->getParent()->getState()));
                    lhsNode->doAction(lhs, params);
                }
                if (rhs->getChild() == nullptr)
                {
                    rhsNode = std::unique_ptr<Node>(new Node(rhs->getParent()->getState()));
                    rhsNode->doAction(rhs, params);
                }
                return Eval::StateBetter(rhsNode->getState(), lhsNode->getState());
            }
        
            return lhs->getValue() < rhs->getValue();
        });

    if ((*edge)->getChild() == nullptr)
    {
        std::unique_ptr<Node> childNode(new Node(m_state));
        childNode->doAction(*edge, params);
        return childNode;
    }

    return std::move((*edge)->getChild());
}

Node & Node::getRandomChild()
{
    return m_children[std::rand() % m_children.size()];
}

void Node::updateNodeValue(FracType newActionValue)
{
    //std::cout << m_action.first.getName() << " node updated" << std::endl;

    m_timesVisited++;
    m_value = m_value + ((1.f / m_timesVisited) * (newActionValue - m_value));

    if (m_value > CURRENT_HIGHEST_VALUE)
    {
        CURRENT_HIGHEST_VALUE = m_value;
    }
}

Node & Node::getChildNode(ActionType action)
{
    BOSS_ASSERT(m_children.size() > 0, "Number of children is %i", m_children.size());

    for (Node & child : m_children)
    {
        if (child.getAction().first == action)
        {
            return child;
        }
    }

    BOSS_ASSERT(false, "Tried to get node with action %s, but it doesn't exist", action.getName().c_str());
}