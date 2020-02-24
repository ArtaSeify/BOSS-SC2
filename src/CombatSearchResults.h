/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "ActionType.h"
#include "Timer.hpp"
#include "BuildOrderAbilities.h"

namespace BOSS
{
    class CombatSearchResults
    {

    public:

        bool solved;            // whether ot not a solution was found
        bool timedOut;          // did the search time-out?

        int solutionLength;     // the length of the solution
        int upperBound;         // upper bound of first node
        int lowerBound;         // lower bound of first node

        int numSimulations;

        uint8  nodesExpanded;   // number of nodes expanded in the search
        uint8  nodeVisits;      // number of nodes visited. Duplicates are counted
        uint8  leafNodesExpanded; // number of leaf nodes expanded in the search
        uint8  leafNodesVisited;  // number of leaf nodes visited. Duplicates are counted

        double timeElapsed;     // time elapsed in milliseconds
        double timeElapsedCPU;  // time elapsed in CPU milliseconds
        float avgBranch;        // avg branching factor

        Timer searchTimer;         

        GameState winner;

        //std::vector<ActionType> buildOrder; // the build order
        BuildOrderAbilities buildOrder;
        BuildOrderAbilities finishedUnitsBuildOrder;
        BuildOrderAbilities usefulBuildOrder;

        float eval;
        float finishedEval;
        float usefulEval;

        float value;
        float finishedValue;
        float usefulValue;

        float minerals;
        float gas;

        int frameCompleted;

        CombatSearchResults();
        CombatSearchResults(bool s, int len, uint8 n, float t, const std::vector<ActionType> & solution);

        void printResults(bool pbo = true);
        void printBuildOrder();
    };
}
