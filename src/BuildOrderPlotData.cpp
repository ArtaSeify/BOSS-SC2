/* -*- c-basic-offset: 4 -*- */

#include "BuildOrderPlotData.h"
#include "Eval.h"

using namespace BOSS;

BuildOrderPlotData::BuildOrderPlotData(const GameState & initialState, const BuildOrderAbilities & buildOrder)
    : m_initialState(initialState)
    , m_buildOrder(buildOrder)
    , m_maxLayer(0)
    , m_maxFinishTime(0)
    , m_boxHeight(20)
    , m_boxHeightBuffer(3)
{
    calculateStartEndTimes();
    calculatePlot();
}

void BuildOrderPlotData::calculateStartEndTimes()
{
    GameState state(m_initialState);

    //BOSS_ASSERT(_buildOrder.isLegalFromState(state), "Build order isn't legal!");

    for (size_t i(0); i < m_buildOrder.size(); ++i)
    {
        auto & actionTargetPair = m_buildOrder[i];
        ActionType type = actionTargetPair.first;
        if (type.isAbility())
        {
            state.doAbility(type, actionTargetPair.second.targetID);
        }
        else
        {
            state.doAction(type);
        }

        m_startTimes.push_back(state.getCurrentFrame());

        /*int finish = state.getCurrentFrame() + type.buildTime();
        if (type.isBuilding() && !type.isAddon() && !type.isMorphed())
        {
            finish += 0; // TODO: building constant walk time
        }

        m_finishTimes.push_back(finish);

        m_maxFinishTime = std::max(m_maxFinishTime, finish);*/

        m_armyValues.push_back(Eval::ArmyTotalResourceSum(state));

        std::pair<int, int> mineralsBefore(state.getCurrentFrame(), (int)(state.getMinerals() + type.mineralPrice()));
        std::pair<int, int> mineralsAfter(state.getCurrentFrame(), (int)state.getMinerals());

        std::pair<int, int> gasBefore(state.getCurrentFrame(), (int)(state.getGas() + type.gasPrice()));
        std::pair<int, int> gasAfter(state.getCurrentFrame(), (int)state.getGas());

        m_minerals.push_back(mineralsBefore);
        m_minerals.push_back(mineralsAfter);
        m_gas.push_back(gasBefore);
        m_gas.push_back(gasAfter);
    }

    // get the finish times
    size_t numInitialUnits = m_initialState.getNumUnits();
    state.fastForward(5000); // ff far enough so everything is done
    const GameState constState = std::as_const(state);
    int abilities = 0;
    m_maxFinishTime = 0;
    for (size_t i(0); i < m_buildOrder.size(); ++i)
    {
        auto & actionTargetPair = m_buildOrder[i];
        ActionType type = actionTargetPair.first;
        int finish;
        if (type.isAbility())
        {
            finish = actionTargetPair.second.frameCast + type.buildTime();
            abilities++;
        }

        else
        {
            finish = constState.getUnit(NumUnits(numInitialUnits + i - abilities)).getFinishFrame();
        }
        

        m_finishTimes.push_back(finish);
        m_maxFinishTime = std::max(m_maxFinishTime, finish);
    }
}

void BuildOrderPlotData::calculatePlot()
{
    m_layers = std::vector<int>(m_startTimes.size(), -1); 

    // determine the layers for each action
    for (size_t i(0); i < m_startTimes.size(); ++i)
    {
        int start               = m_startTimes[i];
#if 0
        //!!!PROBLEM NOT USED
        int finish              = m_finishTimes[i];
#endif        
        auto & actionTargetPair = m_buildOrder[i];
        ActionType type = actionTargetPair.first;

        //std::cout << "action: " << action.getName() << std::endl;
        //std::cout << "start: " << start << " . end: " << finish << std::endl;
        
        if (type.isAbility())
        {
            // Overlap chronoboost with the unit it was cast on
            if (type.getName() == "ChronoBoost")
            {
                size_t numInitialUnits = m_initialState.getNumUnits();
                AbilityAction action = actionTargetPair.second;
                m_layers[i] = m_layers[action.targetProductionID - numInitialUnits];
                continue;
            }
        }

        std::vector<int> layerOverlap;

        // loop through everything up to this action and see which layers it can't be in
        for (size_t j(0); j < i; ++j)
        {
            if (start < m_finishTimes[j] && !m_buildOrder[j].first.isAbility())
            {
                layerOverlap.push_back(m_layers[j]);
            }
        }

        // find a layer we can assign to this value
        int layerTest = 0;
        while (true)
        {
            if (std::find(layerOverlap.begin(), layerOverlap.end(), layerTest) == layerOverlap.end())
            {
                m_layers[i] = layerTest;
                if (layerTest > m_maxLayer)
                {
                    m_maxLayer = layerTest;
                }
                break;
            }

            layerTest++;
        }
    }

    for (size_t i(0); i < m_buildOrder.size(); ++i)
    {
        Position topLeft(m_startTimes[i], m_layers[i] * (m_boxHeight + m_boxHeightBuffer));
        Position bottomRight(m_finishTimes[i], topLeft.y() + m_boxHeight);
        m_rectangles.push_back(Rectangle(m_buildOrder[i].first.getName(), topLeft, bottomRight));
    }
}
