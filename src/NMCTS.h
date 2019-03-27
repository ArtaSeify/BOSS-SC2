#pragma once

#include "Common.h"
#include "CombatSearch_IntegralMCTS.h"

namespace BOSS
{
    class NMCTS : public CombatSearch_IntegralMCTS
    {
    public:
        typedef std::pair<CombatSearch_IntegralDataFinishedUnits, BuildOrderAbilities> SearchResult;

    private:
        void recurse(const GameState & state, int depth);

        SearchResult executeSearch(std::shared_ptr<Node> node, int level);

        int iterationsForLevel(int level) const;

        void backPropogation(std::shared_ptr<Node> node, FracType value);

    public:
        NMCTS(const CombatSearchParameters p = CombatSearchParameters(), const std::string & dir = "",
            const std::string & prefix = "", const std::string & name = "");
    };
}
