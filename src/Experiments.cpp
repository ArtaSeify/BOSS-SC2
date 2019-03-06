/* -*- c-basic-offset: 4 -*- */

#include "Experiments.h"
#include "IntegralExperiment.h"
#include "BuildOrderPlotter.h"
#include "FileTools.h"

using namespace BOSS;

void ExperimentsArta::RunExperiments(const std::string & experimentFilename)
{
    std::ifstream file(experimentFilename);
    json j;
    file >> j;

    BOSS_ASSERT(j.count("Experiments"), "No 'Experiments' member found");

    for (auto it = j["Experiments"].begin(); it != j["Experiments"].end(); ++it)
    {
        const std::string &         experimentName = it.key();
        const json &                val = it.value();

        //std::cout << "Found Experiment:   " << name << std::endl;
        BOSS_ASSERT(val.count("Run"), "Experiment has no Run value");
        BOSS_ASSERT(val["Run"].is_array() && val["Run"][0].is_boolean() && val["Run"][1].is_number_integer(), "Run must be a <bool, int> array");

        if (val["Run"][0] == true)
        {
            BOSS_ASSERT(val.count("SearchType"), "Experiment has no SearchType value");
            BOSS_ASSERT(val["SearchType"].is_array() && val["SearchType"][0].is_string(), "SearchType must be an array, and first element must be a string");
            if (val["SearchType"] == "IntegralMCTS")
            {
                BOSS_ASSERT(val["SearchType"].size() == 4 && val["SearchType"][1].is_number_float()
                                && val["SearchType"][2].is_number_integer() && val["SearchType"][3].is_boolean(), 
                                "Format for MCTS search is: [MCTS, explorationValue, numSimulations]");
            }
            const std::string & searchType = val["SearchType"][0].get<std::string>();

            if (searchType == "IntegralDFS")
            {
                RunDFSExperiment(experimentName, val, val["Run"][1]);
            }
            else if (searchType == "IntegralMCTS")
            {
                RunMCTSExperiment(experimentName, val, val["Run"][1]);
            }
            else
            {
                BOSS_ASSERT(false, "Unknown Experiment Type: %s", searchType.c_str());
            }
        }
    }

    std::cout << "\n\n";
}

void ExperimentsArta::RunDFSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns)
{
    std::cout << "DFS Integral Search Experiment - " << experimentName << std::endl;

    IntegralExperiment intexp(experimentName, exp);
    intexp.run(numberOfRuns);

    std::cout << "    " << experimentName << " completed" << std::endl;
}

void ExperimentsArta::RunMCTSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns)
{
    std::cout << "MCTS Search Experiment - " << experimentName << std::endl;

    IntegralExperiment intexp(experimentName, exp);
    intexp.run(numberOfRuns);

    std::cout << "    " << experimentName << " completed" << std::endl;
}