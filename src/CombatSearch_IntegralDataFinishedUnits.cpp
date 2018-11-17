#include "CombatSearch_IntegralDataFinishedUnits.h"

using namespace BOSS;

CombatSearch_IntegralDataFinishedUnits::CombatSearch_IntegralDataFinishedUnits()
    : m_bestIntegralValue(0)
    , m_chronoBoostEntries(0)
{
    m_integralStack.push_back(IntegralDataFinishedUnits());
}

void CombatSearch_IntegralDataFinishedUnits::update(const GameState & state, const BuildOrderAbilities & buildOrder, const CombatSearchParameters & params, Timer & timer)
{   
    auto & finishedUnits = state.getFinishedUnits();
    int new_units = finishedUnits.size() - (m_integralStack.size() - (m_chronoBoostEntries + 1));

    BOSS_ASSERT(new_units >= 0, "negative new units? %d", new_units);

    // no new units or chronoboosts
    if (new_units == 0 && m_chronoBoostEntries == state.getNumberChronoBoostsCast())
    {
        return;
    }

    // go through all new units
    for (size_t index = finishedUnits.size() - new_units; index < finishedUnits.size(); ++index)
    {
        /*std::cout << "index of unit times: " << finishedUnits[index] << std::endl;
        std::cout << "index of unit: " << unitTimes[finishedUnits[index]].first << std::endl;
        std::cout << "unit name: " << state.getUnit(unitTimes[finishedUnits[index]].first).getType().getName() << std::endl;
        std::cout << "start time of unit: " << unitTimes[finishedUnits[index]].second.first << std::endl;
        std::cout << "end time of unit: " << unitTimes[finishedUnits[index]].second.second << std::endl;*/
        NumUnits unitIndex = finishedUnits[index];

        TimeType startFrame = state.getUnit(unitIndex).getStartFrame();
        TimeType finishFrame = state.getUnit(unitIndex).getFinishFrame();
        
        FracType value = Eval::ArmyResourceSumToIndex(state, index) + m_integralStack.back().eval;
        TimeType timeElapsed = finishFrame - m_integralStack.back().timeFinished;
        FracType valueToAdd = m_integralStack.back().eval * timeElapsed;
        FracType integralToThisPoint = m_integralStack.back().integral_ToThisPoint + valueToAdd;
        FracType integralUntilFrameLimit = integralToThisPoint + (params.getFrameTimeLimit() - finishFrame) * value;
        IntegralDataFinishedUnits entry(value, integralToThisPoint, integralUntilFrameLimit, startFrame, finishFrame);
        m_integralStack.push_back(entry);
    }

    // Chrono Boost entry
    //if (state.getLastAction() == ActionTypes::GetActionType("ChronoBoost") && state.getNumberChronoBoostsCast() > m_chronoBoostEntries)
    if (state.getNumberChronoBoostsCast() > m_chronoBoostEntries)
    {
        auto & chronoboosts = state.getChronoBoostTargets();
        auto & previous_entry = m_integralStack.back();
        IntegralDataFinishedUnits entry(previous_entry.eval, previous_entry.integral_ToThisPoint, previous_entry.integral_UntilFrameLimit, 
                            chronoboosts[m_chronoBoostEntries].frameCast, chronoboosts[m_chronoBoostEntries].frameCast, 'c');
        m_integralStack.push_back(entry);

        m_chronoBoostEntries++;

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
        std::cout << "time: " << timer.getElapsedTimeInMilliSec() << std::endl;
    }
}

void CombatSearch_IntegralDataFinishedUnits::print() const
{
    print(m_bestIntegralStack, m_bestIntegralGameState, m_bestIntegralBuildOrder);
}

void CombatSearch_IntegralDataFinishedUnits::printIntegralData(const size_t index, const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrderAbilities & buildOrder) const
{
    printf("%7d %8d %10.2lf %15.2lf %16.2lf   ", integral_stack[index].timeStarted, integral_stack[index].timeFinished, integral_stack[index].eval, integral_stack[index].integral_ToThisPoint, integral_stack[index].integral_UntilFrameLimit);
    std::cout << buildOrder.getNameString(2, index) << std::endl;
}

void CombatSearch_IntegralDataFinishedUnits::print(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrderAbilities & buildOrder) const
{
    BuildOrderAbilities BOEndTimes = createBuildOrderEndTimes(integral_stack, state);

    std::cout << "\nCombatSearchIntegral Results\n\n";

    std::cout << "BuildOrder sorted by start times" << std::endl;
    std::cout << buildOrder.getNameString(2) << std::endl;

    std::cout << "  Start   Finish   ArmyEval  ArmyIntegral_N   ArmyIntegral_E   BuildOrder\n";
    for (size_t i(1); i < integral_stack.size(); ++i)
    {
        printIntegralData(i, integral_stack, state, BOEndTimes);
    }
}

const BuildOrderAbilities & CombatSearch_IntegralDataFinishedUnits::getBestBuildOrder() const
{
    return m_bestIntegralBuildOrder;
}

void CombatSearch_IntegralDataFinishedUnits::pop_back()
{
    m_integralStack.pop_back();
}

void CombatSearch_IntegralDataFinishedUnits::popFinishedLastOrder(const GameState & prevState, const GameState & currState)
{
    size_t chronoBoostsToRemove = currState.getNumberChronoBoostsCast() - prevState.getNumberChronoBoostsCast();
    m_chronoBoostEntries -= chronoBoostsToRemove;

    size_t numRemove = (currState.getFinishedUnits().size() - prevState.getFinishedUnits().size()) + chronoBoostsToRemove;

    BOSS_ASSERT(numRemove < m_integralStack.size(), "Removing %d elements from integral stack when it has %d elements", numRemove, m_integralStack.size());

    for (size_t pop_times = 0; pop_times < numRemove; ++pop_times)
    {
        pop_back();
    }
}

// creates a build order based on the unit finished times
BuildOrderAbilities CombatSearch_IntegralDataFinishedUnits::createBuildOrderEndTimes(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state) const
{
    BuildOrderAbilities buildOrder;
    auto & finishedUnits = state.getFinishedUnits();
    auto & chronoBoostTargets = state.getChronoBoostTargets();
    size_t chronoboosts = 0;
    for (size_t index = 0; index < integral_stack.size() - 1; ++index)
    {
        const AbilityAction & current_ability = chronoBoostTargets[chronoboosts];
        // chronoboost
        if (integral_stack[index + 1].id == 'c')
        {
            chronoboosts++;
            if (state.getRace() == Races::Protoss)
            {
                buildOrder.add(ActionTypes::GetActionType("ChronoBoost"), current_ability);
            }
            continue;
        }
        buildOrder.add(state.getUnit(finishedUnits[index - chronoboosts]).getType());
    }

    return buildOrder;
}