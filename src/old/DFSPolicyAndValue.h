#pragma once
#include "Common.h"
#include "CombatSearch_Integral.h"
#include "ActionSetAbilities.h"

namespace BOSS
{
    class DFSPolicyAndValue : public CombatSearch_Integral
    {
    private:
        std::vector<ActionValue> evaluateStates(const GameState & state, ActionSetAbilities & legalActions);
        

    protected:
        FracType recurseReturnValue(const GameState & state, int depth);
    
    public:
        DFSPolicyAndValue(const CombatSearchParameters p = CombatSearchParameters(),
            const std::string & dir = "", const std::string & prefix = "", const std::string & name = "");

        virtual void recurse(const GameState & state, int depth);
    };
}
