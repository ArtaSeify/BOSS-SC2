/* -*- c-basic-offset: 4 -*- */

#include "CombatSearchResults.h"

using namespace BOSS;

CombatSearchResults::CombatSearchResults()
    : solved(false)
    , timedOut(false)
    , solutionLength(-1)
    , upperBound(-1)
    , lowerBound(-1)
    , numSimulations(0)
    , nodesExpanded(0)
    , nodeVisits(0)
    , leafNodesExpanded(0)
    , leafNodesVisited(0)
    , timeElapsed(0)
    , timeElapsedCPU(0)
    , avgBranch(0)
    , eval(0)
    , finishedEval(0)
    , usefulEval(0)
    , minerals(0)
    , gas(0)
    , frameCompleted(0)
{
}

void CombatSearchResults::printResults(bool pbo)
{
    printf("%12d%12d%12d%14llu%12.4lf%12.2lf%12.2lf       ",upperBound,lowerBound,solutionLength,nodesExpanded,avgBranch,timeElapsed,(nodesExpanded/(timeElapsed/1000.0)));

    if (pbo)
    {
        printBuildOrder();
    }

    printf("\n");
}

/*void CombatSearchResults::printBuildOrder()
{
    for (size_t i(0); i<buildOrder.size(); ++i)
    {
        printf("%d ", buildOrder[buildOrder.size()-1-i].getID());
    }
}*/

void CombatSearchResults::printBuildOrder()
{
    for (auto & actionTargetPair : buildOrder)
    {
        printf("%s %d", actionTargetPair.first.getName().c_str(), actionTargetPair.second.targetID);
    }
}
