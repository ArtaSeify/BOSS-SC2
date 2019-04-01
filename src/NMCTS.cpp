#include "NMCTS.h"

using namespace BOSS;

NMCTS::NMCTS(const CombatSearchParameters p, const std::string & dir,
    const std::string & prefix, const std::string & name)
{
    m_params = p;

    m_dir = dir;
    m_prefix = prefix;
    m_name = name;

    Edge::USE_MAX_VALUE = m_params.getUseMaxValue();
}

void NMCTS::recurse(const GameState & state, int depth)
{
    std::shared_ptr<Node> root = std::make_shared<Node>(state);

    auto searchResult = executeSearch(root, m_params.getLevel());

    BuildOrderAbilities finishedUnitsBuildOrder = createFinishedUnitsBuildOrder(searchResult.second);

    m_bestIntegralFound = searchResult.first;
    m_bestBuildOrderFound = searchResult.second;
    m_results.highestEval = searchResult.first.getCurrentStackValue();
    m_results.buildOrder = searchResult.second;
    m_results.finishedUnitsBuildOrder = finishedUnitsBuildOrder;
}

NMCTS::SearchResult NMCTS::executeSearch(std::shared_ptr<Node> node, int level)
{
    CombatSearch_IntegralDataFinishedUnits bestResult;
    BuildOrderAbilities bestSolution;

    for (int iteration = 0; iteration < iterationsForLevel(level); ++iteration)
    {
        auto nodePair = getPromisingNode(node);
        std::shared_ptr<Node> promisingNode = nodePair.first;

        // a node that isn't part of the graph yet. We just simulate from this point
        if (nodePair.second)
        {
            randomPlayout(*promisingNode);
            backPropogation(promisingNode, m_promisingNodeIntegral.getCurrentStackValue());

            if (m_promisingNodeIntegral.getCurrentStackValue() > bestResult.getCurrentStackValue())
            {
                bestResult = m_promisingNodeIntegral;
                bestSolution = m_bestBuildOrderFound;
            }
        }

        // the node is part of the graph, so we create its edges and pick one at random
        else
        {
            // the node is part of the graph, so we create its edges and pick one at random
            if (!isTerminalNode(*promisingNode))
            {
                ActionSetAbilities legalActions;
                generateLegalActions(promisingNode->getState(), legalActions, m_params);
                promisingNode->createChildrenEdges(legalActions, m_params);

                // there might be no action possible, so createChildrenEdges creates 0 edges
                if (isTerminalNode(*promisingNode))
                {
                    m_results.leafNodesExpanded++;
                    m_results.leafNodesVisited++;
                }
                else
                {
                    // pick a child at random
                    const GameState & prevNodeState = promisingNode->getState();
                    std::shared_ptr<Edge> action = promisingNode->getRandomEdge();
                    promisingNode = promisingNode->notExpandedChild(action, m_params);
                    updateBOIntegral(*promisingNode, action->getAction(), prevNodeState, false);

                    updateNodeVisits(Edge::NODE_VISITS_BEFORE_EXPAND == 1, isTerminalNode(*promisingNode));
                }

                if (level == 1)
                {
                    randomPlayout(*promisingNode);
                    backPropogation(promisingNode, m_promisingNodeIntegral.getCurrentStackValue());

                    if (m_promisingNodeIntegral.getCurrentStackValue() > bestResult.getCurrentStackValue())
                    {
                        bestResult = m_promisingNodeIntegral;
                        bestSolution = m_bestBuildOrderFound;
                    }
                }
                else
                {
                    CombatSearch_IntegralDataFinishedUnits prevIntegral = m_promisingNodeIntegral;
                    BuildOrderAbilities prevBO = m_promisingNodeBuildOrder;

                    auto searchResult = executeSearch(promisingNode, level - 1);
                    backPropogation(promisingNode, searchResult.first.getCurrentStackValue());

                    m_promisingNodeIntegral = prevIntegral;
                    m_promisingNodeBuildOrder = prevBO;

                    if (searchResult.first.getCurrentStackValue() > bestResult.getCurrentStackValue())
                    {
                        bestResult = searchResult.first;
                        bestSolution = searchResult.second;
                    }
                }
            }
            else
            {
                backPropogation(promisingNode, m_promisingNodeIntegral.getCurrentStackValue());
            }
            m_numSimulations++;
        }
    }

    return SearchResult(bestResult, bestSolution);
}

int NMCTS::iterationsForLevel(int level) const
{
    if (level == 0)
    {
        BOSS_ASSERT(false, "don't call NMCTS with level 0");
        return 10;
    }
    if (level == 1)
    {
        return m_params.getNumPlayouts();
    }
    if (level == 2)
    {
        return 10;
    }

    BOSS_ASSERT(false, "level higher than 2 is not supported");
    return 0;
}

void NMCTS::backPropogation(std::shared_ptr<Node> node, FracType value)
{
    std::shared_ptr<Node> current_node = node;
    std::shared_ptr<Edge> parent_edge = current_node->getParentEdge();

    while (parent_edge != nullptr)
    {
        parent_edge->updateEdge(value);

        current_node = parent_edge->getParent();
        parent_edge = current_node->getParentEdge();
    }
}