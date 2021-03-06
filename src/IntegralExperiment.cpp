/* -*- c-basic-offset: 4 -*- */

#include "IntegralExperiment.h"
#include "CombatSearch.h"
#include "CombatSearch_Integral_DFS.h"
//#include "CombatSearch_Integral_MCTS.h"
#include "CombatSearch_Bucket.h"
#include "CombatSearch_BestResponse.h"
#include "FileTools.h"
#include "Eval.h"

#include <chrono>
#include <thread>
#include <future>
#include <omp.h>

using namespace BOSS;

IntegralExperiment::IntegralExperiment()
    : m_race(Races::None)
{

}

IntegralExperiment::IntegralExperiment(const std::string& experimentName, const json& exp)
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
    m_params.setFrameLimit(exp["FrameTimeLimit"]);

    BOSS_ASSERT(exp.count("SearchTimeLimitMS") && exp["SearchTimeLimitMS"].is_number_integer(), "IntegralExperiment must have a 'SearchTimeLimitMS' int");
    m_params.setSearchTimeLimit(exp["SearchTimeLimitMS"]);

    BOSS_ASSERT(exp.count("PrintNewBest") && exp["PrintNewBest"].is_boolean(), "IntegralExperiment must have a PrintNewBest bool");
    m_params.setPrintNewBest(exp["PrintNewBest"]);

    BOSS_ASSERT(exp.count("SortActions") && exp["SortActions"].is_boolean(), "IntegralSearch must have a SortActions bool");
    m_params.setSortActions(exp["SortActions"]);

    m_searchType = exp["SearchType"][0].get<std::string>();

    /*if (searchType == "IntegralDFS" || searchType == "IntegralDFSVN" || searchType == "IntegralDFSPN" || searchType == "IntegralDFSPVN")
    {
        BOSS_ASSERT(searchType == "IntegralDFS" && !m_params.useNetworkPrediction() ||
            ((searchType == "IntegralDFSVN" || searchType == "IntegralDFSPN" || searchType == "IntegralDFSPVN") && m_params.useNetworkPrediction()), "Turn off UseNetwork flag for standard DFS search");
    }*/

    if (m_searchType == "IntegralMCTS")
    {
        auto& searchParameters = exp["SearchParameters"];

        BOSS_ASSERT(searchParameters.count("ExplorationConstant") && searchParameters["ExplorationConstant"].is_number_float(),
            "SearchParameters must include a float ExplorationConstant");
        m_params.setExplorationValue(searchParameters["ExplorationConstant"]);

        BOSS_ASSERT(searchParameters.count("VisitsBeforeExpand") && searchParameters["VisitsBeforeExpand"].is_number_integer(),
            "SearchParameters must incldue an int VisitsBeforeExpand");
        m_params.setNodeVisitsBeforeExpand(searchParameters["VisitsBeforeExpand"]);

        if (searchParameters.count("Simulations"))
        {
            BOSS_ASSERT(searchParameters["Simulations"].is_number_integer(), "Simulations must be an integer");
            m_params.setNumberOfSimulations(searchParameters["Simulations"]);
        }

        if (searchParameters.count("Nodes"))
        {
            BOSS_ASSERT(searchParameters["Nodes"].is_number_integer(), "Nodes must be an integer");
            m_params.setNodeLimit(searchParameters["Nodes"]);
        }

        if (searchParameters.count("Threads"))
        {
            BOSS_ASSERT(searchParameters["Threads"].is_number_integer(), "Threads must be an integer");
            m_params.setThreads(searchParameters["Threads"]);
        }


        if (exp.count("ChangingRoot"))
        {
            //BOSS_ASSERT(exp["ChangingRoot"].is_array() && exp["ChangingRoot"].size() > 0, "ChangingRoot must be an array");
            auto& params = exp["ChangingRoot"];
            BOSS_ASSERT(params.count("Active") && params["Active"].is_boolean(), "Must have a boolean 'Active' member inside ChangingRoot");
            BOSS_ASSERT(params.count("Simulations") && params["Simulations"].is_number_integer(), "Must have an integer 'Simulations' member inside of SimulationsPerStep");
            BOSS_ASSERT(params.count("Reset") && params["Reset"].is_boolean(), "Must have a boolean 'Reset' member inside ChangingRoot");

            m_params.setChangingRoot(params["Active"]);
            m_params.setSimulationsPerStep(params["Simulations"]);
            m_params.setChangingRootReset(params["Reset"]);
        }
    }

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

        ActionSet relevantActionSet;

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

    //if (searchType == "IntegralMCTS")
    //{
    //    // set some global variables inside of Edge
    //    Edge::USE_MAX_VALUE = m_params.getUseMaxValue();
    //    if (m_params.usePolicyValueNetwork())
    //    {
    //        Edge::MIXING_VALUE = m_params.getMixingValue();
    //    }
    //    Edge::NODE_VISITS_BEFORE_EXPAND = m_params.getNodeVisitsBeforeExpand();
    //    Edge::MAX_EDGE_VALUE_EXPECTED = (FracType)m_params.getValueNormalization();
    //}

    Eval::CalculateUnitValues(m_params.getInitialState());
    Eval::SetUnitWeightVector(Eval::CalculateUnitWeightVector(m_params.getInitialState(), m_params.getEnemyUnits()));
    Eval::setUnitWeightsString();
}

void IntegralExperiment::runExperimentThread(int run)
{
    static std::string stars = "************************************************";

    std::string name = m_name;
    if (m_searchType != "IntegralDFS")
    {
        name += "Run" + std::to_string(run);
    }
    std::string outputDir = m_outputDir + "/" + Assert::CurrentDateTime() + "_" + name;
    FileTools::MakeDirectory(outputDir);

    std::string resultsFile = name;

    std::cout << "\n" << stars << "\n* Running Experiment: " << name << " [" << m_searchType << "]\n" << stars << "\n";

    std::unique_ptr<CombatSearch> combatSearch;

    if (m_searchType == "IntegralDFS")
    {
        combatSearch = std::unique_ptr<CombatSearch>(new CombatSearch_Integral_DFS(m_params));
    }
    //else if (m_searchType == "IntegralMCTS")
    //{
    //    //resultsFile += "_IntegralMCTS";
    //    //combatSearch = std::unique_ptr<CombatSearch>(new CombatSearch_IntegralMCTS(m_params, outputDir, resultsFile, m_name));
    //}
    //else if (m_searchType == "ParallelIntegralMCTS")
    //{
    //    //resultsFile += "_IntegralMCTS";
    //    combatSearch = std::unique_ptr<CombatSearch>(new CombatSearch_Integral_MCTS(m_params, outputDir, resultsFile, m_name));
    //}
    else
    {
        BOSS_ASSERT(false, "CombatSearch type not found: %s", m_searchType.c_str());
    }
    
    combatSearch->search();
    /*#pragma omp critical
    combatSearch->printResults();*/
    combatSearch->writeResultsFile(outputDir, resultsFile);
}

// TODO: FIX RUNTOTALTIMEEXPERIMENT
void IntegralExperiment::runTotalTimeExperiment(int run)
{
    BOSS_ASSERT(false, "Not implemented yet");
    /*
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
        std::unique_ptr<CombatSearch> combatSearch = std::unique_ptr<CombatSearch>(new CombatSearch_Integral_MCTS(params, dir, resultsFile, m_name));

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
/*
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
    */
}

void IntegralExperiment::run(int numberOfRuns)
{
    FileTools::MakeDirectory(m_outputDir);
    
    int threads = std::max(1, int(float(std::thread::hardware_concurrency()) / m_params.getThreads()));

    #pragma omp parallel for num_threads(threads)
    for (int run = 0; run < numberOfRuns; ++run)
    {
        if (m_params.getUseTotalTimeLimit())
        {
            runTotalTimeExperiment(run);
        }
        else
        {
            runExperimentThread(run);
        }
    }
}
