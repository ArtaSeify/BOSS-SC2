#pragma once
#include "Common.h"
#include "CombatSearch_Integral_DFS.h"
#include "Node.h"

#include "gsl/gsl_rng.h"
#include "gsl/gsl_randist.h"
#include <random>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace BOSS
{
    class CombatSearch_Integral_MCTS : public CombatSearch_Integral_DFS
    {     
    protected:
        struct BuildOrderIntegral
        {
            CombatSearch_IntegralData_FinishedUnits integral;
            BuildOrderAbilities buildOrder;

            BuildOrderIntegral() : integral(), buildOrder() { }

            BuildOrderIntegral(const CombatSearch_IntegralData_FinishedUnits & newIntegral, const BuildOrderAbilities & newBuildOrder)
            {
                integral = newIntegral;
                buildOrder = newBuildOrder;
            }
        };

        struct ResultLog
        {
            uint8 nodesExpanded;
            uint8 nodeVisits;
            uint8 leafNodesExpanded;
            uint8 leafNodesVisited;
            double timeElapsedMS;
            int numTotalSimulations;
            std::string buildOrder;
            FracType eval;
            FracType value;

            ResultLog(uint8 newNodesExpanded, uint8 newNodeVisits, uint8 newLeafNodesExpanded, uint8 newLeafNodesVisited,
                    double newTimeElapsedMS, int newNumTotalSimulations, std::string newBuildOrder,
                    FracType newEval, FracType newValue)
            : nodesExpanded(newNodesExpanded)
            , nodeVisits(newNodeVisits)
            , leafNodesExpanded(newLeafNodesExpanded)
            , leafNodesVisited(newLeafNodesVisited)
            , timeElapsedMS(newTimeElapsedMS)
            , numTotalSimulations(newNumTotalSimulations)
            , buildOrder(newBuildOrder)
            , eval(newEval)
            , value(newValue)
            {

            }

            void writeToSS(std::stringstream & ss)
            {
                ss << nodesExpanded << "," << nodeVisits << "," << leafNodesExpanded 
                    << "," << leafNodesVisited << "," << timeElapsedMS << "," 
                    << numTotalSimulations << "," << buildOrder << "," << eval << ","
                    << value << "\n";
            }
            
        };

        struct StateData
        {
            std::pair<std::string, int> state;
            std::string policy;
            FracType stateValue;
            FracType highestFoundValue;

            StateData() : state(), policy(), stateValue(), highestFoundValue() { }
        };

        enum class ThreadMessage
        {
            RootNotChanged, RootChanged, SearchFinished
        };

        std::shared_ptr<Node>                           m_currentRoot;
        std::atomic_int                                 m_numTotalSimulations;
        int                                             m_simulationsPerRoot;
        std::mutex                                      m_backPropMutex;

        std::atomic_int                                 m_numCurrentRootSimulations;
        std::atomic_int                                 m_threadsWaitingForChangeRoot;
        BuildOrderIntegral                              m_buildOrderIntegralChangedRoot;
        std::mutex                                      m_changeRootMutex;
        std::condition_variable                         m_changeRootCompleted;
        std::atomic<ThreadMessage>                      m_rootChanged;

        BuildOrderIntegral                              m_bestResultFound;
        std::vector<ResultLog>                          m_resultLog;
        std::mutex                                      m_resultFileMutex;
        std::vector<StateData>                          m_stateData;

        RNG m_rnggen;
        gsl_rng* m_gsl_r;

        std::atomic<uint8>  m_nodesExpanded;   // number of nodes expanded in the search
        std::atomic<uint8>  m_nodeVisits;      // number of nodes visited. Duplicates are counted
        std::atomic<uint8>  m_leafNodesExpanded; // number of leaf nodes expanded in the search
        std::atomic<uint8>  m_leafNodesVisited;  // number of leaf nodes visited. Duplicates are counted
        
    
        void run(const GameState & state, int depth);
        void MCTSSearch(int threadID);

        void evaluatePolicyNetwork();
        
        std::shared_ptr<Edge> getPromisingEdge(const std::shared_ptr<Node> & node, BuildOrderIntegral& buildOrderIntegral);
        
        void rollout(GameState & state, BuildOrderIntegral& buildOrderIntegral, bool terminal);
        
        // does a random action
        bool doRandomAction(GameState & state, BuildOrderIntegral & buildOrderIntegral);

        //void getChronoBoostTargets(const Node & node, ActionSetAbilities & legalActions);

        void updateIntegralTerminal(const GameState & state, BuildOrderIntegral & buildOrderIntegral);
    
        // updates both m_promisingNodeIntegral and m_promisingNodeBuildOrder
        void updateBOIntegral(const GameState & state, const ActionAbilityPair & action, BuildOrderIntegral & buildOrderIntegral, bool terminal);

        // updates values of the nodes explored in this iteration
        void backPropagation(const std::shared_ptr<Edge> & edge, const BuildOrderIntegral& buildOrderIntegral);

        bool changeRoot(int threadID);

        // sets the class variables to the best build order found during search
        //std::pair<BuildOrderAbilities, CombatSearch_IntegralData_FinishedUnits> pickBestBuildOrder(std::shared_ptr<Node> root, bool useVisitCount);

        bool timeLimitReached();

        void updateNodeVisits(bool nodeExpanded, bool isTerminal);

        void writeSummaryToQueue();

        void writeRootData(const BuildOrderIntegral& rootIntegral);

        virtual void printResults();

        virtual void writeResultsFile(const std::string & dir, const std::string & filename);

    public:
        CombatSearch_Integral_MCTS(const CombatSearchParameters p = CombatSearchParameters(), const std::string & dir = "",
                                                                    const std::string & prefix = "", const std::string & name = "");
        ~CombatSearch_Integral_MCTS();
        int getNumSimulations() const { return m_numTotalSimulations; }
    };
}

