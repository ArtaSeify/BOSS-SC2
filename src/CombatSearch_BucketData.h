/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "BuildOrderAbilities.h"
#include "Common.h"
#include "GameState.h"
#include "Eval.h"

namespace BOSS
{

    class BucketData
    {
    public:
        double                      eval;
        BuildOrderAbilities         buildOrder;
        GameState                   state;

        BucketData()
            : eval(0)
        {
        }
    };

    class CombatSearch_BucketData
    {
        std::vector<BucketData>     m_buckets;
        int                         m_frameLimit;

        BucketData & getBucketData(const GameState & state);

    public:

    CombatSearch_BucketData(int frameLimit, size_t numBuckets);

    const BucketData & getBucket(size_t index) const;
    size_t numBuckets() const;
    size_t getBucketIndex(const GameState & state) const;
        
        void update(const GameState & state, const BuildOrderAbilities & buildOrder);

        bool isDominated(const GameState & state);

        void print() const;
        std::string getBucketResultsString();
    };

}
