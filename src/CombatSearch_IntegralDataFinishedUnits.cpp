#include "CombatSearch_IntegralDataFinishedUnits.h"

using namespace BOSS;

CombatSearch_IntegralDataFinishedUnits::CombatSearch_IntegralDataFinishedUnits()
    : m_bestIntegralValue(0)
{
    m_integralStack.push_back(IntegralDataFinishedUnits());
}

void CombatSearch_IntegralDataFinishedUnits::update(const GameState & state, const BuildOrder & buildOrder, const CombatSearchParameters & params)
{
    getNewFinishTimes(state, buildOrder);
    double value = Eval::ArmyCompletedResourceSum(state);
    double timeElapsed = state.getCurrentFrame() - m_integralStack.back().timeAdded;
    double valueToAdd = m_integralStack.back().eval * timeElapsed;
    double integralToThisPoint = m_integralStack.back().integral_ToThisPoint + valueToAdd;
    double integralUntilFrameLimit = integralToThisPoint + (params.getFrameTimeLimit() - state.getCurrentFrame()) * value;
    IntegralDataFinishedUnits entry(value, integralToThisPoint, integralUntilFrameLimit, state.getCurrentFrame(), 0);
    m_integralStack.push_back(entry);

    // we have found a new best if:
    // 1. the new army integral is higher than the previous best
    // 2. the new army integral is the same as the old best but the build order is 'better'
    if ((m_integralStack.back().integral_UntilFrameLimit > m_bestIntegralValue)
        || ((m_integralStack.back().integral_UntilFrameLimit == m_bestIntegralValue) && Eval::BuildOrderBetter(buildOrder, m_bestIntegralBuildOrder)))
    {
        m_bestIntegralValue = m_integralStack.back().integral_UntilFrameLimit;
        m_bestIntegralStack = m_integralStack;
        m_bestIntegralBuildOrder = buildOrder;
        m_bestIntegralGameState = state;

        // print the newly found best to console
        //printIntegralData(m_integralStack.size() - 1);
        print(m_integralStack, state);
    }
}

void CombatSearch_IntegralDataFinishedUnits::printIntegralData(const size_t index) const
{
    printf("%7d %10.2lf %15.2lf %16.2lf   ", m_bestIntegralStack[index].timeAdded, m_bestIntegralStack[index].eval, m_bestIntegralStack[index].integral_ToThisPoint, m_bestIntegralStack[index].integral_UntilFrameLimit);
    std::cout << m_bestIntegralBuildOrder.getNameString(m_bestIntegralGameState, 2, index) << std::endl;
}

void CombatSearch_IntegralDataFinishedUnits::print() const
{
    std::cout << "\nFinal CombatSearchIntegral Results\n\n";
    std::cout << "  Frame   ArmyEval  ArmyIntegral_N   ArmyIntegral_E   BuildOrder\n";

    for (size_t i(1); i < m_bestIntegralStack.size(); ++i)
    {
        printIntegralData(i);
    }
}

void CombatSearch_IntegralDataFinishedUnits::printIntegralData(const size_t index, const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state) const
{
    printf("%7d %10.2lf %15.2lf %16.2lf   ", integral_stack[index].timeAdded, integral_stack[index].eval, integral_stack[index].integral_ToThisPoint, integral_stack[index].integral_UntilFrameLimit);
    std::cout << m_bestIntegralBuildOrder.getNameString(state, 2, index) << std::endl;
}

void CombatSearch_IntegralDataFinishedUnits::print(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state) const
{
    std::cout << "\CombatSearchIntegral Results\n\n";
    std::cout << "  Frame   ArmyEval  ArmyIntegral_N   ArmyIntegral_E   BuildOrder\n";

    for (size_t i(1); i < integral_stack.size(); ++i)
    {
        printIntegralData(i, integral_stack, state);
    }
}

void CombatSearch_IntegralDataFinishedUnits::getNewFinishTimes(const GameState & state, const BuildOrder & buildOrder)
{
    auto & units = state.getUnitsFinished();
    for (size_t entry_index(1); entry_index < m_integralStack.size(); ++entry_index)
    {
        if (m_integralStack[entry_index].timeFinished == 0)
        {
            for (size_t units_index(0); units_index < units.size(); ++units_index)
            {
                if (units[units_index].first.getType() == buildOrder[entry_index - 1])
                {
                    m_integralStack[entry_index].timeFinished = units[units_index].second;
                    break;
                }
            }
        }
    }
}

const BuildOrder & CombatSearch_IntegralDataFinishedUnits::getBestBuildOrder() const
{
    return m_bestIntegralBuildOrder;
}

void CombatSearch_IntegralDataFinishedUnits::pop_back()
{
    m_integralStack.pop_back();
}