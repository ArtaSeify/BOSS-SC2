#include "Node.h"
#include "Edge.h"

using namespace BOSS;

int Edge::NODE_VISITS_BEFORE_EXPAND = 1;
bool Edge::USE_MAX_VALUE = true;
FracType Edge::HIGHEST_VALUE_FOUND = 1.f;
FracType Edge::MIXING_VALUE = 0.0;
int Edge::VIRTUAL_LOSS = 5;

Edge::Edge()
    : m_visits(0)
    , m_virtualLoss(0)
    , m_valueSimulations(0)
    , m_valueNetwork(0)
    , m_value(0)
    , m_average(0)
    , m_max(0)
    , m_policy(0)
    , m_action(ActionAbilityPair(ActionTypes::None, AbilityAction()))
    , m_child()
    , m_parent()
    , m_lock()
{

}

Edge::Edge(const ActionAbilityPair & action, std::shared_ptr<Node> parent)
    : m_visits(0)
    , m_virtualLoss(0)
    , m_valueSimulations(0)
    , m_valueNetwork(0)
    , m_value(0)
    , m_average(0)
    , m_max(0)
    , m_policy(0)
    , m_action(action)
    , m_child()
    , m_parent(parent)
    , m_lock()
{

}

void Edge::cleanUp()
{
    std::scoped_lock sl(m_lock);

    //std::cout << "cleaning up edge: " << m_action.first.getName() << std::endl;
    if (m_child != nullptr)
    {
        //std::cout << "cleaning up child of this edge" << std::endl;
        m_child->cleanUp(0);
        //std::cout << "child use count: " << m_child.use_count() << std::endl;
        m_child.reset();
    }
    //std::cout << "parent use count: " << m_parent.use_count() << std::endl;
    m_parent.reset();
}

void Edge::reset()
{
    std::scoped_lock sl(m_lock);

    //m_total = 0;
    m_average = 0;
    m_max = 0;
    m_visits = 0;
    m_virtualLoss = 0;
    m_policy = 0;
}

void Edge::updateEdge(FracType simulationValue, FracType networkValue)
{
    std::scoped_lock sl(m_lock);

    m_max = std::max(simulationValue, m_max);
    
    if (USE_MAX_VALUE)
    {
        bool newEdgeValue = false;
        if (m_valueSimulations < simulationValue)
        {
            m_valueSimulations = simulationValue;
            if (MIXING_VALUE < 1)
            {
                newEdgeValue = true;
            }
        }

        if (m_valueNetwork < networkValue)
        {
            m_valueNetwork = networkValue;
            if (MIXING_VALUE > 0)
            {
                newEdgeValue = true;
            }
        }

        if (newEdgeValue)
        {
            setNewEdgeValue();
        }
    }
    /*else
    {
        m_average = m_averageValue + ((1.f / m_visits +) * (simulationValue - m_averageValue));
        m_valueSimulations = m_averageValue;
        setNewEdgeValue();
    }*/
}

bool Edge::setChild(const std::shared_ptr<Node>& child)
{ 
    std::scoped_lock sl(m_lock);

    // Another thread set the child already, just return
    if (m_child != nullptr)
    {
        return false;
    }
    assert(m_child == nullptr);
    m_child = child;
    return true;
}

void Edge::setNewEdgeValue()
{
    std::scoped_lock sl(m_lock);

    m_value = (MIXING_VALUE * m_valueNetwork) + ((1 - MIXING_VALUE) * m_valueSimulations);

    if (m_value > HIGHEST_VALUE_FOUND)
    {
        HIGHEST_VALUE_FOUND = m_value;
    }
}

void Edge::printValues() const
{
    std::scoped_lock sl(m_lock);

    std::cout << "Edge action: " << m_action.first.getName() << std::endl;
    std::cout << "Network Value: " << m_valueNetwork << std::endl;
    std::cout << "Network policy: " << m_policy << std::endl;
    std::cout << "Simulations value: " << m_valueSimulations << " with " << m_visits << " simulations " << std::endl;
    std::cout << "Edge Value: " << m_value << std::endl;
    std::cout << "Virtual Loss: " << m_virtualLoss << std::endl;
    std::cout << std::endl;
}

FracType Edge::calculatePUCT(FracType exploration, FracType totalVisitsSqrt, int normalization) const
{
    std::scoped_lock sl(m_lock);

    assert(m_policy >= 0.f);

    // value normalization should put value in range [0, 1]
    //FracType normalizedValue = (FracType)m_max / normalization;
    FracType normalizedValue = m_max / normalization;

    if (abs(normalizedValue) > 1.001)
    {
        return std::numeric_limits<FracType>::lowest();
    }

    assert(abs(normalizedValue) <= 1.001);

    return normalizedValue + (exploration * m_policy * totalVisitsSqrt / (1 + m_visits + m_virtualLoss));
}

void Edge::visited()
{
    std::scoped_lock sl(m_lock);

    ++m_visits;
    m_virtualLoss += VIRTUAL_LOSS;
    //m_average = m_total / ((FracType)m_visits + m_virtualLoss);
}