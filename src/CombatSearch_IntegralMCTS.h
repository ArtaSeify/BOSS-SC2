#pragma once
#include "Common.h"
#include "CombatSearch_Integral.h"
#include "Node.h"
#include <random>

namespace BOSS
{
    class CombatSearch_IntegralMCTS : public CombatSearch_Integral
    {
        FracType m_exploration_parameter;

        int m_writeEveryKSimulations;

        std::mt19937 m_rnggen;
     
    protected:
        int m_numTotalSimulations;
        int m_numCurrentRootSimulations;
        int m_simulationsPerStep;

        std::map<std::string, std::vector<FracType>> m_rootRewards;

        CombatSearch_IntegralDataFinishedUnits  m_bestIntegralFound;
        BuildOrderAbilities                     m_bestBuildOrderFound;
        bool                                    m_needToWriteBestValue;

        std::stringstream m_resultsStream;

        CombatSearch_IntegralDataFinishedUnits m_promisingNodeIntegral;
        BuildOrderAbilities m_promisingNodeBuildOrder;
    
        void recurse(const GameState & state, int depth);
        
        // returns the node and isNodeJustCreated 
        std::pair<std::shared_ptr<Node>, bool> getPromisingNode(std::shared_ptr<Node> node);
        
        bool isTerminalNode(const Node & node) const;
        
        void randomPlayout(Node node);
        
        // does a random action
        void doRandomAction(Node & node, const GameState & prevGameState);

        //void getChronoBoostTargets(const Node & node, ActionSetAbilities & legalActions);

        void updateIntegralTerminal(const Node & node, const GameState & prevGameState);
    
        // updates both m_promisingNodeIntegral and m_promisingNodeBuildOrder
        void updateBOIntegral(const Node & node, const ActionAbilityPair & action, const GameState & prevGameState, bool permanantUpdate);

        // updates values of the nodes explored in this iteration
        void backPropogation(std::shared_ptr<Node> node);

        // sets the class variables to the best build order found during search
        std::pair<BuildOrderAbilities, CombatSearch_IntegralDataFinishedUnits> pickBestBuildOrder(std::shared_ptr<Node> root, bool useVisitCount);

        void test(const GameState & state);
        void test2(const GameState & state);

        bool timeLimitReached();

        void updateNodeVisits(bool nodeExpanded, bool isTerminal);

        void writeResultsToFile(std::shared_ptr<Node> root);

        virtual void printResults();

        virtual void writeResultsFile(const std::string & dir, const std::string & filename);

    public:
        CombatSearch_IntegralMCTS(const CombatSearchParameters p = CombatSearchParameters(), const std::string & dir = "", 
                                                                    const std::string & prefix = "", const std::string & name = "");
        ~CombatSearch_IntegralMCTS();
        int getNumSimulations() const { return m_numTotalSimulations; }
    };
}

