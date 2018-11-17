/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "ActionType.h"
#include "GameState.h"
#include "BuildOrder.h"

namespace BOSS
{
    class DFBB_BuildOrderSearchResults
    {

    public:
   
	BuildOrder buildOrder;  // the build order

	bool solved;            // whether ot not a solution was found
        bool timedOut;          // did the search time-out?
        bool solutionFound;     // did we find any solution
	
	int upperBound;		// upper bound of first node
	
	uint8 nodesExpanded;	// number of nodes expanded in the search
	
	float timeElapsed;	// time elapsed in milliseconds

        GameState finalState;
	
	DFBB_BuildOrderSearchResults();
		
	void printResults(bool pbo = true) const;
	void printBuildOrder() const;
    };
}
