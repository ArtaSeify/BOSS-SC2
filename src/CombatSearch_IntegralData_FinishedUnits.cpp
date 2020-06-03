/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch_IntegralData_FinishedUnits.h"

using namespace BOSS;

CombatSearch_IntegralData_FinishedUnits::CombatSearch_IntegralData_FinishedUnits()
    : m_chronoBoostEntries(0)
    , m_bestIntegralValue(0)
    , m_bestIntegralBuildOrder(BuildOrder())
    , m_bestIntegralGameState(GameState())
{
    m_integralStack.push_back(IntegralData_FinishedUnits());
    m_bestIntegralStack = m_integralStack;
}

void CombatSearch_IntegralData_FinishedUnits::update(const GameState & state, const BuildOrder & buildOrder, const CombatSearchParameters & params, Timer & timer, bool useTieBreaker)
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
        // save build order only during integral search
        if (useTieBreaker)
        {
            m_bestIntegralBuildOrder = buildOrder;
            m_bestIntegralGameState = state;
        }

        // print the newly found best to console
        //printIntegralData(m_integralStack.size() - 1);
        
        if (params.getPrintNewBest())
        {
            print();
            std::cout << "time: " << timer.getElapsedTimeInMilliSec() << std::endl;
        }
    }
}

void CombatSearch_IntegralData_FinishedUnits::addUnitEntry(const GameState & state, int unitIndex, TimeType startFrame, TimeType endFrame, const CombatSearchParameters & params)
{
    FracType value = m_integralStack.back().eval;
    
    if (params.getEnemyUnits().size() > 0)
    {
        value += Eval::UnitValueWithOpponent(state, state.getUnit(unitIndex).getType(), params);
    }
    else
    {
       value += Eval::UnitValue(state, state.getUnit(unitIndex).getType());
    }
    
    FracType integralToThisPoint = 0;
    FracType integralUntilFrameLimit = 0;

    if (params.getMaximizeValue())
    {
        integralToThisPoint = value;
        integralUntilFrameLimit = value;
    }

    else
    {
        TimeType timeElapsed = endFrame - m_integralStack.back().timeFinished;
        FracType valueToAdd = m_integralStack.back().eval * timeElapsed;
        integralToThisPoint = m_integralStack.back().integral_ToThisPoint + valueToAdd;
        integralUntilFrameLimit = integralToThisPoint + (params.getFrameLimit() - endFrame) * value;
    }

    IntegralData_FinishedUnits entry(value, integralToThisPoint, integralUntilFrameLimit, startFrame, endFrame);
    m_integralStack.push_back(entry);
}

void CombatSearch_IntegralData_FinishedUnits::addChronoBoostEntry(TimeType startFrame, TimeType endFrame, const CombatSearchParameters & params)
{
    FracType value = m_integralStack.back().eval;

    FracType integralToThisPoint = 0;
    FracType integralUntilFrameLimit = 0;

    if (params.getMaximizeValue())
    {
        integralToThisPoint = value;
        integralUntilFrameLimit = value;
    }

    else
    {
        TimeType timeElapsed = endFrame - m_integralStack.back().timeFinished;
        FracType valueToAdd = m_integralStack.back().eval * timeElapsed;
        integralToThisPoint = m_integralStack.back().integral_ToThisPoint + valueToAdd;
        integralUntilFrameLimit = integralToThisPoint + (params.getFrameLimit() - endFrame) * value;
    }

    IntegralData_FinishedUnits entry(value, integralToThisPoint, integralUntilFrameLimit, startFrame, endFrame, 'c');
    m_integralStack.push_back(entry);

    m_chronoBoostEntries++;
}

void CombatSearch_IntegralData_FinishedUnits::print(const BuildOrder & buildOrder) const
{
    if (buildOrder.size() == 0)
    {
        print(m_bestIntegralStack, m_bestIntegralGameState, m_bestIntegralBuildOrder);
    }
    else
    {
        print(m_bestIntegralStack, m_bestIntegralGameState, buildOrder);
    }
}

void CombatSearch_IntegralData_FinishedUnits::printIntegralData(int index, const std::vector<IntegralData_FinishedUnits> & integral_stack, const GameState & /*!!! PROBLEM UNUSED state */, const BuildOrder & buildOrder) const
{
    printf("%7d %8d %10.2lf %15.2lf %16.2lf   ", (int)integral_stack[index].timeStarted, (int)integral_stack[index].timeFinished, integral_stack[index].eval, integral_stack[index].integral_ToThisPoint, integral_stack[index].integral_UntilFrameLimit);
    std::cout << buildOrder.getNameString(2, index) << std::endl;
}

void CombatSearch_IntegralData_FinishedUnits::print(const std::vector<IntegralData_FinishedUnits> & integral_stack, const GameState & state, const BuildOrder & buildOrder) const
{
    BuildOrder BOEndTimes = createBuildOrderEndTimes(integral_stack, state);

    std::cout << "\nSearch Results\n\n";

    std::cout << "BuildOrder sorted by start times" << std::endl;
    std::cout << buildOrder.getNameString(2) << std::endl;

    std::cout << "  Start   Finish   ArmyEval  ArmyIntegral_N   ArmyIntegral_E   BuildOrder\n";
    for (int i(1); i < integral_stack.size(); ++i)
    {
        printIntegralData(i, integral_stack, state, BOEndTimes);
    }
}

void CombatSearch_IntegralData_FinishedUnits::writeToFile(const std::string & dir, const std::string & filename, const BuildOrder & buildOrder)
{
    std::stringstream ss;

    // build order sorted by start times
    if (buildOrder.size() == 0)
    {
        ss << m_bestIntegralBuildOrder.getNameString(2) << "\n";
    }
    else
    {
        ss << buildOrder.getNameString(2) << "\n";
    }

    // build order sorted by end times
    BuildOrder BOEndTimes = createBuildOrderEndTimes(m_bestIntegralStack, m_bestIntegralGameState);
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
                ss << "_" << BOEndTimes[i - 1].second.targetType.getName() << "_" << BOEndTimes[i - 1].second.targetProductionType.getName();
            }
        }

        ss << "\n";
    }

    std::ofstream file(dir + "/" + filename + "_BuildOrder.txt");
    file << ss.str();
    file.close();
}

const BuildOrder & CombatSearch_IntegralData_FinishedUnits::getBestBuildOrder() const
{
    return m_bestIntegralBuildOrder;
}

void CombatSearch_IntegralData_FinishedUnits::pop_back()
{
    assert(!m_integralStack.empty());
    m_integralStack.pop_back();
}

void CombatSearch_IntegralData_FinishedUnits::clear()
{
    m_integralStack.clear();
    m_integralStack.push_back(IntegralData_FinishedUnits());
}

void CombatSearch_IntegralData_FinishedUnits::popFinishedLastOrder(const GameState & prevState, const GameState & currState)
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

BuildOrder CombatSearch_IntegralData_FinishedUnits::createBuildOrderEndTimes() const
{
    return createBuildOrderEndTimes(m_integralStack, m_bestIntegralGameState);
}

// creates a build order based on the unit finished times
BuildOrder CombatSearch_IntegralData_FinishedUnits::createBuildOrderEndTimes(const std::vector<IntegralData_FinishedUnits> & integral_stack, const GameState & state) const
{
    BuildOrder buildOrder;
    auto & finishedUnits = state.getFinishedUnits();
    auto & chronoBoostTargets = state.getChronoBoostTargets();
    int chronoboosts = 0;

    //BOSS_ASSERT(integral_stack.size() - 1 == finishedUnits.size(), "Number of finished units on stack %i doesn't match the number of finished units %i in state",
    //                                                                integral_stack.size() - 1, finishedUnits.size());

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
