#pragma once
#include "CombatSearch_Integral.h"
#include "Node.h"
#include <random>

namespace BOSS
{
    class CombatSearch_IntegralMCTS : public CombatSearch_Integral
    {
        FracType m_exploration_parameter;

        int m_numSimulations;
        int m_simulationsPerStep;
        
        std::mt19937 m_rnggen;
        
        int m_writeEveryKSimulations;
        std::string m_resultsSaveDir;
        std::string m_resultsFilePrefix;
        std::stringstream m_resultsStream;
        std::stringstream m_dataStream;

        CombatSearch_IntegralDataFinishedUnits m_promisingNodeIntegral;
        BuildOrderAbilities m_promisingNodeBuildOrder;

        void recurse(const GameState & state, int depth);
        
        // returns the node and isNodeJustCreated 
        std::pair<std::shared_ptr<Node>, bool> getPromisingNode(std::shared_ptr<Node> node);
        
        bool isTerminalNode(const Node & node) const;
        
        void randomPlayout(Node node);
        
        // does a random action
        void doRandomAction(Node & node, const GameState & prevGameState);
    
        // updates both m_promisingNodeIntegral and m_promisingNodeBuildOrder
        void updateBOIntegral(const Node & node, const ActionAbilityPair & action, const GameState & prevGameState, bool permanantUpdate);

        // updates values of the nodes explored in this iteration
        void backPropogation(std::shared_ptr<Node> node);

        // sets the class variables to the best build order found during search
        void pickBestBuildOrder(std::shared_ptr<Node> root, bool useVisitCount);

        void test(const GameState & state);
        void test2(const GameState & state);

        void writeResultsToFile(std::shared_ptr<Node> root, int simulationsWritten);

        virtual void printResults();

        virtual void writeResultsFile(const std::string & dir, const std::string & filename);

    public:
        CombatSearch_IntegralMCTS(const CombatSearchParameters p = CombatSearchParameters(), const std::string & dir = "", 
                                                                    const std::string & prefix = "", const std::string & name = "");

        ~CombatSearch_IntegralMCTS();

        int getNumSimulations() const { return m_numSimulations; }
    };
}

