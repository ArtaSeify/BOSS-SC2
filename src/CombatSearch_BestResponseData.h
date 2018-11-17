/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "Eval.h"
#include "BuildOrderAbilities.h"

namespace BOSS
{
    
    class CombatSearch_BestResponseData
    {
        GameState m_enemyInitialState;
        BuildOrderAbilities m_enemyBuildOrder;

        std::vector<GameState> m_enemyStates;
        std::vector< std::pair<TimeType, FracType>> m_enemyArmyValues;
        std::vector< std::pair<TimeType, FracType>> m_selfArmyValues;

        float m_bestEval;
        BuildOrderAbilities m_bestBuildOrder;
        GameState m_bestState;

        float compareBuildOrder(const GameState & state,
                                const BuildOrderAbilities & buildOrder);
        size_t getStateIndex(const GameState & state);

        void calculateArmyValues(const GameState & state,
                                 const BuildOrderAbilities & buildOrder,
                                 std::vector<std::pair<TimeType, FracType>> & values);

    public:

        CombatSearch_BestResponseData(const GameState & enemyState,
                                      const BuildOrderAbilities & enemyBuildOrder);

        void update(const GameState & initialState,
                    const GameState & currentState,
                    const BuildOrderAbilities & buildOrder);

        const BuildOrderAbilities & getBestBuildOrder() const;
    };
}
