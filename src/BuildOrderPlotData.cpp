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
    int numInitialUnits = m_initialState.getNumUnits();

    for (int i(0); i < m_buildOrder.size(); ++i)
    {
        auto & actionTargetPair = m_buildOrder[i];
        ActionType type = actionTargetPair.first;
        if (type.isAbility())
        {
            state.doAbility(type, actionTargetPair.second.targetID);
            m_chronoBoosts.push_back(i);
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

    
    int latestTimeFinish = 0;
    const GameState stateLatestFinish(state);
    for (int i = 0; i < stateLatestFinish.getNumUnits(); ++i)
    {
        int timeUntilBuilt = stateLatestFinish.getUnit(i).getTimeUntilBuilt();
        if (timeUntilBuilt > latestTimeFinish)
        {
            latestTimeFinish = timeUntilBuilt;
        }
    }
    state.fastForward(state.getCurrentFrame() + latestTimeFinish + 1); // ff far enough so everything is done

    // get the finish times
    const GameState constState(state);
    int abilities = 0;
    m_maxFinishTime = 0;

    for (int i(0); i < m_buildOrder.size(); ++i)
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
    // <index inside unit vector of GameState, layer>
    m_layers = std::vector<std::pair<int,int>>(m_startTimes.size(), std::make_pair(-1, -1)); 

    int numInitialUnits = m_initialState.getNumUnits();
    int chronoBoostsSoFar = 0;

    // determine the layers for each action
    for (int i(0); i < m_startTimes.size(); ++i)
    {
        int start               = m_startTimes[i];

        auto & actionTargetPair = m_buildOrder[i];
        ActionType type = actionTargetPair.first;

        //std::cout << "action: " << action.getName() << std::endl;
        //std::cout << "start: " << start << " . end: " << finish << std::endl;
        
        if (type.isAbility())
        {
            // Overlap chronoboost with the unit it was cast on
            if (type.getName() == "ChronoBoost")
            {
                AbilityAction action = actionTargetPair.second;

                for (int j = 0; j < i; ++j)
                {
                    if (m_layers[j].first == action.targetProductionID - numInitialUnits)
                    {
                        m_layers[i] = m_layers[j];
                    }
                }

                chronoBoostsSoFar++;
            }

            continue;
        }

        std::vector<int> layerOverlap;

        // loop through everything up to this action and see which layers it can't be in
        for (int j(0); j < i; ++j)
        {
            if (start < m_finishTimes[j] && !m_buildOrder[j].first.isAbility())
            {
                layerOverlap.push_back(m_layers[j].second);
            }
        }

        // find a layer we can assign to this value
        int layerTest = 0;
        while (true)
        {
            if (std::find(layerOverlap.begin(), layerOverlap.end(), layerTest) == layerOverlap.end())
            {
                m_layers[i].second = layerTest;
                if (layerTest > m_maxLayer)
                {
                    m_maxLayer = layerTest;
                }
                break;
            }

            layerTest++;
        }

        m_layers[i].first = i - chronoBoostsSoFar;
    }

    for (int i(0); i < m_buildOrder.size(); ++i)
    {
        Position topLeft(m_startTimes[i], m_layers[i].second * (m_boxHeight + m_boxHeightBuffer));
        Position bottomRight(m_finishTimes[i], topLeft.y() + m_boxHeight);
        m_rectangles.push_back(Rectangle(m_buildOrder[i].first.getName(), topLeft, bottomRight));
    }
}
