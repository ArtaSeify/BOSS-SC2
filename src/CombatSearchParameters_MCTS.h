#pragma once

#include "Common.h"
#include "CombatSearchParameters.h"

namespace BOSS
{
    class CombatSearchParameters_MCTS : public CombatSearchParameters
    {
        struct ChangingRoot
        {
            //      Setting this flag will make the search commit to an action after performing
            //      m_simulationsPerStep. This has been shown to improve the performance of single-player
            //      MCTS. e.g. see https://project.dke.maastrichtuniversity.nl/games/files/articles/KNOSYS_SameGame.pdf
            bool changingRoot;

            //      Setting this flag will reset all the information in the tree after 
            //      changing the root.
            bool changingRootReset;

            //      The number of simulations that will be performed before the root is changed
            int simulationsPerStep;

            ChangingRoot() : changingRoot(false), changingRootReset(false), simulationsPerStep(std::numeric_limits<int>::max()) {}
        };

        ChangingRoot m_changingRoot;

        //      The value of the exploration parameter C
        FracType m_explorationValue;

        //      Limits the number of nodes that can be traveresed during the search.
        //      This includes expanded nodes and nodes visited during the simulation
        //      stage.
        uint8 m_nodeLimit;

        //      The number of simulations that will be performed before search is finished.
        int m_numberOfSimulations;

        //      Number of parallel threads (Tree Parallelization)
        int m_threads;

        //      When search is finished, there could be remaining time left. By setting
        //      this flag to true, a new search will start when the last one is finished,
        //      until all the time is spent. The results of each search will be saved and
        //      put into separate files.
        bool m_useTotalTimeLimit;

        //      Number of times a node must be visited before it is expanded (added to the search tree)
        int m_nodeVisitsBeforeExpand;

    public:
        CombatSearchParameters_MCTS();

        void setExplorationValue(FracType value);
        FracType getExplorationValue() const;

        void setChangingRoot(bool value);
        bool getChangingRoot() const;

        void setChangingRootReset(bool value);
        bool getChangingRootReset() const;

        void setNumberOfSimulations(int value);
        int getNumberOfSimulations() const;

        void setNodeLimit(uint8 value);
        uint8 getNodeLimit() const;

        void setSimulationsPerStep(int value);
        int getSimulationsPerStep() const;

        void setThreads(int threads);
        int getThreads() const;

        void setUseTotalTimeLimit(bool value);
        bool getUseTotalTimeLimit() const;

        void setNodeVisitsBeforeExpand(int value);
        int getNodeVisitsBeforeExpand() const;
    };
}

