#include "Node.h"

using namespace BOSS;

Node::Node()
    : //m_parent ()
     m_state ()
    , m_action()
    , m_timesVisited (0)
    , m_children()
    , m_value (0.f)
{

}

Node::Node(const GameState & state)
    : //m_parent()
     m_state(state)
    , m_action()
    , m_timesVisited(0)
    , m_children()
    , m_value(0.f)
{

}

Node::Node(const GameState & state, BOSS::Node * parent)
    : m_parent (parent)
    , m_state (state)
    , m_action ()
    , m_timesVisited (0)
    //, m_children ()
    , m_value (0.f)
{

}

void Node::createChildren(ActionSetAbilities & actions)
{
    // create all the children
    for (int a = 0; a < actions.size(); ++a)
    {
        const int index = actions.size() - (a + 1);

        Node * childNode = new Node(m_state, this);

        if (childNode->doAction(actions, index))
        {
            m_children.push_back(childNode);
        }
    }

    std::cout << "done with creating children" << std::endl;
}

bool Node::doAction(ActionSetAbilities & actions, int index)
{
    const auto & actionTargetPair = actions[index];
    ActionType action = actionTargetPair.first;
    NumUnits actionTarget = actionTargetPair.second;

    // if it's the plain CB without a target, we need to get the targets for the ability
    if (action == ActionTypes::GetSpecialAction(m_state.getRace()) && actionTarget == -1)
    {
        int sizeBefore = actions.size();

        m_state.getSpecialAbilityTargets(actions, index);

        // the new target 
        actionTarget = actions.getAbilityTarget(index + (actions.size() - sizeBefore));

        // the ability is no longer valid, skip
        if (actionTarget == -1)
        {
            return false;
        }
    }
    
    std::cout << "doing action: " << action.getName() << std::endl;

    if (action.isAbility())
    {
        m_state.doAbility(action, actionTarget);
    }
    else
    {
        m_state.doAction(action);
    }

    // set the action of this child node
    setAction(actions[index]);

    return true;
}

Node & Node::selectChild(int exploration_param) const
{
    // uniform policy
    float policyValue = 1.f / m_children.size();

    int totalEdgeVisits = 0;
    for (auto & child : m_children)
    {
        totalEdgeVisits += child->timesVisited();
    }

    float UCBValue = exploration_param * policyValue *
        static_cast<float>(std::sqrt(totalEdgeVisits));

    float maxActionValue = 0;
    int maxIndex = 0;
    int index = 0;

    for (auto & child : m_children)
    {
        // calculate UCB value and get the total value of action
        // Q(s, a) + u(s, a)
        float childUCBValue = UCBValue / (1 + child->timesVisited());
        float actionValue = child->getValue() + childUCBValue;

        // store the index of this action
        if (maxActionValue < actionValue)
        {
            maxActionValue = actionValue;
            maxIndex = index;
        }

        ++index;
    }

    return *m_children[index];
}

Node & Node::getRandomChild() const
{
    return *m_children[std::rand() % m_children.size()];
}

void Node::updateNodeValue(FracType newActionValue)
{
    m_timesVisited++;
    m_value = m_value + ((1.f / m_timesVisited) * (newActionValue - m_value));
}