/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch_Integral.h"

using namespace BOSS;

CombatSearch_Integral::CombatSearch_Integral(const CombatSearchParameters p)
{
    m_params = p;

    m_file.open("../bin/data/states.csv", std::ofstream::out | std::ofstream::trunc);

    //BOSS_ASSERT(m_params.getInitialState().getRace() != Races::None, "Combat search initial state is invalid");
}

CombatSearch_Integral::~CombatSearch_Integral()
{
    m_file << m_ss.rdbuf();
    m_ss.clear();
    m_file.close();
}

void CombatSearch_Integral::recurse(const GameState & state, int depth)
{
    FracType highestValueFound = recurseReturnValue(state, depth);
}

FracType CombatSearch_Integral::recurseReturnValue(const GameState & state, int depth)
{
    if (timeLimitReached())
    {
        throw BOSS_COMBATSEARCH_TIMEOUT;
    }

    updateResults(state);

    if (m_params.getSaveResults() && m_results.nodesExpanded % 100000 == 0)
    {
        m_file << m_ss.rdbuf();
        m_ss.clear();
    }

    FracType nodeIntegralValue = 0;
    FracType nodeIntegralToThisPoint = m_integral.getValueToThisPoint();
    bool ffCalculated = false;

    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);

    for (int a(0); a < legalActions.size(); ++a)
    {
        const int index = legalActions.size() - (a + 1);
        GameState child(state);

        const ActionSetAbilities::ActionTargetPair & actionTargetPair = legalActions[index];

        ActionType action = actionTargetPair.first;
        NumUnits actionTarget = actionTargetPair.second;

        // if it's the plain CB without a target, we need to get the targets for the ability
        if (action == ActionTypes::GetSpecialAction(state.getRace()) && actionTarget == -1)
        {
            int sizeBefore = legalActions.size();

            state.getSpecialAbilityTargets(legalActions, index);

            // the new target 
            actionTarget = legalActions.getAbilityTarget(index + (legalActions.size() - sizeBefore));

            // the ability is no longer valid, skip
            if (actionTarget == -1)
            {
                continue;
            }
        }

        if (action.isAbility())
        {
            child.doAbility(action, actionTarget);
            m_buildOrder.add(action, child.getLastAbility());
        } 
        else
        {
            child.doAction(action);
            m_buildOrder.add(action);
        }

        // can't go over the time limit
        if (child.getCurrentFrame() <= m_params.getFrameTimeLimit())
        {
            //std::cout << "action added: " << action.getName() << std::endl;
            //std::cout << "target of action added: " << actionTarget << std::endl;
            //std::cout << "frame of action added: " << child.getCurrentFrame() << std::endl;
            
            m_integral.update(child, m_buildOrder, m_params, m_searchTimer);

            nodeIntegralValue = std::max(nodeIntegralValue, recurseReturnValue(child, depth + 1));

            m_buildOrder.pop_back();
            m_integral.popFinishedLastOrder(state, child);
        }

        // go upto the time limit and update the integral stack
        else
        {
            m_buildOrder.pop_back();
            if (!ffCalculated)
            {
                GameState child_framelimit(state);
                child_framelimit.fastForward(m_params.getFrameTimeLimit());

                m_integral.update(child_framelimit, m_buildOrder, m_params, m_searchTimer);
                nodeIntegralValue = std::max(nodeIntegralValue, m_integral.getCurrentStackValue());

                m_integral.popFinishedLastOrder(state, child_framelimit);

                ffCalculated = true;
            }
        }
    }

    if (m_params.getSaveResults())
    {
        state.writeToSS(m_ss);
        m_ss << ", " << nodeIntegralValue - nodeIntegralToThisPoint << "\n";
    }

    //std::cout << "Value to this point: " << nodeIntegralToThisPoint << ". Total value: " << nodeIntegralValue << std::endl;
    //std::cout << nodeIntegralValue << std::endl;

    return nodeIntegralValue;
}

void CombatSearch_Integral::printResults()
{
    m_integral.print();
}

void CombatSearch_Integral::setBestBuildOrder()
{
    m_results.buildOrder = m_integral.getBestBuildOrder();
}


#include "BuildOrderPlotter.h"
void CombatSearch_Integral::writeResultsFile(const std::string & dir, const std::string & filename)
{
    BuildOrderPlotter plot;
    plot.setOutputDir(dir);
    plot.addPlot(filename, m_params.getInitialState(), m_integral.getBestBuildOrder());
    plot.doPlots();

    m_integral.writeToFile(dir, filename);
}