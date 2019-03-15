#pragma once
#include "Common.h"
#include "CombatSearch_IntegralMCTS.h"

namespace BOSS
{
    class NMCS : public CombatSearch_IntegralMCTS
    {
        
        void recurse(const GameState & state, int depth);

        std::pair<CombatSearch_IntegralDataFinishedUnits, BuildOrderAbilities>
            executeSearch(Node node, int level, int depth);

        std::pair<CombatSearch_IntegralDataFinishedUnits, BuildOrderAbilities> 
            randomPlayout(Node node);

        //std::pair<CombatSearch_IntegralDataFinishedUnits, BuildOrderAbilities>
         //   updateBOIntegral(const Node & node, const ActionAbilityPair & action, const GameState & prevGameState, bool permanantUpdate);

        bool timeLimitReached();

        virtual void printResults();
        virtual void writeResultsFile(const std::string & dir, const std::string & filename);

    public:
        NMCS(const CombatSearchParameters p = CombatSearchParameters());

        NMCS(const CombatSearchParameters p = CombatSearchParameters(), const std::string & dir = "",
            const std::string & prefix = "", const std::string & name = "");

    };
}

