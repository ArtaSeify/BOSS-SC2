/* -*- c-basic-offset: 4 -*- */

#include "Experiments.h"
#include "IntegralExperimentOMP.h"
#include "BuildOrderPlotter.h"
#include "FileTools.h"
#include <thread>
#include <future>
#include <omp.h>

using namespace BOSS;

void ExperimentsArta::RunExperiments(const std::string & experimentFilename)
{
    omp_set_nested(true);
    omp_set_dynamic(true);
    std::ifstream file(experimentFilename);
    json j;
    file >> j;

    BOSS_ASSERT(j.count("Experiments"), "No 'Experiments' member found");
    BOSS_ASSERT(j.count("ExperimentsInParallel") && j["ExperimentsInParallel"].is_number_integer(), "Need integer 'ExperimentsInParallel'");
    std::vector<int> experimentsPerThread = threadSplit(int(j["Experiments"].size()), j["ExperimentsInParallel"]);

    /*for (int threadRun : experimentsPerThread)
    {
        std::cout << threadRun << std::endl;
    }*/

    std::vector<std::future<void>> threads(j["ExperimentsInParallel"].get<int>());
    int startingIndex = 0;
    for (int thread = 0; thread < threads.size(); ++thread)
    {
        threads[thread] = std::async(runExperimentsThread, j, thread, experimentsPerThread[thread], startingIndex);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        startingIndex += experimentsPerThread[thread];
    }
    
    for (auto & thread : threads)
    {
        thread.wait();
    }

    std::cout << "\n\n";
}

void ExperimentsArta::runExperimentsThread(const json & j, int thread, int experimentsForThread, int startingIndex)
{
    //#pragma omp parallel for
    for (int index = 0; index < experimentsForThread; ++index)
    {
        // move iterator forward
        auto it = j["Experiments"].begin();
        std::advance(it, index + startingIndex);

        const std::string &         experimentName = it.key();
        const json &                val = it.value();
        
        //std::cout << "Found Experiment:   " << name << std::endl;
        BOSS_ASSERT(val.count("Run"), "Experiment has no Run value");
        BOSS_ASSERT(val["Run"].is_array() && val["Run"][0].is_boolean() && val["Run"][1].is_number_integer(), "Run must be a <bool, int> array");

        if (val["Run"][0] == true)
        {
            BOSS_ASSERT(val.count("SearchType"), "Experiment has no SearchType value");
            BOSS_ASSERT(val["SearchType"].is_array() && val["SearchType"][0].is_string(), "SearchType must an array with first element a string");
            if (val["SearchType"] == "IntegralMCTS" || val["SearchType"] == "ParallelIntegralMCTS")
            {
                auto & searchParameters = val["SearchParameters"];
                BOSS_ASSERT(searchParameters.count("ExplorationConstant") && searchParameters["ExplorationConstant"].is_number_float(), "There must be a float ExplorationConstant");
                BOSS_ASSERT((searchParameters.count("Nodes") && searchParameters["Nodes"].is_number_integer()) || (searchParameters.count("Simulations") && searchParameters["Simulations"].is_number_integer()), "There must be an int Nodes or Simulations");
                BOSS_ASSERT(searchParameters.count("UseMax") && searchParameters["UseMax"].is_boolean(), "There must be a bool UseMax");
            }

            else if (val["SearchType"] == "IntegralNMCS")
            {
                auto & searchParameters = val["SearchParameters"];
                BOSS_ASSERT(searchParameters.count("Playouts") && searchParameters["Playouts"].is_number_integer(), "There must be an int Playouts");
                BOSS_ASSERT(searchParameters.count("Level") && searchParameters["Level"].is_number_integer(), "There must be an int Level");
            }

            else if (val["SearchType"] == "IntegralNMCTS")
            {
                auto & searchParameters = val["SearchParameters"];
                BOSS_ASSERT(searchParameters.count("Playouts") && searchParameters["Playouts"].is_number_integer(), "There must be an int Playouts");
                BOSS_ASSERT(searchParameters.count("Level") && searchParameters["Level"].is_number_integer(), "There must be an int Level");
                BOSS_ASSERT(searchParameters.count("UseMax") && searchParameters["UseMax"].is_boolean(), "There must be a bool UseMax");
            }

            const std::string & searchType = val["SearchType"][0].get<std::string>();

            if (searchType == "IntegralDFS" || searchType == "IntegralDFSVN" || searchType == "IntegralDFSPN" || searchType == "IntegralDFSPVN")
            {
                RunDFSExperiment(experimentName, val, val["Run"][1]);
            }
            else if (searchType == "IntegralMCTS" || searchType == "ParallelIntegralMCTS")
            {
                RunMCTSExperiment(experimentName, val, val["Run"][1]);
            }
            else if (searchType == "IntegralNMCS")
            {
                RunNMCSExperiment(experimentName, val, val["Run"][1]);
            }
            else if (searchType == "IntegralNMCTS")
            {
                RunNMCTSExperiment(experimentName, val, val["Run"][1]);
            }
            else
            {
                BOSS_ASSERT(false, "Unknown Experiment Type: %s", searchType.c_str());
            }
        }
    }
}

std::vector<int> ExperimentsArta::threadSplit(int numExperiments, int experimentsInParallel)
{
    std::vector<int> experimentsPerThread(experimentsInParallel);
    int expsPerThread = numExperiments / experimentsInParallel;

    for (int i = 0; i < experimentsInParallel; ++i)
    {
        experimentsPerThread[i] = expsPerThread;
    }

    // can't evenly divide the experiments
    if (numExperiments % experimentsInParallel != 0)
    {
        int experimentsLeft = numExperiments - (expsPerThread * experimentsInParallel);
        for (int i = 0; i < experimentsLeft; ++i)
        {
            experimentsPerThread[i]++;
        }
    }

    return experimentsPerThread;
}

void ExperimentsArta::RunDFSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns)
{
    std::cout << "DFS Integral Search Experiment - " << experimentName << std::endl;

    IntegralExperimentOMP intexp(experimentName, exp);
    intexp.run(numberOfRuns);

    std::cout << "    " << experimentName << " completed" << std::endl;
}

void ExperimentsArta::RunMCTSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns)
{
    std::cout << "MCTS Search Experiment - " << experimentName << std::endl;

    IntegralExperimentOMP intexp(experimentName, exp);
    intexp.run(numberOfRuns);

    std::cout << "    " << experimentName << " completed" << std::endl;
}

void ExperimentsArta::RunNMCSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns)
{
    std::cout << "NMCS Search Experiment - " << experimentName << std::endl;

    IntegralExperimentOMP intexp(experimentName, exp);
    intexp.run(numberOfRuns);

    std::cout << "    " << experimentName << " completed" << std::endl;
}

void ExperimentsArta::RunNMCTSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns)
{
    std::cout << "NMCTS Search Experiment - " << experimentName << std::endl;

    IntegralExperimentOMP intexp(experimentName, exp);
    intexp.run(numberOfRuns);

    std::cout << "    " << experimentName << " completed" << std::endl;
}