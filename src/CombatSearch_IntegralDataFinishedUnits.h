/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "CombatSearchParameters.h"
#include "Eval.h"
#include "BuildOrderAbilities.h"
#include "Timer.hpp"

namespace BOSS
{

    class IntegralDataFinishedUnits
    {
    public:
        FracType    eval;
        FracType    integral_ToThisPoint;
        FracType    integral_UntilFrameLimit;
        TimeType    timeStarted;
        TimeType    timeFinished;
        sint1       id;

        IntegralDataFinishedUnits(FracType e, FracType i_c, FracType i_l, TimeType t_a, TimeType t_f)
            : eval(e)
            , integral_ToThisPoint(i_c)
            , integral_UntilFrameLimit(i_l)
            , timeStarted(t_a)
            , timeFinished(t_f)
            , id(0)
        {

        }

        IntegralDataFinishedUnits(FracType e, FracType i_c, FracType i_l, TimeType t_a, TimeType t_f, sint1 newID)
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
        std::vector<IntegralDataFinishedUnits>      m_integralStack;
        int                                         m_chronoBoostEntries;

        std::vector<IntegralDataFinishedUnits>      m_bestIntegralStack;
        FracType                                    m_bestIntegralValue;
        BuildOrderAbilities                         m_bestIntegralBuildOrder;
        GameState                                   m_bestIntegralGameState;

        BuildOrderAbilities createBuildOrderEndTimes(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state) const;

    public:
        CombatSearch_IntegralDataFinishedUnits();

        void update(const GameState & state, const BuildOrderAbilities & buildOrder, const CombatSearchParameters & params, Timer & timer, bool useTieBreaker=true);
        void addUnitEntry(const GameState & state, int unitIndex, TimeType startFrame, TimeType endFrame, const CombatSearchParameters & params);
        void addChronoBoostEntry(TimeType startFrame, TimeType endFrame, const CombatSearchParameters & params);
        
        void clear();
        void pop_back();
        void popFinishedLastOrder(const GameState & prevState, const GameState & currState);

        void print() const;
        void printIntegralData(const int index, const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrderAbilities & buildOrder) const;
        void print(const std::vector<IntegralDataFinishedUnits> & integral_stack, const GameState & state, const BuildOrderAbilities & buildOrder) const;

        void writeToFile(const std::string & dir, const std::string & filename);

        const BuildOrderAbilities & getBestBuildOrder() const;
        FracType getValueToThisPoint() const { return m_integralStack.back().integral_ToThisPoint; }
        FracType getCurrentStackValue() const { return m_integralStack.back().integral_UntilFrameLimit; }
        FracType getBestStackValue() const { return m_bestIntegralValue; }
    };
}
