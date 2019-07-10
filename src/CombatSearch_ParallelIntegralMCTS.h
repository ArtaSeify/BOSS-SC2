#pragma once
#include "Common.h"
#include "CombatSearch_Integral.h"
#include "Node.h"
#include "GPUQueue.h"

#include "gsl/gsl_rng.h"
#include "gsl/gsl_randist.h"
#include <random>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace BOSS
{
    class CombatSearch_ParallelIntegralMCTS : public CombatSearch_Integral
    {     
    protected:
        struct BuildOrderIntegral
        {
            CombatSearch_IntegralDataFinishedUnits integral;
            BuildOrderAbilities buildOrder;

            BuildOrderIntegral() : integral(), buildOrder() { }

            BuildOrderIntegral(const CombatSearch_IntegralDataFinishedUnits & newIntegral, const BuildOrderAbilities & newBuildOrder)
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
            std::string state;
            std::string policy;
            FracType stateValue;

            StateData() : state(), policy(), stateValue() { }
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

        std::mt19937 m_rnggen;
        gsl_rng* m_gsl_r;

        std::atomic<uint8>  m_nodesExpanded;   // number of nodes expanded in the search
        std::atomic<uint8>  m_nodeVisits;      // number of nodes visited. Duplicates are counted
        std::atomic<uint8>  m_leafNodesExpanded; // number of leaf nodes expanded in the search
        std::atomic<uint8>  m_leafNodesVisited;  // number of leaf nodes visited. Duplicates are counted
        
    
        void recurse(const GameState & state, int depth);
        void MCTSSearch(int threadID);

        void evaluatePolicyNetwork();
        
        std::shared_ptr<Node> getPromisingNode(Node & node, BuildOrderIntegral& buildOrderIntegral);
        
        void randomPlayout(Node node, BuildOrderIntegral& buildOrderIntegral);
        
        // does a random action
        bool doRandomAction(Node & currentNode, BuildOrderIntegral & buildOrderIntegral);

        //void getChronoBoostTargets(const Node & node, ActionSetAbilities & legalActions);

        void updateIntegralTerminal(const Node & node, BuildOrderIntegral & buildOrderIntegral);
    
        // updates both m_promisingNodeIntegral and m_promisingNodeBuildOrder
        void updateBOIntegral(const Node & currNode, const ActionAbilityPair & action, BuildOrderIntegral & buildOrderIntegral);

        // updates values of the nodes explored in this iteration
        void backPropogation(Node & node, const BuildOrderIntegral& buildOrderIntegral);

        // in a multithreaded setting, we do more simulations than the limit. We need to fix the edge
        // statistics by reducing the visit count for the extra simulations
        void fixEdgeVisits(Node & node);

        // sets the class variables to the best build order found during search
        //std::pair<BuildOrderAbilities, CombatSearch_IntegralDataFinishedUnits> pickBestBuildOrder(std::shared_ptr<Node> root, bool useVisitCount);

        bool timeLimitReached();

        void updateNodeVisits(bool nodeExpanded, bool isTerminal);

        void writeSummaryToQueue();

        void writeRootData();

        virtual void printResults();

        virtual void writeResultsFile(const std::string & dir, const std::string & filename);

    public:
        CombatSearch_ParallelIntegralMCTS(const CombatSearchParameters p = CombatSearchParameters(), const std::string & dir = "",
                                                                    const std::string & prefix = "", const std::string & name = "");
        ~CombatSearch_ParallelIntegralMCTS();
        int getNumSimulations() const { return m_numTotalSimulations; }
    };
}

