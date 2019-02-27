/* -*- c-basic-offset: 4 -*- */

#include "CombatSearch_Integral.h"

using namespace BOSS;

CombatSearch_Integral::CombatSearch_Integral(const CombatSearchParameters p, int run)
{
    m_params = p;

    if (m_params.getSaveStates())
    {
        m_file.open(CONSTANTS::ExecutablePath + "/data/stateValuePairs_" + std::to_string(run) + ".csv", std::ofstream::out | std::ofstream::app);
    }

    //BOSS_ASSERT(m_params.getInitialState().getRace() != Races::None, "Combat search initial state is invalid");
}

CombatSearch_Integral::~CombatSearch_Integral()
{
    m_file << m_ss.rdbuf();
    m_ss.str(std::string());
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

    if (m_params.getSaveStates() && m_results.nodesExpanded % 100000 == 0)
    {
        m_file << m_ss.rdbuf();
        m_ss.str(std::string());
    }

    FracType nodeIntegralValue = 0;
    FracType nodeIntegralToThisPoint = m_integral.getValueToThisPoint();
    bool ffCalculated = false;

    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);

    for (int a(0); a < legalActions.size(); ++a)
    {
        const int index = a;
        GameState child(state);

        auto action = legalActions[index];

        // if it's the plain CB without a target, we need to get the targets for the ability
        if (action.first.isAbility() && action.second == -1)
        {
            child.getSpecialAbilityTargets(legalActions, index);
            // the ability is no longer valid, skip
            if (legalActions[index].second == -1)
            {
                continue;
            }
        }

        action = legalActions[index];

        if (action.first.isAbility())
        {
            child.doAbility(action.first, action.second);
            m_buildOrder.add(action.first, child.getLastAbility());
        } 
        else
        {
            child.doAction(action.first);
            m_buildOrder.add(action.first);
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
            //if (!ffCalculated)
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

    if (m_params.getSaveStates())
    {
        state.writeToSS(m_ss, m_params);
        m_ss << "," << nodeIntegralValue - nodeIntegralToThisPoint << "\n";
    }

    //std::cout << "Value to this point: " << nodeIntegralToThisPoint << ". Total value: " << nodeIntegralValue << std::endl;
    //std::cout << nodeIntegralValue << std::endl;

    return nodeIntegralValue;
}

void CombatSearch_Integral::printResults()
{
    m_integral.print();
    std::cout << "\nSearched " << m_results.nodesExpanded << " nodes in " << m_results.timeElapsed << "ms @ " << (1000.0*m_results.nodesExpanded / m_results.timeElapsed) << " nodes/sec\n\n";
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

    std::ofstream file(dir + "/" + filename + "_BuildOrder.txt", std::ofstream::out | std::ofstream::app);
    file << "\nSearched " << m_results.nodesExpanded << " nodes in " << m_results.timeElapsed << "ms @ " << (1000.0*m_results.nodesExpanded / m_results.timeElapsed) << " nodes/sec";
}