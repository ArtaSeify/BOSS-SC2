/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "JSONTools.h"

namespace BOSS
{
    namespace ExperimentsArta
    {
        void RunExperiments(const std::string & experimentFilename);
        void RunDFSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns);
        void RunMCTSExperiment(const std::string & experimentName, const json & exp, int numberOfRuns);
    }

}