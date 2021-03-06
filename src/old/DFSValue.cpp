#define BOOST_PYTHON_STATIC_LIB
#include <boost/python/list.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/object_attributes.hpp>
#include "DFSValue.h"
#include "FileTools.h"

using namespace BOSS;
namespace python = boost::python;

DFSValue::DFSValue(const CombatSearchParameters p,
    const std::string & dir, const std::string & prefix, const std::string & name)
    : CombatSearch_Integral(p, dir, prefix, name)
{

}

void DFSValue::recurse(const GameState & state, int depth)
{
    m_highestValueFound = recurseReturnValue(state, depth);
    m_results.buildOrder = m_integral.getBestBuildOrder();
}

FracType DFSValue::recurseReturnValue(const GameState & state, int depth)
{
    if (timeLimitReached())
    {
        throw BOSS_COMBATSEARCH_TIMEOUT;
    }

    updateResults(state);

    FracType nodeIntegralToThisPoint = m_integral.getCurrentStackValue();
    FracType nodeIntegralValue = nodeIntegralToThisPoint;
    bool ffCalculated = false;
    bool isLeafNode = true;

    ActionSetAbilities legalActions;
    generateLegalActions(state, legalActions, m_params);
    auto actionValues = evaluateStates(state, legalActions);
    std::sort(actionValues.begin(), actionValues.end(),
        [](const ActionValue & lhs, const ActionValue & rhs) { return lhs.evaluation > rhs.evaluation; });

    for (int index = 0; index < actionValues.size(); ++index)
    {
        GameState child(state);

        auto action = actionValues[index].action;

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

        //std::cout << "action added: " << action.getName() << std::endl;
        //std::cout << "target of action added: " << actionTarget << std::endl;
        //std::cout << "frame of action added: " << child.getCurrentFrame() << std::endl;

        m_integral.update(child, m_buildOrder, m_params, m_searchTimer, true);
        isLeafNode = false;

        nodeIntegralValue = std::max(nodeIntegralValue, recurseReturnValue(child, depth + 1));

        m_buildOrder.pop_back();
        m_integral.popFinishedLastOrder(state, child);
    }

    if (m_params.getSaveStates())
    {
        state.writeToSS(m_ssStates, m_params);
        m_ssStates << "," << nodeIntegralValue - nodeIntegralToThisPoint << "\n";
        //json stateValuePair;
        //stateValuePair["State"] = state.writeToJson(m_params);
        //stateValuePair["Value"] = nodeIntegralValue - nodeIntegralToThisPoint;
        //std::vector<std::uint8_t> v_msgpack = json::to_msgpack(stateValuePair);
        //m_jStates.insert(m_jStates.end(), v_msgpack.begin(), v_msgpack.end());
        m_statesWritten++;

        if (m_statesWritten % 1000000 == 0)
        {
            FileTools::MakeDirectory(CONSTANTS::ExecutablePath + "/SavedStates");
            //std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + "_" + std::to_string(m_filesWritten) + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
            std::ofstream fileStates(CONSTANTS::ExecutablePath + "/SavedStates/" + m_name + ".csv", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
            fileStates << m_ssStates.rdbuf();
            m_ssStates.str(std::string());
            m_ssStates.clear();
            //fileStates.write(reinterpret_cast<const char*>(m_jStates.data()), m_jStates.size());
            //m_jStates.clear();
            m_filesWritten++;
        }
    }


    if (nodeIntegralValue > highestValueThusFar)
    {
        highestValueThusFar = nodeIntegralValue;
        m_ssHighestValue << m_results.nodesExpanded << "," << highestValueThusFar << "\n";
    }

    if (isLeafNode)
    {
        m_results.leafNodesExpanded++;
    }

    //std::cout << "Value to this point: " << nodeIntegralToThisPoint << ". Total value: " << nodeIntegralValue << std::endl;
    //std::cout << nodeIntegralValue << std::endl;

    return nodeIntegralValue;
}

std::vector<ActionValue> DFSValue::evaluateStates(const GameState & state, ActionSetAbilities & legalActions)
{
    std::stringstream ss;
    std::vector<ActionValue> actionValues;

    bool printNewBest = m_params.getPrintNewBest();
    m_params.setPrintNewBest(false);
    CombatSearch_IntegralDataFinishedUnits tempIntegral(m_integral);

    for (int index = 0; index < legalActions.size(); ++index)
    {
        GameState stateCopy(state);
        auto action = legalActions[index];

        if (action.first.isAbility())
        {
            stateCopy.doAbility(action.first, action.second);
        }
        else
        {
            stateCopy.doAction(action.first);
        }

        tempIntegral.update(stateCopy, m_buildOrder, m_params, m_searchTimer, false);

        ActionValue actionValue;
        actionValue.action = action;
        actionValue.evaluation = tempIntegral.getCurrentStackValue();
        actionValues.push_back(actionValue);

        tempIntegral.popFinishedLastOrder(state, stateCopy);

        stateCopy.writeToSS(ss, m_params);
        if (index < legalActions.size() - 1)
        {
            ss << "\n";
        }
    }

    m_params.setPrintNewBest(printNewBest);

    //std::cout << legalActions.toString() << std::endl;
    //std::cout << ss.str() << std::endl;

    if (legalActions.size() > 0)
    {
        // evaluate the states. the results will be returned as a list of FracTypes
        python::object values = CONSTANTS::Predictor.attr("predict")(ss.str().c_str());

        BOSS_ASSERT(len(values) == legalActions.size(), "size of values %i does not match the size of legalActions %i", len(values), legalActions.size());

        std::cout << std::endl;
        std::cout << "current frame: " << state.getCurrentFrame() << std::endl;
        for (int i = 0; i < legalActions.size(); ++i)
        {
            std::cout << "action id: " << legalActions[i].first.getID() << std::endl;
            FracType network_pred = python::extract<FracType>(values[i]);
            actionValues[i].evaluation += network_pred;
            std::cout << legalActions[i].first.getName() << ": network: " << network_pred << ", total: " << actionValues[i].evaluation << std::endl;
        }
        std::cout << std::endl;
    }

    return actionValues;
}