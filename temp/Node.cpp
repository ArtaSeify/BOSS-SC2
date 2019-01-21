#include "Node.h"
#include "Eval.h"

using namespace BOSS;

FracType Node::CURRENT_HIGHEST_VALUE = 1.f;

Node::Node()
    : m_parent ()
    , m_state ()
    , m_action()
    , m_timesVisited (0)
    , m_children()
    , m_value (0.f)
{

}

Node::Node(const GameState & state)
    : m_parent()
    , m_state(state)
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
    , m_children ()
    , m_value (0.f)
{

}

void Node::createChildren(ActionSetAbilities & actions, const CombatSearchParameters & params)
{
    // create all the children
    for (int a = 0; a < actions.size(); ++a)
    {
        const int index = actions.size() - (a + 1);

        Node childNode(m_state, this);

        if (childNode.doAction(actions, index, params))
        {
            m_children.push_back(childNode);
        }
    }
}

bool Node::doAction(ActionSetAbilities & actions, int index, const CombatSearchParameters & params)
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

    if (action.isAbility())
    {
        m_state.doAbility(action, actionTarget);
    }
    else
    {
        m_state.doAction(action);
    }

    // if we go over the frame time limit, this node is invalid
    if (m_state.getCurrentFrame() > params.getFrameTimeLimit())
    {
        return false;
    }

    // set the action of this child node
    setAction(actions[index]);

    return true;
}

Node & Node::selectChild(int exploration_param)
{
    // uniform policy
    float policyValue = 1.f / m_children.size();

    int totalChildVisits = 0;
    for (auto & child : m_children)
    {
        totalChildVisits += child.timesVisited();
    }

    float UCBValue = exploration_param * policyValue *
        static_cast<float>(std::sqrt(totalChildVisits));

    float maxActionValue = 0;
    int maxIndex = 0;
    int index = 0;

    for (auto & child : m_children)
    {
        // calculate UCB value and get the total value of action
        // Q(s, a) + u(s, a)
        // we normalize the action value to a range of [0, 1] using the highest
        // value of the search thus far. 
        float childUCBValue = UCBValue / (1 + child.timesVisited());
        float actionValue = (child.getValue() / CURRENT_HIGHEST_VALUE) + childUCBValue;

        //std::cout << "times visited: " << child.timesVisited() << std::endl;

        // store the index of this action
        if (maxActionValue < actionValue)
        {
            maxActionValue = actionValue;
            maxIndex = index;
        }

        ++index;
    }

    //std::cout << std::endl;

    return m_children[maxIndex];
}

Node & Node::getChildHighestValue()
{
    // get the nodde with the highest action value
    auto child = std::max_element(m_children.begin(), m_children.end(),
        [this](const Node & lhs, const Node & rhs) 
        { 
            if (lhs.getValue() == rhs.getValue())
            {
                return Eval::StateBetter(rhs.getState(), lhs.getState());
            }
        
            return lhs.getValue() < rhs.getValue();
        });

    return *child;
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