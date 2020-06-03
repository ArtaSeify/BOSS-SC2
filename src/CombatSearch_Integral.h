#pragma once

#include "Common.h"
#include "GameState.h"
#include "CombatSearch.h"
#include "CombatSearch_IntegralData_FinishedUnits.h"

namespace BOSS
{
    class CombatSearch_Integral : public CombatSearch
    {
    protected:
        CombatSearch_IntegralData_FinishedUnits m_integral;

    public:
        BuildOrder createFinishedUnitsBuildOrder(const BuildOrder& buildOrder) const;
        BuildOrder createUsefulBuildOrder(const BuildOrder& buildOrder) const;

        virtual void printResults();
    };
}

