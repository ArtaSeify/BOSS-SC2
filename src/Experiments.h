/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "JSONTools.h"

namespace BOSS
{
    namespace ExperimentsArta
    {
        void RunExperiments(const std::string & experimentFilename);
        std::vector<int> threadSplit(int numExperiments, int experimentsInParallel);
        void runExperimentsThread(const json & j, int thread, int experimentsPrevThread, int startingIndex);
        void RunDFSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns);
        void RunMCTSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns);
        void RunNMCSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns);
        void RunNMCTSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns);
    }

}