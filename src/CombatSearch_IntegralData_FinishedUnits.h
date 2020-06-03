/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "CombatSearchParameters.h"
#include "Eval.h"
#include "BuildOrder.h"
#include "Timer.hpp"

namespace BOSS
{

    class IntegralData_FinishedUnits
    {
    public:
        FracType    eval;
        FracType    integral_ToThisPoint;
        FracType    integral_UntilFrameLimit;
        TimeType    timeStarted;
        TimeType    timeFinished;
        sint1       id;

        IntegralData_FinishedUnits(FracType e, FracType i_c, FracType i_l, TimeType t_a, TimeType t_f)
            : eval(e)
            , integral_ToThisPoint(i_c)
            , integral_UntilFrameLimit(i_l)
            , timeStarted(t_a)
            , timeFinished(t_f)
            , id(0)
        {

        }

        IntegralData_FinishedUnits(FracType e, FracType i_c, FracType i_l, TimeType t_a, TimeType t_f, sint1 newID)
            : eval(e)
            , integral_ToThisPoint(i_c)
            , integral_UntilFrameLimit(i_l)
            , timeStarted(t_a)
            , timeFinished(t_f)
            , id(newID)
        {

        }

        IntegralData_FinishedUnits()
            : eval(0.f)
            , integral_ToThisPoint(0.f)
            , integral_UntilFrameLimit(0.f)
            , timeStarted(0)
            , timeFinished(0)
            , id(0)
        {

        }
    };

    class CombatSearch_IntegralData_FinishedUnits
    {
        std::vector<IntegralData_FinishedUnits>      m_integralStack;
        int                                         m_chronoBoostEntries;

        std::vector<IntegralData_FinishedUnits>      m_bestIntegralStack;
        FracType                                    m_bestIntegralValue;
        BuildOrder                         m_bestIntegralBuildOrder;
        GameState                                   m_bestIntegralGameState;

    public:
        CombatSearch_IntegralData_FinishedUnits();

        void update(const GameState & state, const BuildOrder & buildOrder, const CombatSearchParameters & params, Timer & timer, bool useTieBreaker);
        void addUnitEntry(const GameState & state, int unitIndex, TimeType startFrame, TimeType endFrame, const CombatSearchParameters & params);
        void addChronoBoostEntry(TimeType startFrame, TimeType endFrame, const CombatSearchParameters & params);
        
        void clear();
        void pop_back();
        void popFinishedLastOrder(const GameState & prevState, const GameState & currState);

        void print(const BuildOrder & buildOrder = BuildOrder()) const;
        void printIntegralData(int index, const std::vector<IntegralData_FinishedUnits> & integral_stack, const GameState & state, const BuildOrder & buildOrder) const;
        void print(const std::vector<IntegralData_FinishedUnits> & integral_stack, const GameState & state, const BuildOrder & buildOrder) const;

        BuildOrder createBuildOrderEndTimes() const;
        BuildOrder createBuildOrderEndTimes(const std::vector<IntegralData_FinishedUnits> & integral_stack, const GameState & state) const;

        void writeToFile(const std::string & dir, const std::string & filename, const BuildOrder & buildOrder = BuildOrder());

        const BuildOrder & getBestBuildOrder() const;
        FracType getValueToThisPoint() const { return m_integralStack.back().integral_ToThisPoint; }
        FracType getCurrentStackValue() const { return m_integralStack.back().integral_UntilFrameLimit; }
        FracType getCurrentStackEval() const { return m_integralStack.back().eval; }
        FracType getBestStackValue() const { return m_bestIntegralValue; }

        const GameState& getState() const { return m_bestIntegralGameState; }
        void setState(const GameState& state) { m_bestIntegralGameState = state; }
    };
}
