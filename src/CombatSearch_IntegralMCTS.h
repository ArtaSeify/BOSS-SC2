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
        int m_writeEveryKSimulations;
        std::mt19937 m_rnggen;

        std::string m_save_dir;
        std::string m_file_prefix;

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
        void updateBOIntegral(const Node & node, const Action & action, const GameState & prevGameState);

        // updates values of the nodes explored in this iteration
        void backPropogation(std::shared_ptr<Node> node);

        // sets the class variables to the best build order found during search
        void pickBestBuildOrder(std::shared_ptr<Node> root, bool useVisitCount);

        void test(const GameState & state);

        void writeResultsToFile(std::shared_ptr<Node> root);

        virtual void printResults();

    public:
        CombatSearch_IntegralMCTS(const CombatSearchParameters p = CombatSearchParameters(), const std::string & dir = "", const std::string & prefix = "");

    };
}

