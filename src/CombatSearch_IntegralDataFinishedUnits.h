#pragma once

#include "Common.h"
#include "GameState.h"
#include "CombatSearchParameters.h"
#include "Eval.h"
#include "BuildOrderAbilities.h"

namespace BOSS
{

    class IntegralDataFinishedUnits
    {
    public:
        double  eval;
        double  integral_ToThisPoint;
        double  integral_UntilFrameLimit;
        size_t     timeStarted;
        size_t     timeFinished;
        char id;

        IntegralDataFinishedUnits(double e, double i_c, double i_l, size_t t_a, size_t t_f)
            : eval(e)
            , integral_ToThisPoint(i_c)
            , integral_UntilFrameLimit(i_l)
            , timeStarted(t_a)
            , timeFinished(t_f)
            , id(0)
        {

        }

        IntegralDataFinishedUnits(double e, double i_c, double i_l, size_t t_a, size_t t_f, char newID)
            : eval(e)
            , integral_ToThisPoint(i_c)
            , integral_UntilFrameLimit(i_l)
            , timeStarted(t_a)
            , timeFinished(t_f)
            , id(newID)
        {

        }

        IntegralDataFinishedUnits()
            : eval(0)
            , integral_ToThisPoint(0)
            , integral_UntilFrameLimit(0)
            , timeStarted(0)
            , timeFinished(0)
            , id(0)
        {

        }
    };

    class CombatSearch_IntegralDataFinishedUnits
    {
        std::vector<IntegralDataFinishedUnits>       m_integralStack;
        size_t                                       m_chronoBoostEntries;

        std::vector<IntegralDataFinishedUnits>       m_bestIntegralStack;
        double                                       m_bestIntegralValue;
        BuildOrderAbilities                          m_bestIntegralBuildOrder;
        GameState                                    m_bestIntegralGameState;

        BuildOrderAbilities createBuildOrderEndTimes(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state) const;

    public:

        CombatSearch_IntegralDataFinishedUnits();

        void update(const GameState & state, const BuildOrderAbilities & buildOrder, const CombatSearchParameters & params);
        void pop_back();
        void popFinishedLastOrder(const GameState & prevState, const GameState & currState);

        void print() const;
        void printIntegralData(const size_t index, const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrderAbilities & buildOrder) const;
        void print(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrderAbilities & buildOrder) const;

        const BuildOrderAbilities & getBestBuildOrder() const;
    };

}
