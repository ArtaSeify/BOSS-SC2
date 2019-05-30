/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include "GameState.h"
#include "BuildOrderAbilities.h"

namespace BOSS
{

    class CombatSearchParameters
    {
        std::vector<int> m_maxActions;

        //      Flag which determines whether or not doubling macro actions will be used in search.
        //      Macro actions (see paper) trade suboptimality for decreasing search depth. For large
        //          plans repetitions are necessary. For example, to make probes only build in twos
        //          set useRepetitions = true and repetitionValues[probeAction] = 2
        //
        //      true:  macro actions are used, stored in repetitionValues array
        //      false: macro actions not used, all actions will be carried out once
        bool m_useRepetitions;
        std::vector<int> m_repetitionValues;
    
        //      Flag which determines whether increasing repetitions will be used in search
        //      Increasing repetitions means the reptition value will be 1 until we have at least
        //      repetitionThresholds[a] count of action a. For example, setting:
        //          repetitionThresholds[pylonAction] = 1, repetitionValues[pylonAction] = 2
        //          means that the first pylon will build on its own but all further pylons will
        //          be built 2 at a time.
        //
        //      true:  increasing repetitions are used
        //      false: increasing repetitions not used
        bool m_useIncreasingRepetitions;
        std::vector<int> m_repetitionThresholds;

        //        Flag which determines whether or not we use worker cutoff pruning in search.
        //        Worker cutoff pruning stops workers from being constructed after a certain number
        //            of frames have passed in the search. Intuitively we build the majority of workers
        //            at the beginning of a build, so this can enforce it to make search faster. If
        //          true, workers are no longer legal if currentFrame > workerCutoff * upperBound
        //          in our search algorithm. If workerCutoff is 1, workers will not be pruned.
        //
        //      true:  worker cutoff is used
        //      false: worker cutoff not used
        bool m_useWorkerCutoff;                    
        float m_workerCutoff;
    
        //      Flag which determines whether or not we always make workers during search
        //      This abstraction changes the search so that it always makes a worker if it is able to. It
        //          accomplished this by modifying the current legal actions to exclude anything that
        //          can't be started before the next worker. This ends up producing longer makespans but
        //          the economy it produces is much better. Optimal makespan plans often produce very
        //          few workers and the long term economy suffers.
        //
        //      true:  always make workers is used
        //      false: always make workers is not used
        bool m_useAlwaysMakeWorkers;
    
        //      Flag which determines whether or not we use supply bounding in our search
        //      Supply bounding makes supply producing buildings illegal if we are currently ahead
        //          on supply by a certain amount. If we currently have more than
        //          supplyBoundingThreshold extra supply buildings worth of supply, we no longer
        //          build them. This is an abstraction used to make search faster which may
        //          produce suboptimal plans.
        //
        //      true:  supply bounding is used
        //      false: supply bounding is not used
        bool m_useSupplyBounding;
        int m_supplyBoundingThreshold;
    
    
        //      Flag which determines whether or not we use various heuristics in our search.
        //
        //      true:  the heuristic is used
        //      false: the heuristic is not used
        bool m_useLandmarkLowerBoundHeuristic;
        bool m_useResourceLowerBoundHeuristic;
    
        //      Search time limit measured in milliseconds
        //      If searchTimeLimit is set to a value greater than zero, the search will effectively
        //          time out and the best solution so far will be used in the results. This is
        //          accomplished by throwing an exception if the time limit is hit. Time is checked
        //          once every 1000 nodes expanded, as checking the time is slow.
        float m_searchTimeLimit;
    
        //      Initial upper bound for the DFBB search
        //      If this value is set to zero, DFBB search will automatically determine an
        //          appropriate upper bound using an upper bound heuristic. If it is non-zero,
        //          it will use the value as an initial bound.
        int m_initialUpperBound;
            
        //      Initial GameState used for the search. See GameState.h for details
        GameState m_initialState;
        BuildOrderAbilities m_openingBuildOrder;

        GameState m_enemyInitialState;
        BuildOrderAbilities m_enemyBuildOrder;
        std::vector<int>    m_enemyUnits;
        RaceID              m_enemyRace;

        ActionSetAbilities m_relevantActions;
        int m_frameTimeLimit;
        bool m_printNewBest;
        bool m_sortActions;
        bool m_saveStates;
        FracType m_explorationValue;
        bool m_changingRoot;
        bool m_changingRootReset;
        bool m_useMaxValue;
        bool m_maximizeValue;
        int m_numberOfSimulations;
        uint8 m_numberOfNodes;
        int m_simulationsPerStep;
        FracType m_simulationsPerStepDecay;
        bool m_usePolicyNetwork;
        int m_threadsForExperiment;
        int m_numPlayouts;
        int m_level;
        bool m_useTotalTimeLimit;
        FracType m_SDConstant;
        int m_temperatureChange;

    public:

        // alternate constructor
        CombatSearchParameters();
    
        void setRepetitions(ActionType a, int repetitions);
        int getRepetitions(ActionType a) const;

        void setMaxActions(ActionType a, int max);
        int getMaxActions(ActionType a) const;

        void setRelevantActions(const ActionSetAbilities & set);
        const ActionSetAbilities & getRelevantActions() const;

        void setInitialState(const GameState & s);
        const GameState & getInitialState() const;

        void setEnemyInitialState(const GameState & s);
        const GameState & getEnemyInitialState() const;

        void setOpeningBuildOrder(const BuildOrderAbilities & buildOrder);
        const BuildOrderAbilities & getOpeningBuildOrder() const;

        void setEnemyBuildOrder(const BuildOrderAbilities & buildOrder);
        const BuildOrderAbilities & getEnemyBuildOrder() const;

        void setEnemyUnits(const std::vector<int> & units);
        const std::vector<int> & getEnemyUnits() const;

        void setEnemyRace(RaceID race);
        RaceID getEnemyRace() const;

        void setSearchTimeLimit(float timeLimitMS);
        float getSearchTimeLimit() const;

        void setFrameTimeLimit(int limit);
        int getFrameTimeLimit() const;

        void setAlwaysMakeWorkers(bool flag);
        bool getAlwaysMakeWorkers() const;

        void setPrintNewBest(bool flag);
        bool getPrintNewBest() const;

        void setSortActions(bool flag);
        bool getSortActions() const;

        void setSaveStates(bool flag);
        bool getSaveStates() const;

        void setExplorationValue(FracType value);
        FracType getExplorationValue() const;

        void setChangingRoot(bool value);
        bool getChangingRoot() const;

        void setChangingRootReset(bool value);
        bool getChangingRootReset() const;

        void setUseMaxValue(bool value);
        bool getUseMaxValue() const;

        void setMaximizeValue(bool value);
        bool getMaximizeValue() const;

        void setNumberOfSimulations(int value);
        int getNumberOfSimulations() const;

        void setNumberOfNodes(uint8 value);
        uint8 getNumberOfNodes() const;

        void setSimulationsPerStep(int value);
        int getSimulationsPerStep() const;

        void setSimulationsPerStepDecay(FracType value);
        FracType getSimulationsPerStepDecay() const;

        void setUsePolicyNetwork(bool value);
        bool usePolicyNetwork() const;

        void setThreadsForExperiment(int threads);
        int getThreadsForExperiment() const;

        void setNumPlayouts(int value);
        int getNumPlayouts() const;

        void setLevel(int value);
        int getLevel() const;

        void setUseTotalTimeLimit(bool value);
        bool getUseTotalTimeLimit() const;

        void setSDConstant(FracType value);
        FracType getSDConstant() const;

        void setTemperatureChange(int value);
        int getTemperatureChange() const;
    
        void print();
    };
}
