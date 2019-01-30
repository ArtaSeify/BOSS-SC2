/* -*- c-basic-offset: 4 -*- */

#include "IntegralExperiment.h"
#include "CombatSearch.h"
#include "CombatSearch_Integral.h"
#include "CombatSearch_Bucket.h"
#include "CombatSearch_BestResponse.h"
#include "CombatSearch_IntegralMCTS.h"
#include "FileTools.h"

using namespace BOSS;

IntegralExperiment::IntegralExperiment()
    : m_race(Races::None)
{

}

IntegralExperiment::IntegralExperiment(const std::string & experimentName, const json & exp)
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

    BOSS_ASSERT(exp.count("SaveResults") && exp["SaveResults"].is_boolean(), "IntegralSearch must have a SaveResults bool");
    m_params.setSaveResults(exp["SaveResults"]);

    const std::string & searchType = exp["SearchType"][0].get<std::string>();
    m_searchType = searchType;

    if (searchType == "IntegralMCTS")
    {
        m_params.setExplorationValue(exp["SearchType"][1]);
        m_params.setNumberOfSimulations(exp["SearchType"][2]);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_params.getExplorationValue();
        m_name += "C" + ss.str();
    }

    if (searchType == "IntegralDFS")
    {
        m_name += "DFS";
    }

    if (exp.count("MaxActions"))
    {
        const json & maxActions = exp["MaxActions"];
        BOSS_ASSERT(maxActions.is_array(), "MaxActions is not an array");

        for (size_t i(0); i < maxActions.size(); ++i)
        {
            BOSS_ASSERT(maxActions[i].is_array(), "MaxActions element must be array of size 2");

            BOSS_ASSERT(maxActions[i].size() == 2 && maxActions[i][0u].is_string() && maxActions[i][1u].is_number_integer(), "MaxActions element must be [\"Action\", Count]");

            const std::string & typeName = maxActions[i][0u];

            BOSS_ASSERT(ActionTypes::TypeExists(typeName), "Action Type doesn't exist: %s", typeName.c_str());

            m_params.setMaxActions(ActionTypes::GetActionType(typeName), maxActions[i][1]);
        }
    }

    if (exp.count("RelevantActions"))
    {
        const json & relevantActions = exp["RelevantActions"];
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
        const json & brVal = exp["BestResponseParams"];

        BOSS_ASSERT(brVal.is_object(), "BestResponseParams not an object");
        BOSS_ASSERT(brVal.count("EnemyState"), "bestResponseParams must have 'enemyState' string");
        BOSS_ASSERT(brVal.count("EnemyBuildOrder"), "bestResponseParams must have 'enemyBuildOrder' string");

        BOSS_ASSERT(brVal.count("EnemyState") && brVal["EnemyState"].is_string(), "bestResponseParams must have a 'EnemyState' string");
        m_params.setEnemyInitialState(BOSSConfig::Instance().GetState(brVal["EnemyState"]));

        BOSS_ASSERT(brVal.count("EnemyBuildOrder") && brVal["EnemyBuildOrder"].is_string(), "BestResponseParams must have a 'EnemyBuildOrder' string");
        m_params.setEnemyBuildOrder(BOSSConfig::Instance().GetBuildOrder(brVal["EnemyBuildOrder"]));
    }
}

void IntegralExperiment::run(int numberOfRuns)
{
    FileTools::MakeDirectory(m_outputDir);

    static std::string stars = "************************************************";
    for (int i(0); i < numberOfRuns; ++i)
    {
        std::string name = m_name + "Run" + std::to_string(i);
        std::string outputDir = m_outputDir + "/" + Assert::CurrentDateTime() + "_" + name;
        FileTools::MakeDirectory(outputDir);

        std::shared_ptr<CombatSearch> combatSearch;
        std::string resultsFile = name;

        std::cout << "\n" << stars << "\n* Running Experiment: " << name << " [" << m_searchType << "]\n" << stars << "\n";

        if (m_searchType == "Integral")
        {
            combatSearch = std::shared_ptr<CombatSearch>(new CombatSearch_Integral(m_params));
            //resultsFile += "_Integral";
        }
        else if (m_searchType == "Bucket")
        {
            combatSearch = std::shared_ptr<CombatSearch>(new CombatSearch_Bucket(m_params));
            //resultsFile += "_Bucket";
        }
        else if (m_searchType == "BestResponse")
        {
            combatSearch = std::shared_ptr<CombatSearch>(new CombatSearch_BestResponse(m_params));
            //resultsFile += "_BestResponse";
        }
        else if (m_searchType == "IntegralMCTS")
        {
            //resultsFile += "_IntegralMCTS";
            combatSearch = std::shared_ptr<CombatSearch>(new CombatSearch_IntegralMCTS(m_params, outputDir, resultsFile));
        }
        else
        {
            BOSS_ASSERT(false, "CombatSearch type not found: %s", m_searchType.c_str());
        }

        combatSearch->search();
        combatSearch->printResults();
        combatSearch->writeResultsFile(outputDir, resultsFile);
        const CombatSearchResults & results = combatSearch->getResults();
        if (m_searchType != "IntegralMCTS")
        {
            std::cout << "\nSearched " << results.nodesExpanded << " nodes in " << results.timeElapsed << "ms @ " << (1000.0*results.nodesExpanded / results.timeElapsed) << " nodes/sec\n\n";
        }
    }
}
