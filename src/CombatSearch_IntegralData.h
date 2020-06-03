/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "Eval.h"
#include "BuildOrder.h"

namespace BOSS
{
    
    class IntegralData
    {
    public:
        FracType eval;
        FracType integral;
        int    timeAdded;

        IntegralData(FracType e, FracType i, int t)
            : eval(e)
            , integral(i)
            , timeAdded(t)
        {
    
        }
    
        IntegralData()
            : eval(0)
            , integral(0)
            , timeAdded(0)
        {
    
        }
    };

    class CombatSearch_IntegralData
    {
        std::vector<IntegralData> m_integralStack;
        std::vector<IntegralData> m_bestIntegralStack;
        FracType                  m_bestIntegralValue;
        BuildOrder       m_bestIntegralBuildOrder;
        GameState                 m_bestIntegralGameState;

    public:

        CombatSearch_IntegralData();

        void update(const GameState & state, const BuildOrder & buildOrder);
        void pop_back();

        void printIntegralData(int index) const;
        void print() const;

        const BuildOrder & getBestBuildOrder() const;
    };
}
