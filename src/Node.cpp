#include "Node.h"

using namespace BOSS;

Node::Node()
    : m_parent ()
    , m_state ()
    , m_timesVisited (0)
    , m_children()
{

}

Node::Node(const CombatSearchParameters & params, const GameState & state, BOSS::Node & parent)
    : m_parent (&parent)
    , m_state (state)
    , m_timesVisited (0)
    , m_children (params.getRelevantActions().size())
{

}

void Node::createChildren(ActionSetAbilities & actions, const CombatSearchParameters & params)
{
    GameState childState(m_state);

    for (int a = 0; a < actions.size(); ++a)
    {
        const int index = actions.size() - (a + 1);

        const auto & actionTargetPair = actions[index];
        ActionType action = actionTargetPair.first;
        NumUnits actionTarget = actionTargetPair.second;

        // if it's the plain CB without a target, we need to get the targets for the ability
        if (action == ActionTypes::GetSpecialAction(m_state.getRace()) && actionTarget == -1)
        {
            int sizeBefore = actions.size();

            m_state.getSpecialAbilityTargets(actions, index);

            // the ability is no longer valid, skip
            if (sizeBefore > actions.size())
            {
                --a;
                continue;
            }

            // the new target 
            actionTarget = actions.getAbilityTarget(index + (actions.size() - sizeBefore));
        }

        if (action.isAbility())
        {
            childState.doAbility(action, actionTarget);
        }
        else
        {
            childState.doAction(action);
        }

        // create node and add it as a child
        Node childNode(params, childState, *this);
        m_children.emplace_back(&childNode, 0.f);
    }
}

Node & Node::selectChild(int exploration_param) const
{
    // uniform policy
    float policyValue = 1.f / m_children.size();
    float UCBValue = exploration_param * policyValue *
        static_cast<float>(std::sqrt(m_timesVisited));

    float maxActionValue = 0;
    int maxIndex = 0;
    int index = 0;

    for (auto & edge : m_children)
    {
        // calculate UCB value and get the total value of action
        // Q(s, a) + u(s, a)
        float childUCBValue = UCBValue / (1 + edge.first->timesVisited());
        float actionValue = edge.second + childUCBValue;

        // store the index of this action
        if (maxActionValue < actionValue)
        {
            maxActionValue = actionValue;
            maxIndex = index;
        }

        ++index;
    }

    return *m_children[index].first;
}