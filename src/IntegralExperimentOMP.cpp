/* -*- c-basic-offset: 4 -*- */

#include "IntegralExperimentOMP.h"
#include "CombatSearch.h"
#include "CombatSearch_Integral.h"
#include "CombatSearch_Bucket.h"
#include "CombatSearch_BestResponse.h"
#include "CombatSearch_IntegralMCTS.h"
#include "NMCS.h"
#include "NMCTS.h"
#include "DFSPolicy.h"
#include "DFSValue.h"
#include "DFSPolicyAndValue.h"
#include "FileTools.h"

#include <boost/chrono.hpp>
#include <chrono>
#include <thread>
#include <future>
#include <omp.h>


using namespace BOSS;

IntegralExperimentOMP::IntegralExperimentOMP()
    : m_race(Races::None)
{

}

IntegralExperimentOMP::IntegralExperimentOMP(const std::string& experimentName, const json& exp)
    : m_name(experimentName)
    , m_race(Races::None)
{
    BOSS_ASSERT(exp.count("Race") && exp["Race"].is_string(), "IntegralExperiment must have a 'Race' string");
    m_race = Races::GetRaceID(exp["Race"]);

    BOSS_ASSERT(exp.count("OutputDir") && exp["OutputDir"].is_string(), "IntegralExperiment must have an 'OutputDir' string");
    m_outputDir = exp["OutputDir"].get<std::string>();

    BOSS_ASSERT(exp.count("StartingState") && exp["StartingState"].is_string(), "IntegralExperiment must have a 'StartingState' string");
    m_params.setInitialState(BOSSConfig::Instance().GetState(exp["StartingState"]));

    BOSS_ASSERT(exp.count("FrameTimeLimit") && exp["FrameTimeLimit"].is_number_integer(), "IntegralExperiment must have a 'FrameTimeLimit' int");
    m_params.setFrameTimeLimit(exp["FrameTimeLimit"]);

    BOSS_ASSERT(exp.count("SearchTimeLimitMS") && exp["SearchTimeLimitMS"].is_number_integer(), "IntegralExperiment must have a 'SearchTimeLimitMS' int");
    m_params.setSearchTimeLimit(exp["SearchTimeLimitMS"]);

    BOSS_ASSERT(exp.count("PrintNewBest") && exp["PrintNewBest"].is_boolean(), "IntegralExperiment must have a PrintNewBest bool");
    m_params.setPrintNewBest(exp["PrintNewBest"]);

    BOSS_ASSERT(exp.count("SortActions") && exp["SortActions"].is_boolean(), "IntegralSearch must have a SortActions bool");
    m_params.setSortActions(exp["SortActions"]);

    BOSS_ASSERT(exp.count("SaveStates") && exp["SaveStates"].is_boolean(), "IntegralSearch must have a SaveStates bool");
    m_params.setSaveStates(exp["SaveStates"]);

    BOSS_ASSERT(exp.count("UseNetwork") && exp["UseNetwork"].is_boolean(), "IntegralSearch must have a UseNetwork bool");
    m_params.setNetworkPrediction(exp["UseNetwork"]);

    BOSS_ASSERT(exp.count("Threads") && exp["Threads"].is_number_integer(), "Integral Search must have a Threads int");
    m_params.setThreadsForExperiment(exp["Threads"]);

    const std::string& searchType = exp["SearchType"][0].get<std::string>();
    m_searchType = searchType;

    if (searchType == "IntegralDFS" || searchType == "IntegralDFSVN" || searchType == "IntegralDFSPN" || searchType == "IntegralDFSPVN")
    {
        BOSS_ASSERT(searchType == "IntegralDFS" && !m_params.useNetworkPrediction() ||
            ((searchType == "IntegralDFSVN" || searchType == "IntegralDFSPN" || searchType == "IntegralDFSPVN") && m_params.useNetworkPrediction()), "Turn off UseNetwork flag for standard DFS search");
    }

    if (searchType == "IntegralMCTS")
    {
        auto& searchParameters = exp["SearchParameters"];
        m_params.setExplorationValue(searchParameters["ExplorationConstant"]);
        m_params.setUseMaxValue(searchParameters["UseMax"]);

        if (searchParameters.count("Simulations"))
        {
            m_params.setNumberOfSimulations(searchParameters["Simulations"]);
        }
        else
        {
            m_params.setNumberOfSimulations(std::numeric_limits<int>::max());
        }

        if (searchParameters.count("Nodes"))
        {
            m_params.setNumberOfNodes(searchParameters["Nodes"]);
        }
        else
        {
            m_params.setNumberOfNodes(std::numeric_limits<int>::max());
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_params.getExplorationValue();
        m_name += "C" + ss.str();
    }

    else if (searchType == "IntegralNMCS")
    {
        auto& searchParameters = exp["SearchParameters"];
        m_params.setNumPlayouts(searchParameters["Playouts"]);
        m_params.setLevel(searchParameters["Level"]);
    }

    else if (searchType == "IntegralNMCTS")
    {
        auto& searchParameters = exp["SearchParameters"];
        m_params.setNumPlayouts(searchParameters["Playouts"]);
        m_params.setLevel(searchParameters["Level"]);
        m_params.setUseMaxValue(searchParameters["UseMax"]);
    }

    /*if (searchType == "IntegralDFS")
    {
        m_name += "DFS";
    }*/

    if (exp.count("MaxActions"))
    {
        const json& maxActions = exp["MaxActions"];
        BOSS_ASSERT(maxActions.is_array(), "MaxActions is not an array");

        for (size_t i(0); i < maxActions.size(); ++i)
        {
            BOSS_ASSERT(maxActions[i].is_array(), "MaxActions element must be array of size 2");

            BOSS_ASSERT(maxActions[i].size() == 2 && maxActions[i][0u].is_string() && maxActions[i][1u].is_number_integer(), "MaxActions element must be [\"Action\", Count]");

            const std::string& typeName = maxActions[i][0u];

            BOSS_ASSERT(ActionTypes::TypeExists(typeName), "Action Type doesn't exist: %s", typeName.c_str());

            m_params.setMaxActions(ActionTypes::GetActionType(typeName), maxActions[i][1]);
        }
    }

    if (exp.count("RelevantActions"))
    {
        const json& relevantActions = exp["RelevantActions"];
        BOSS_ASSERT(relevantActions.is_array(), "RelevantActions is not an array");

        ActionSetAbilities relevantActionSet;

        for (size_t i(0); i < relevantActions.size(); ++i)
        {
            BOSS_ASSERT(relevantActions[i].is_string(), "RelvantActions element must be action type string");
            std::string element = relevantActions[i];
            BOSS_ASSERT(ActionTypes::TypeExists(relevantActions[i]), "Action Type doesn't exist: %s", element.c_str());

            relevantActionSet.add(ActionTypes::GetActionType(element));
        }

        m_params.setRelevantActions(relevantActionSet);
    }

    if (exp.count("AlwaysMakeWorkers"))
    {
        BOSS_ASSERT(exp["AlwaysMakeWorkers"].is_boolean(), "AlwaysMakeWorkers should be a bool");

        m_params.setAlwaysMakeWorkers(exp["AlwaysMakeWorkers"]);
    }

    if (exp.count("OpeningBuildOrder"))
    {
        BOSS_ASSERT(exp["OpeningBuildOrder"].is_string(), "OpeningBuildOrder should be a string");
        m_params.setOpeningBuildOrder(BOSSConfig::Instance().GetBuildOrder(exp["OpeningBuildOrder"]));
    }

    if (exp.count("BestResponseParams"))
    {
        const json& brVal = exp["BestResponseParams"];

        BOSS_ASSERT(brVal.is_object(), "BestResponseParams not an object");
        BOSS_ASSERT(brVal.count("EnemyState"), "bestResponseParams must have 'enemyState' string");
        BOSS_ASSERT(brVal.count("EnemyBuildOrder"), "bestResponseParams must have 'enemyBuildOrder' string");

        BOSS_ASSERT(brVal.count("EnemyState") && brVal["EnemyState"].is_string(), "bestResponseParams must have a 'EnemyState' string");
        m_params.setEnemyInitialState(BOSSConfig::Instance().GetState(brVal["EnemyState"]));

        BOSS_ASSERT(brVal.count("EnemyBuildOrder") && brVal["EnemyBuildOrder"].is_string(), "BestResponseParams must have a 'EnemyBuildOrder' string");
        m_params.setEnemyBuildOrder(BOSSConfig::Instance().GetBuildOrder(brVal["EnemyBuildOrder"]));
    }

    if (exp.count("SimulationsPerStep"))
    {
        BOSS_ASSERT(exp["SimulationsPerStep"].is_array() && exp["SimulationsPerStep"].size() == 3, "SimulationsPerStep must be an array of size 3");
        BOSS_ASSERT(exp["SimulationsPerStep"][0].is_boolean(), "Entry 0 of SimulationsPerStep must be a bool");
        BOSS_ASSERT(exp["SimulationsPerStep"][1].is_number_integer(), "Entry 1 of SimulationsPerStep must be an integer");
        BOSS_ASSERT(exp["SimulationsPerStep"][2].is_number_float(), "Entry 2 of SimulationsPerStep must be a float");
        m_params.setChangingRoot(exp["SimulationsPerStep"][0]);
        m_params.setSimulationsPerStep(exp["SimulationsPerStep"][1]);
        m_params.setSimulationsPerStepDecay(exp["SimulationsPerStep"][2]);
    }

    if (exp.count("OpponentUnits"))
    {
        BOSS_ASSERT(exp["OpponentUnits"].is_array() && exp["OpponentUnits"].size() > 0, "OpponentUnits must be an array");
        std::vector<int> enemyUnits(ActionTypes::GetAllActionTypes().size());
        m_params.setEnemyRace(ActionTypes::GetActionType(exp["OpponentUnits"][0][0].get<std::string>()).getRace());
        for (auto& units : exp["OpponentUnits"])
        {
            BOSS_ASSERT(units.is_array() && units.size() == 2, "Unit vector inside OpponentUnits must be an array of size 2");
            BOSS_ASSERT(units[0].is_string(), "First index of unit must be a string corresponding to name");
            BOSS_ASSERT(units[1].is_number_integer(), "Second index of unit must be an integer corresponding to the number of units");

            enemyUnits[ActionTypes::GetActionType(units[0].get<std::string>()).getID()] = units[1];
        }

        m_params.setEnemyUnits(enemyUnits);
    }

    if (exp.count("MaximizeValue"))
    {
        BOSS_ASSERT(exp["MaximizeValue"].is_boolean(), "MaximizeValue must be a bool");
        m_params.setMaximizeValue(exp["MaximizeValue"]);
    }

    if (exp.count("UseTotalTimeLimit"))
    {
        BOSS_ASSERT(exp["UseTotalTimeLimit"].is_boolean(), "UseTotalTimeLimit must be a bool");
        m_params.setUseTotalTimeLimit(exp["UseTotalTimeLimit"]);
    }
}

void IntegralExperimentOMP::runExperimentThread(int thread, int numRuns, int startingIndex)
{
    static std::string stars = "************************************************";

    for (int i(0); i < numRuns; ++i)
    {
        int index = i + (startingIndex);
        std::string name = m_name;
        if (m_searchType != "IntegralDFS")
        {
            name += "Run" + std::to_string(index);
        }
        std::string outputDir = m_outputDir + "/" + Assert::CurrentDateTime() + "_" + name;
        FileTools::MakeDirectory(outputDir);

        std::string resultsFile = name;

        std::cout << "\n" << stars << "\n* Running Experiment: " << name << " [" << m_searchType << "]\n" << stars << "\n";

        std::unique_ptr<CombatSearch> combatSearch;

        if (m_searchType == "IntegralDFS")
        {
            combatSearch = std::unique_ptr<CombatSearch>(new CombatSearch_Integral(m_params, outputDir, resultsFile, m_name));
        }
        else if (m_searchType == "IntegralDFSVN")
        {
            combatSearch = std::unique_ptr<CombatSearch>(new DFSValue(m_params, outputDir, resultsFile, m_name));
        }
        else if (m_searchType == "IntegralDFSPN")
        {
            combatSearch = std::unique_ptr<CombatSearch>(new DFSPolicy(m_params, outputDir, resultsFile, m_name));
        }
        else if (m_searchType == "IntegralDFSPVN")
        {
            combatSearch = std::unique_ptr<CombatSearch>(new DFSPolicyAndValue(m_params, outputDir, resultsFile, m_name));
        }
        else if (m_searchType == "IntegralMCTS")
        {
            //resultsFile += "_IntegralMCTS";
            combatSearch = std::unique_ptr<CombatSearch>(new CombatSearch_IntegralMCTS(m_params, outputDir, resultsFile, m_name));
        }
        else if (m_searchType == "IntegralNMCS")
        {
            combatSearch = std::unique_ptr<CombatSearch>(new NMCS(m_params, outputDir, resultsFile, m_name));
        }
        else if (m_searchType == "IntegralNMCTS")
        {
            combatSearch = std::unique_ptr<CombatSearch>(new NMCTS(m_params, outputDir, resultsFile, m_name));
        }
        else
        {
            BOSS_ASSERT(false, "CombatSearch type not found: %s", m_searchType.c_str());
        }

        combatSearch->search();
        combatSearch->printResults();
        combatSearch->writeResultsFile(outputDir, resultsFile);
        const CombatSearchResults& results = combatSearch->getResults();
    }
}

void IntegralExperimentOMP::runTotalTimeExperiment(int run)
{
    static std::string stars = "************************************************";

    std::string name = m_name;
    if (m_searchType != "IntegralDFS")
    {
        name += "Run" + std::to_string(run);
    }
    std::string outputDir = m_outputDir + "/" + Assert::CurrentDateTime() + "_" + name;
    FileTools::MakeDirectory(outputDir);

    std::cout << "\n" << stars << "\n* Running Experiment: " << name << " [" << m_searchType << "]\n" << stars << "\n";

    auto startRealTime = std::chrono::system_clock::now();
    boost::chrono::thread_clock::time_point start = boost::chrono::thread_clock::now();

    int numRuns = 0;
    std::string resultsFile = m_name;
    std::vector<CombatSearchResults> results;
    CombatSearchParameters params = m_params;
    int nodesPerMilliSecond = 300;
    params.setNumberOfNodes(int(params.getSearchTimeLimit() * nodesPerMilliSecond));
    params.setSearchTimeLimit(std::numeric_limits<float>::max());

    while (true)
    {
        if (results.size() > 0)
        {
            params.setNumberOfNodes(params.getNumberOfNodes() - results.back().nodeVisits);

            // we're done
            if (params.getNumberOfNodes() <= 0)
            {
                break;
            }
        }

        //std::cout << "node limit is: " << params.getNumberOfNodes() << std::endl;

        std::string dir = outputDir + "/Run" + std::to_string(numRuns);
        FileTools::MakeDirectory(dir);

        //std::cout << "search time limit: " << params.getSearchTimeLimit() << std::endl;
        std::unique_ptr<CombatSearch> combatSearch = std::unique_ptr<CombatSearch>(new CombatSearch_IntegralMCTS(params, dir, resultsFile, m_name));

        combatSearch->search();
        //combatSearch->printResults();
        combatSearch->writeResultsFile(dir, resultsFile);
        results.push_back(combatSearch->getResults());
        ++numRuns;
    }

    CombatSearchResults avgResults;
    CombatSearchResults bestResults;
    int bestRun = 0;
    int bestRunFinished = 0;
    int bestRunUseful = 0;
    for (int index = 0; index < results.size(); ++index)
    {
        const auto& result = results[index];
        avgResults.eval += result.eval;
        avgResults.value += result.value;
        if (result.eval > bestResults.eval)
        {
            bestResults.eval = result.eval;
            bestResults.value = result.value;
            bestResults.buildOrder = result.buildOrder;
            bestRun = index;
        }

        avgResults.finishedEval += result.finishedEval;
        avgResults.finishedValue += result.finishedValue;
        if (result.finishedEval > bestResults.finishedEval)
        {
            bestResults.finishedEval = result.finishedEval;
            bestResults.finishedValue = result.finishedValue;
            bestResults.finishedUnitsBuildOrder = result.finishedUnitsBuildOrder;
            bestRunFinished = index;
        }

        avgResults.usefulEval += result.usefulEval;
        avgResults.usefulValue += result.usefulValue;
        if (result.usefulEval > bestResults.usefulEval || results.size() == 1)
        {
            bestResults.usefulEval = result.usefulEval;
            bestResults.usefulValue = result.usefulValue;
            bestResults.usefulBuildOrder = result.usefulBuildOrder;
            bestResults.leafNodesExpanded = result.leafNodesExpanded;
            bestResults.leafNodesVisited = result.leafNodesVisited;
            bestResults.nodesExpanded = result.nodesExpanded;
            bestResults.nodeVisits = result.nodeVisits;
            bestResults.timeElapsed = (result.timeElapsed / 1000);
            bestResults.timeElapsedCPU = (result.timeElapsedCPU / 1000);
            bestResults.numSimulations = result.numSimulations;
            bestRunUseful = index;
        }

        avgResults.leafNodesExpanded = result.leafNodesExpanded;
        avgResults.leafNodesVisited += result.leafNodesVisited;
        avgResults.nodesExpanded += result.nodesExpanded;
        avgResults.nodeVisits += result.nodeVisits;
        avgResults.timeElapsed += (result.timeElapsed / 1000);
        avgResults.timeElapsedCPU += (result.timeElapsedCPU / 1000);
        avgResults.numSimulations += result.numSimulations;
    }
    double totalTimeElapsed = avgResults.timeElapsed;
    double totalTimeElapsedCPU = avgResults.timeElapsedCPU;

    avgResults.eval /= results.size();
    avgResults.value /= results.size();
    avgResults.finishedEval /= results.size();
    avgResults.finishedValue /= results.size();
    avgResults.usefulEval /= results.size();
    avgResults.usefulValue /= results.size();
    avgResults.leafNodesExpanded /= results.size();
    avgResults.leafNodesVisited /= results.size();
    avgResults.nodesExpanded /= results.size();
    avgResults.nodeVisits /= results.size();
    avgResults.timeElapsed /= results.size();
    avgResults.timeElapsedCPU /= results.size();
    avgResults.numSimulations /= (int)results.size();

    /*std::cout << "total real search time: " << totalTimeElapsed << std::endl;
    std::cout << "total cpu search time: " << totalTimeElapsedCPU << std::endl;

    std::cout << "real time elapsed: " << std::chrono::duration<double>(std::chrono::system_clock::now() - startRealTime).count() << std::endl;
    std::cout << "cpu time elapsed: " << boost::chrono::duration_cast<boost::chrono::duration<long long, boost::milli>>(boost::chrono::thread_clock::now() - start).count() / 1000 << std::endl;
*/
    json jResult;

    jResult["Average"]["Eval"] = avgResults.eval;
    jResult["Average"]["Value"] = avgResults.value;
    jResult["Average"]["FinishedEval"] = avgResults.finishedEval;
    jResult["Average"]["FinishedValue"] = avgResults.finishedValue;
    jResult["Average"]["UsefulEval"] = avgResults.usefulEval;
    jResult["Average"]["UsefulValue"] = avgResults.usefulValue;
    jResult["Average"]["LeafNodesExpanded"] = avgResults.leafNodesExpanded;
    jResult["Average"]["LeafNodesVisited"] = avgResults.leafNodesVisited;
    jResult["Average"]["NodesExpanded"] = avgResults.nodesExpanded;
    jResult["Average"]["NodeVisits"] = avgResults.nodeVisits;
    jResult["Average"]["TimeElapsed"] = avgResults.timeElapsed;
    jResult["Average"]["TimeElapsedCPU"] = avgResults.timeElapsedCPU;
    jResult["Average"]["NumSimulations"] = avgResults.numSimulations;
    jResult["Average"]["NumRuns"] = results.size();

    jResult["Best"]["Eval"] = bestResults.eval;
    jResult["Best"]["Value"] = bestResults.value;
    jResult["Best"]["EvalBuildOrder"] = bestResults.buildOrder.getNameString();
    jResult["Best"]["FinishedEval"] = bestResults.finishedEval;
    jResult["Best"]["FinishedValue"] = bestResults.finishedValue;
    jResult["Best"]["FinishedBuildOrder"] = bestResults.finishedUnitsBuildOrder.getNameString();
    jResult["Best"]["UsefulEval"] = bestResults.usefulEval;
    jResult["Best"]["UsefulValue"] = bestResults.usefulValue;
    jResult["Best"]["UsefulBuildOrder"] = bestResults.usefulBuildOrder.getNameString();
    jResult["Best"]["LeafNodesExpanded"] = bestResults.leafNodesExpanded;
    jResult["Best"]["LeafNodesVisited"] = bestResults.leafNodesVisited;
    jResult["Best"]["NodesExpanded"] = bestResults.nodesExpanded;
    jResult["Best"]["NodeVisits"] = bestResults.nodeVisits;
    jResult["Best"]["TimeElapsed"] = bestResults.timeElapsed;
    jResult["Best"]["TimeElapsedCPU"] = bestResults.timeElapsedCPU;
    jResult["Best"]["NumSimulations"] = bestResults.numSimulations;
    jResult["Best"]["BestRunEval"] = bestRun;
    jResult["Best"]["BestRunEvalFinished"] = bestRunFinished;
    jResult["Best"]["BestRunEvalUseful"] = bestRunUseful;

    std::ofstream outputStream(outputDir + "/Results.json");
    outputStream << std::setw(4) << jResult << std::endl;
}

void IntegralExperimentOMP::run(int numberOfRuns)
{
    FileTools::MakeDirectory(m_outputDir);

    //#pragma omp parallel for
    for (int run = 0; run < numberOfRuns; ++run)
    {
        if (m_params.getUseTotalTimeLimit())
        {
            runTotalTimeExperiment(run);
        }
        else
        {

        }
    }   
}
