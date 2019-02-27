/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch_IntegralDataFinishedUnits.h"

using namespace BOSS;

CombatSearch_IntegralDataFinishedUnits::CombatSearch_IntegralDataFinishedUnits()
    : m_chronoBoostEntries(0)
    , m_bestIntegralValue(0)
    , m_bestIntegralBuildOrder(BuildOrderAbilities())
    , m_bestIntegralGameState(GameState())
{
    m_integralStack.push_back(IntegralDataFinishedUnits());
    m_bestIntegralStack = m_integralStack;
}

void CombatSearch_IntegralDataFinishedUnits::update(const GameState & state, const BuildOrderAbilities & buildOrder, const CombatSearchParameters & params, Timer & timer, bool useTieBreaker)
{
    auto & finishedUnits = state.getFinishedUnits();
    int new_units = int(finishedUnits.size() - (m_integralStack.size() - (m_chronoBoostEntries + 1)));

    BOSS_ASSERT(new_units >= 0, "negative new units? %d", new_units);

    //// no new units or chronoboosts
    //if (new_units == 0 && m_chronoBoostEntries == state.getNumberChronoBoostsCast())
    //{
    //    return;
    //}

    auto & chronoboosts = state.getChronoBoostTargets();

    // go through all new units
    for (int index = int(finishedUnits.size() - new_units); index < finishedUnits.size(); ++index)
    {
        /*std::cout << "index of unit times: " << finishedUnits[index] << std::endl;
        std::cout << "index of unit: " << unitTimes[finishedUnits[index]].first << std::endl;
        std::cout << "unit name: " << state.getUnit(unitTimes[finishedUnits[index]].first).getType().getName() << std::endl;
        std::cout << "start time of unit: " << unitTimes[finishedUnits[index]].second.first << std::endl;
        std::cout << "end time of unit: " << unitTimes[finishedUnits[index]].second.second << std::endl;*/
        NumUnits unitIndex = finishedUnits[index];

        TimeType unitStartFrame = state.getUnit(unitIndex).getStartFrame();
        TimeType unitFinishFrame = state.getUnit(unitIndex).getFinishFrame();

        // Chrono Boost entries
        while (state.getNumberChronoBoostsCast() > m_chronoBoostEntries)
        {
            TimeType cbFinishFrame = chronoboosts[m_chronoBoostEntries].frameCast;

            // if chronoboost was cast before the unit, we add it first
            if (cbFinishFrame < unitFinishFrame)
            {
                addChronoBoostEntry(chronoboosts[m_chronoBoostEntries].frameCast, cbFinishFrame, params);
            }

            // since the chronoboost was cast after thiis unit was finished, we add the unit first
            else
            {
                break;
            }
        }

        addUnitEntry(state, unitIndex, unitStartFrame, unitFinishFrame, params);
    }

    if (state.getNumberChronoBoostsCast() > m_chronoBoostEntries)
    {
        auto & chronoboosts = state.getChronoBoostTargets();
        TimeType cbStartFrame = chronoboosts[m_chronoBoostEntries].frameCast;
        TimeType cbFinishFrame = chronoboosts[m_chronoBoostEntries].frameCast;

        addChronoBoostEntry(cbStartFrame, cbFinishFrame, params);
    }
    
    // we have found a new best if:
        // 1. the new army integral is higher than the previous best
        // 2. the new army integral is the same as the old best but the build order is 'better'
    if ((m_integralStack.back().integral_UntilFrameLimit > m_bestIntegralValue) ||
        (!useTieBreaker && (m_integralStack.back().integral_UntilFrameLimit >= m_bestIntegralValue))
        || (useTieBreaker && (m_integralStack.back().integral_UntilFrameLimit == m_bestIntegralValue) && Eval::StateBetter(state, m_bestIntegralGameState)))
    {
        m_bestIntegralValue = m_integralStack.back().integral_UntilFrameLimit;
        m_bestIntegralStack = m_integralStack;
        m_bestIntegralBuildOrder = buildOrder;
        m_bestIntegralGameState = state;

        // print the newly found best to console
        //printIntegralData(m_integralStack.size() - 1);
        
        if (params.getPrintNewBest())
        {
            print();
            std::cout << "time: " << timer.getElapsedTimeInMilliSec() << std::endl;
        }
    }
}

void CombatSearch_IntegralDataFinishedUnits::addUnitEntry(const GameState & state, int unitIndex, TimeType startFrame, TimeType endFrame, const CombatSearchParameters & params)
{
    FracType value = Eval::ArmyResourceUnit(state, unitIndex) + m_integralStack.back().eval;
    TimeType timeElapsed = endFrame - m_integralStack.back().timeFinished;
    FracType valueToAdd = m_integralStack.back().eval * timeElapsed;
    FracType integralToThisPoint = m_integralStack.back().integral_ToThisPoint + valueToAdd;
    FracType integralUntilFrameLimit = integralToThisPoint + (params.getFrameTimeLimit() - endFrame) * value;

    IntegralDataFinishedUnits entry(value, integralToThisPoint, integralUntilFrameLimit, startFrame, endFrame);
    m_integralStack.push_back(entry);
}

void CombatSearch_IntegralDataFinishedUnits::addChronoBoostEntry(TimeType startFrame, TimeType endFrame, const CombatSearchParameters & params)
{
    FracType value = m_integralStack.back().eval;
    TimeType timeElapsed = endFrame - m_integralStack.back().timeFinished;
    FracType valueToAdd = m_integralStack.back().eval * timeElapsed;
    FracType integralToThisPoint = m_integralStack.back().integral_ToThisPoint + valueToAdd;
    FracType integralUntilFrameLimit = integralToThisPoint + (params.getFrameTimeLimit() - endFrame) * value;

    IntegralDataFinishedUnits entry(value, integralToThisPoint, integralUntilFrameLimit, startFrame, endFrame, 'c');
    m_integralStack.push_back(entry);

    m_chronoBoostEntries++;
}

void CombatSearch_IntegralDataFinishedUnits::print() const
{
    print(m_bestIntegralStack, m_bestIntegralGameState, m_bestIntegralBuildOrder);
}

void CombatSearch_IntegralDataFinishedUnits::printIntegralData(const int index, const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & /*!!! PROBLEM UNUSED state */, const BuildOrderAbilities & buildOrder) const
{
    printf("%7d %8d %10.2lf %15.2lf %16.2lf   ", (int)integral_stack[index].timeStarted, (int)integral_stack[index].timeFinished, integral_stack[index].eval, integral_stack[index].integral_ToThisPoint, integral_stack[index].integral_UntilFrameLimit);
    std::cout << buildOrder.getNameString(2, index) << std::endl;
}

void CombatSearch_IntegralDataFinishedUnits::print(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrderAbilities & buildOrder) const
{
    BuildOrderAbilities BOEndTimes = createBuildOrderEndTimes(integral_stack, state);

    std::cout << "\nSearch Results\n\n";

    std::cout << "BuildOrder sorted by start times" << std::endl;
    std::cout << buildOrder.getNameString(2) << std::endl;

    std::cout << "  Start   Finish   ArmyEval  ArmyIntegral_N   ArmyIntegral_E   BuildOrder\n";
    for (int i(1); i < integral_stack.size(); ++i)
    {
        printIntegralData(i, integral_stack, state, BOEndTimes);
    }
}

void CombatSearch_IntegralDataFinishedUnits::writeToFile(const std::string & dir, const std::string & filename)
{
    std::stringstream ss;

    // build order sorted by start times
    ss << m_bestIntegralBuildOrder.getNameString(2) << "\n";

    // build order sorted by end times
    BuildOrderAbilities BOEndTimes = createBuildOrderEndTimes(m_bestIntegralStack, m_bestIntegralGameState);
    ss << BOEndTimes.getNameString(2) << "\n";

    for (int i = 1; i < m_bestIntegralStack.size(); ++i)
    {
        auto & element = m_bestIntegralStack[i];
        ss << element.timeStarted << " " << element.timeFinished << " " << element.eval << " "
            << element.integral_ToThisPoint << " " << element.integral_UntilFrameLimit << " ";

        if (i <= BOEndTimes.size())
        {
            ss << BOEndTimes[i - 1].first.getName();
            if (BOEndTimes[i - 1].first.getName() == "ChronoBoost")
            {
                ss << "_" << BOEndTimes[i - 1].second.targetType.getName();
            }
        }

        ss << "\n";
    }

    std::ofstream file(dir + "/" + filename + "_BuildOrder.txt");
    file << ss.str();
    file.close();
}

const BuildOrderAbilities & CombatSearch_IntegralDataFinishedUnits::getBestBuildOrder() const
{
    return m_bestIntegralBuildOrder;
}

void CombatSearch_IntegralDataFinishedUnits::pop_back()
{
    assert(!m_integralStack.empty());
    m_integralStack.pop_back();
}

void CombatSearch_IntegralDataFinishedUnits::clear()
{
    m_integralStack.clear();
    m_integralStack.push_back(IntegralDataFinishedUnits());
}

void CombatSearch_IntegralDataFinishedUnits::popFinishedLastOrder(const GameState & prevState, const GameState & currState)
{
    int chronoBoostsToRemove = currState.getNumberChronoBoostsCast() - prevState.getNumberChronoBoostsCast();
    m_chronoBoostEntries -= chronoBoostsToRemove;

    int numRemove = (int)((currState.getFinishedUnits().size() - prevState.getFinishedUnits().size()) + chronoBoostsToRemove);

    BOSS_ASSERT(numRemove < (int)m_integralStack.size(), "Removing %d elements from integral stack when it has %d elements", numRemove, m_integralStack.size());

    for (int pop_times = 0; pop_times < numRemove; ++pop_times)
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
    int chronoboosts = 0;
    for (int index = 0; index < int(integral_stack.size() - 1); ++index)
    {
        // chronoboost
        if (integral_stack[index + 1].id == 'c')
        {
            const AbilityAction & ability = chronoBoostTargets[chronoboosts];
            chronoboosts++;
            buildOrder.add(ActionTypes::GetSpecialAction(state.getRace()), ability);
            continue;
        }
        buildOrder.add(state.getUnit(finishedUnits[index - chronoboosts]).getType());
    }

    return buildOrder;
}
