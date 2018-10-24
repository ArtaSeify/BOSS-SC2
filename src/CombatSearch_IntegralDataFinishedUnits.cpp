#include "CombatSearch_IntegralDataFinishedUnits.h"

using namespace BOSS;

CombatSearch_IntegralDataFinishedUnits::CombatSearch_IntegralDataFinishedUnits()
    : m_bestIntegralValue(0)
    , m_chronoBoostEntries(0)
{
    m_integralStack.push_back(IntegralDataFinishedUnits());
}

void CombatSearch_IntegralDataFinishedUnits::update(const GameState & state, const BuildOrder & buildOrder, const CombatSearchParameters & params)
{   
    auto & finishedUnits = state.getFinishedUnits();
    int new_units = finishedUnits.size() - (m_integralStack.size() - (m_chronoBoostEntries + 1));

    BOSS_ASSERT(new_units >= 0, "negative new units? %d", new_units);

    // no new units or chronoboosts
    if (new_units == 0 && m_chronoBoostEntries == state.getChronoBoostsCast())
    {
        return;
    }

    auto & unitTimes = state.getUnitTimes();
    // go through all new units
    for (size_t index = finishedUnits.size() - new_units; index < finishedUnits.size(); ++index)
    {
        /*std::cout << "index of unit times: " << finishedUnits[index] << std::endl;
        std::cout << "index of unit: " << unitTimes[finishedUnits[index]].first << std::endl;
        std::cout << "unit name: " << state.getUnit(unitTimes[finishedUnits[index]].first).getType().getName() << std::endl;
        std::cout << "start time of unit: " << unitTimes[finishedUnits[index]].second.first << std::endl;
        std::cout << "end time of unit: " << unitTimes[finishedUnits[index]].second.second << std::endl;*/
        
        size_t startFrame = unitTimes[finishedUnits[index]].second.first;
        size_t finishFrame = unitTimes[finishedUnits[index]].second.second;
        
        double value = Eval::ArmyResourceSumToIndex(state, index);
        double timeElapsed = finishFrame - m_integralStack.back().timeFinished;
        double valueToAdd = m_integralStack.back().eval * timeElapsed;
        double integralToThisPoint = m_integralStack.back().integral_ToThisPoint + valueToAdd;
        double integralUntilFrameLimit = integralToThisPoint + (params.getFrameTimeLimit() - finishFrame) * value;
        IntegralDataFinishedUnits entry(value, integralToThisPoint, integralUntilFrameLimit, startFrame, finishFrame);
        m_integralStack.push_back(entry);
    }

    // Chrono Boost entry
    if (state.getLastAction() == ActionTypes::GetActionType("ChronoBoost") && state.getChronoBoostsCast() > m_chronoBoostEntries)
    {
        m_chronoBoostEntries++;
        auto & previous_entry = m_integralStack.back();
        IntegralDataFinishedUnits entry(previous_entry.eval, previous_entry.integral_ToThisPoint, previous_entry.integral_UntilFrameLimit, state.getCurrentFrame(), state.getCurrentFrame());
        m_integralStack.push_back(entry);

        //std::cout << "chronoboost added!" << std::endl;
    }

    // we have found a new best if:
        // 1. the new army integral is higher than the previous best
        // 2. the new army integral is the same as the old best but the build order is 'better'
    if ((m_integralStack.back().integral_UntilFrameLimit > m_bestIntegralValue)
        || ((m_integralStack.back().integral_UntilFrameLimit == m_bestIntegralValue) && Eval::StateBetter(state, m_bestIntegralGameState)))
    {
        m_bestIntegralValue = m_integralStack.back().integral_UntilFrameLimit;
        m_bestIntegralStack = m_integralStack;
        m_bestIntegralBuildOrder = buildOrder;
        m_bestIntegralGameState = state;

        // print the newly found best to console
        //printIntegralData(m_integralStack.size() - 1);
        print();
    }
}

void CombatSearch_IntegralDataFinishedUnits::print() const
{
    print(m_bestIntegralStack, m_bestIntegralGameState, m_bestIntegralBuildOrder);
}

void CombatSearch_IntegralDataFinishedUnits::printIntegralData(const size_t index, const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrder & buildOrder) const
{
    printf("%7d %8d %10.2lf %15.2lf %16.2lf   ", integral_stack[index].timeStarted, integral_stack[index].timeFinished, integral_stack[index].eval, integral_stack[index].integral_ToThisPoint, integral_stack[index].integral_UntilFrameLimit);
    std::cout << buildOrder.getNameString(state, 2, index) << std::endl;
}

void CombatSearch_IntegralDataFinishedUnits::print(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrder & buildOrder) const
{
    BuildOrder BOEndTimes = createBuildOrderEndTimes(integral_stack, state);

    std::cout << "\nCombatSearchIntegral Results\n\n";

    std::cout << "BuildOrder sorted by start times" << std::endl;
    std::cout << buildOrder.getNameString(state, 2) << std::endl;

    std::cout << "  Start   Finish   ArmyEval  ArmyIntegral_N   ArmyIntegral_E   BuildOrder\n";
    for (size_t i(1); i < integral_stack.size(); ++i)
    {
        printIntegralData(i, integral_stack, state, BOEndTimes);
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

void CombatSearch_IntegralDataFinishedUnits::popFinishedLastOrder(const GameState & prevState, const GameState & currState)
{
    size_t chronoBoostsToRemove = currState.getChronoBoostsCast() - prevState.getChronoBoostsCast();
    m_chronoBoostEntries -= chronoBoostsToRemove;

    size_t numRemove = (currState.getFinishedUnits().size() - prevState.getFinishedUnits().size()) + chronoBoostsToRemove;

    BOSS_ASSERT(numRemove < m_integralStack.size(), "Removing %d elements from integral stack when it has %d elements", numRemove, m_integralStack.size());

    for (size_t pop_times = 0; pop_times < numRemove; ++pop_times)
    {
        pop_back();
    }
}

// needs finishing
BuildOrder CombatSearch_IntegralDataFinishedUnits::createBuildOrderEndTimes(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state) const
{
    BuildOrder buildOrder;
    auto & finishedUnits = state.getFinishedUnits();
    auto & unitTimes = state.getUnitTimes();
    auto & chronoBoostTargets = state.getChronoBoostTargets();
    size_t chronoboosts = 0;
    for (size_t index = 0; index < integral_stack.size() - 1; ++index)
    {
        if (integral_stack[index + 1].timeStarted == integral_stack[index + 1].timeFinished)
        {
            buildOrder.add(ActionTypes::GetActionType("ChronoBoost"), chronoBoostTargets[chronoboosts++]);
            continue;
        }

        buildOrder.add(state.getUnit(unitTimes[finishedUnits[index - chronoboosts]].first).getType());
    }

    return buildOrder;
}