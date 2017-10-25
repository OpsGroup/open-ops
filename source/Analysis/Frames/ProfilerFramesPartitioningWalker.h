#ifndef PROFILER_FRAMES_PARTITIONING_WALKER_H_INCLUDED_
#define PROFILER_FRAMES_PARTITIONING_WALKER_H_INCLUDED_

#include "Analysis/Frames/FramesGraph.h"
#include "Analysis/Frames/FrameDataSpecification.h"
#include "Analysis/Frames/IProfiler.h"
#include "Analysis/Frames/BlockNestAnalizer.h"
#include "Reprise/Service/DeepWalker.h"
#include "OPS_Core/MemoryHelper.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/ParametricLinearExpressions.h"

namespace OPS
{
	namespace Analysis
	{
		namespace Frames
		{
            using namespace OPS::Reprise;
            using namespace Shared;
            using namespace Shared::ExpressionHelpers;

			class ProfilerFramesPartitioningWalker: public OPS::Reprise::Service::DeepWalker
			{
			public:
				ProfilerFramesPartitioningWalker(std::tr1::shared_ptr<IProfiler> spProfiler, long_long_t maxCommandSize, long_long_t maxDataSize)
					:m_spProfiler(spProfiler), m_maxCommandSize(maxCommandSize), m_maxDataSize(maxDataSize), m_pFirstFrame(NULL)
				{	
					resetCurrents();
				}

				FramesGraph getResult()
				{
					saveFrame(createFrame());

					if(m_pFirstFrame != NULL)
					{
						m_framesGraph.setFirstExecutableNode(m_pFirstFrame);
					}
					m_framesGraph.setLastExecutableNodes(m_previousFrames);

					return m_framesGraph;
				}

				void visit(ExpressionStatement& expressionStatement)
				{
					if (!tryPushStatement(expressionStatement))
					{
						throw OPS::Exception("partitioningBlock: impermissible statement assign");
					}
				}

				void visit(OPS::Reprise::ForStatement& forStatement)
				{
					if (tryPushStatement(forStatement))
					{
						return;
					}
					saveFrame(createFrame());

					// Try to split forStatement into several frames by iterations
					if (tryToSplitForStatementByIterations(forStatement))
					{
						return;
					}

                    // Calculate resource usage for headers
                    long_long_t commandSize = m_spProfiler->GetSizeCommand(forStatement, true);
                    long_long_t dataSize = m_spProfiler->GetSizeData(forStatement, true);

					if(commandSize > m_maxCommandSize || dataSize > m_maxDataSize)
					{
						throw OPS::Exception("partitioningBlock: impermissible statement assign");
					}

                    Frame* pForHeaderFrame = createHeaderFrame(
						&forStatement, 
						calculateResourceUsage(commandSize, dataSize)
					);
                    saveFrame(pForHeaderFrame);

					ProfilerFramesPartitioningWalker walkerBody(m_spProfiler, m_maxCommandSize, m_maxDataSize);
					forStatement.getBody().accept(walkerBody);
					FramesGraph subGraph = walkerBody.getResult();
					FramesGraph::FramesCollection bodyLastExecutableNodes = subGraph.getLastExecutableNodes();

					insertSubGraph(subGraph);

					for (std::set<Frame*>::iterator it = bodyLastExecutableNodes.begin(); it != bodyLastExecutableNodes.end(); ++it)
					{
                        m_framesGraph.addEdge(*it, pForHeaderFrame);
					}
				}

				void visit(OPS::Reprise::WhileStatement& whileStatement)
				{
					if (tryPushStatement(whileStatement))
					{
						return;
					}
					saveFrame(createFrame());

                    // Calculate resource usage for headers
                    long_long_t commandSize = m_spProfiler->GetSizeCommand(whileStatement, true);
                    long_long_t dataSize = m_spProfiler->GetSizeData(whileStatement, true);

					if(commandSize > m_maxCommandSize || dataSize > m_maxDataSize)
					{
						throw OPS::Exception("partitioningBlock: impermissible statement assign");
					}

					ProfilerFramesPartitioningWalker walkerBody(m_spProfiler, m_maxCommandSize, m_maxDataSize);
					whileStatement.getBody().accept(walkerBody);
					FramesGraph subGraph = walkerBody.getResult();
					FramesGraph::FramesCollection bodyLastExecutableNodes = subGraph.getLastExecutableNodes();

					Frame* pWhileHeaderFrame = createHeaderFrame(
						&whileStatement,
						calculateResourceUsage(commandSize, dataSize)
					);

					if (whileStatement.isPreCondition())
					{
						saveFrame(pWhileHeaderFrame); // Attach frame with header to graph
						
						insertSubGraph(subGraph);   // Insert body subgraph. Edges from header to body added in insertSubGraph procedure

						// Insert edges from body to header
						for (FramesGraph::FramesCollection::iterator it = bodyLastExecutableNodes.begin(); it != bodyLastExecutableNodes.end(); ++it)
						{
							m_framesGraph.addEdge(*it, pWhileHeaderFrame);
						}
					}
					else
					{
						insertSubGraph(subGraph);   // Insert body subgraph. Edges from previous nodes to body added in insertSubGraph procedure

						if (bodyLastExecutableNodes.size() > 0)
						{
							// Force setting m_previousFrames to guarantee that next calling of
							// saveAndResetCurrentFrame will lead to adding edges from body to header only
							m_previousFrames = subGraph.getLastExecutableNodes(); 
						}
						if (subGraph.getFirstExecutableNode() != NULL && m_pFirstFrame == NULL)
						{
							m_pFirstFrame = subGraph.getFirstExecutableNode();
						}
						
						saveFrame(pWhileHeaderFrame); // Attach frame with header to graph

						// Insert edges from header to body
						Frame* pSubgraphFirstExecutableNode = subGraph.getFirstExecutableNode();
						if (pSubgraphFirstExecutableNode != NULL)
						{
							m_framesGraph.addEdge(pWhileHeaderFrame, pSubgraphFirstExecutableNode);
						}
					}
				}

				void visit(OPS::Reprise::IfStatement& ifStatement)
				{
					if(tryPushStatement(ifStatement))
					{
						return;
					}
					
					saveFrame(createFrame());

                    // Calculate resource usage for headers
                    long_long_t commandSize = m_spProfiler->GetSizeCommand(ifStatement, true);
                    long_long_t dataSize = m_spProfiler->GetSizeData(ifStatement, true);

					if(commandSize > m_maxCommandSize || dataSize > m_maxDataSize)
					{
						throw OPS::Exception("partitioningBlock: impermissible statement assign");
					}
						
					saveFrame(createHeaderFrame(
						&ifStatement,
						calculateResourceUsage(commandSize, dataSize)
					));

					ProfilerFramesPartitioningWalker walkerThen(m_spProfiler, m_maxCommandSize, m_maxDataSize);
					ifStatement.getThenBody().accept(walkerThen);
					insertSubGraph(walkerThen.getResult());
					std::set<Frame*> sinksThen = walkerThen.m_previousFrames;

					ProfilerFramesPartitioningWalker walkerElse(m_spProfiler, m_maxCommandSize, m_maxDataSize);
					ifStatement.getElseBody().accept(walkerElse);
					insertSubGraph(walkerElse.getResult());
					std::set<Frame*> sinksElse = walkerElse.m_previousFrames;

					if (!sinksThen.empty() || !sinksElse.empty())
					{
						// Если нет стоков - это значит, что соответствующая ветка пуста.
						// След-но управление оператору, следующему за условным оператором,
						// может передаваться после заголовка.
						// В противном случае не может.
						m_previousFrames.clear();
					}
					m_previousFrames.insert(sinksThen.begin(), sinksThen.end());
					m_previousFrames.insert(sinksElse.begin(), sinksElse.end());
				}

			private:
				void insertSubGraph(FramesGraph subGraph)
				{
					// Добаляем все вершины подграфа
					FramesGraph::FramesCollection newNodes = subGraph.getNodes();
					for (FramesGraph::FramesCollection::iterator it = newNodes.begin(); it != newNodes.end(); ++it )
					{
						m_framesGraph.addNode(*it);
					}
					// Добавляем дуги подграфа
					FramesGraph::AccesssibilityMap accessibilityMap = subGraph.getAccesssibilityMap();
					for (FramesGraph::AccesssibilityMap::iterator accMapIt = accessibilityMap.begin(); accMapIt != accessibilityMap.end(); ++accMapIt )
					{
						FramesGraph::FramesCollection accessibleNodes = accMapIt->second;
						for (FramesGraph::FramesCollection::iterator it = accessibleNodes.begin(); it != accessibleNodes.end(); ++it)
						{
							m_framesGraph.addEdge(accMapIt->first, *it);
						}
					}
					// Добавляем дуги, ведущие в подграф.
					if (subGraph.getFirstExecutableNode() != NULL)
					{
						for (std::set<Frame*>::iterator it = m_previousFrames.begin(); it != m_previousFrames.end(); ++it)
						{
							m_framesGraph.addEdge(*it, subGraph.getFirstExecutableNode());
						}
					}
				}
				bool tryPushStatement(StatementBase& statement)
				{
                    long_long_t commandSize = m_spProfiler->GetSizeCommand(statement);
                    long_long_t dataSize = m_spProfiler->GetSizeData(statement);

					if(m_currentFrameCommandSize + commandSize <= m_maxCommandSize && m_currentFrameDataSize + dataSize <= m_maxDataSize)
					{
						m_currentNodes.push_back(&statement);
						m_currentFrameCommandSize += commandSize;
						m_currentFrameDataSize += dataSize;
						m_currentResourceUsage = calculateResourceUsage(m_currentFrameCommandSize, m_currentFrameDataSize);
						return true;
					}
					if(commandSize <= m_maxCommandSize && dataSize <= m_maxDataSize)
					{
						saveFrame(createFrame());

						m_currentNodes.push_back(&statement);
						m_currentFrameCommandSize = commandSize;
						m_currentFrameDataSize = dataSize;
						m_currentResourceUsage = calculateResourceUsage(m_currentFrameCommandSize, m_currentFrameDataSize);
						return true;
					}
					return false;
				}


				void resetCurrents()
				{
					m_currentFrameCommandSize = 0;
					m_currentFrameDataSize = 0;
					m_currentNodes.clear();
					m_currentResourceUsage = 0;
				}

				double calculateResourceUsage(long_long_t commandSize, long_long_t dataSize)
				{
					double commandUsage = m_maxCommandSize > 0 && commandSize > 0 ? 1.0 * commandSize / m_maxCommandSize : 0;
					double dataUsage = m_maxDataSize > 0 && dataSize > 0 ? 1.0 * dataSize / m_maxDataSize : 0;

					return commandUsage > dataUsage ? commandUsage : dataUsage;
				}

                bool findIterationsCount(ForStatement& forStatement, int& iterationsCount)
                {
                    if(!Editing::forIsBasic(forStatement))
                    {
                        return false;
                    }

                    ReprisePtr<ExpressionBase> rpIterationsCount(&(
                            op(Editing::getBasicForFinalExpression(forStatement)) - op(Editing::getBasicForInitExpression(forStatement))
                        ));

                    ReprisePtr<ExpressionBase> rpSimplifiedIterationsCount(ParametricLinearExpression::simplify(rpIterationsCount.get()));

                    if (rpSimplifiedIterationsCount->is_a<StrictLiteralExpression>())
                    {
                        iterationsCount = rpSimplifiedIterationsCount->cast_to<StrictLiteralExpression>().getInt32();
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                bool tryToSplitForStatementByIterations(ForStatement& forStatement, const std::vector<ForStatement*>& nest, const std::vector<int>& iterationsCounts, double& resourceUsage)
                {
                    OPS_ASSERT(nest.size() == iterationsCounts.size());

                    std::map<const ForStatement*, int> loopsAndIterationsCounts;
					for(size_t loopIndex = 0; loopIndex < nest.size(); ++loopIndex)
                    {
                        loopsAndIterationsCounts[nest[loopIndex]] = iterationsCounts[loopIndex];
                    }

                    long_long_t commandSize = m_spProfiler->GetSizeCommand(forStatement);
                    long_long_t dataSize = m_spProfiler->GetSizeData(forStatement, loopsAndIterationsCounts);

                    if(commandSize > m_maxCommandSize || dataSize > m_maxDataSize)
                    {
                        return false;
                    }

                    resourceUsage = calculateResourceUsage(commandSize, dataSize);

                    return true;
                }

                std::vector<ForStatement*> getMaxDenseLoopNest(ForStatement& forStatement)
                {
                    std::list<ForStatement*> nestList;
                    ForStatement* pCurrentForStatement = &forStatement;
                    while(pCurrentForStatement != NULL)
                    {
                        int iterationsCount;
                        if(!findIterationsCount(*pCurrentForStatement, iterationsCount))
                        {
                            break;
                        }

                        nestList.push_back(pCurrentForStatement);
                        if(pCurrentForStatement->getBody().getChildCount() != 1)
                        {
                            break;
                        }
                        pCurrentForStatement = pCurrentForStatement->getBody().getChild(0).cast_ptr<ForStatement>();
                    }

                    std::vector<ForStatement*> result(nestList.begin(), nestList.end());
                    return result;
                }

                std::vector<int> getIterationsCounts(const std::vector<ForStatement*>& nest)
                {
                    std::vector<int> result(nest.size());
					for(size_t loopIndex = 0; loopIndex < nest.size(); ++loopIndex)
                    {
                        int iterationsCount;
                        findIterationsCount(*nest[loopIndex], iterationsCount);
                        result[loopIndex] = iterationsCount;
                    }
                    return result;
                }

                std::vector<ForStatement*> findNestToBlock(const std::vector<ForStatement*>& maxNest)
                {
                    // For now try to find outer sub nest
                    // todo: refactor this method to use external sub nest extracting tool
                    std::list<ForStatement*> listNest;
                    listNest.push_back(maxNest[0]);
					for(size_t maxNestLoopIndex = 1; maxNestLoopIndex < maxNest.size(); ++maxNestLoopIndex)
                    {
                        BlockNestAnalizer bna(*maxNest[0], maxNestLoopIndex + 1);
                        if(bna.analyse())
                        {
                            listNest.push_back(maxNest[maxNestLoopIndex]);
                        }
                    }

                    return std::vector<ForStatement*>(listNest.begin(), listNest.end());
                }

                bool refineIterationsCounts(const std::vector<int> minIterationsCounts, const std::vector<int> maxIterationsCounts, std::vector<int>& refinedIterationsCounts)
                {
                    // For now use algorithm below
                    // todo: refactor this method to use external tool

                    OPS_ASSERT(minIterationsCounts.size() == maxIterationsCounts.size());

                    refinedIterationsCounts.resize(minIterationsCounts.size());
                    bool isRefined = false;
					for(size_t loopIndex = 0; loopIndex < maxIterationsCounts.size(); ++loopIndex)
                    {
                        if(maxIterationsCounts[loopIndex] - minIterationsCounts[loopIndex] > 1)
                        {
                            refinedIterationsCounts[loopIndex] = minIterationsCounts[loopIndex] + (maxIterationsCounts[loopIndex] - minIterationsCounts[loopIndex]) / 2;
                            isRefined = true;
                        }
                    }
                    return isRefined;
                }

				bool tryToSplitForStatementByIterations(ForStatement& forStatement)
				{
                    // 1. Find max loop nest that could be blocked
                    std::vector<ForStatement*> maxNest = getMaxDenseLoopNest(forStatement);

                    // 2. Determine which loops should be blocked
                    std::vector<ForStatement*> nest = findNestToBlock(maxNest);

                    // 3. Calculate block widths
                    std::vector<int> maxIterationsCounts = getIterationsCounts(nest);
                    std::vector<int> minIterationsCounts(nest.size(), 0);

                    std::vector<int> refinedIterationsCounts;
                    std::vector<int> foundIterationsCounts;
                    double foundResourceUsage;
                    bool isFound = false;

                    while(refineIterationsCounts(minIterationsCounts, maxIterationsCounts, refinedIterationsCounts))
                    {
                        double resourceUsage;
                        if(tryToSplitForStatementByIterations(forStatement, nest, refinedIterationsCounts, resourceUsage))
                        {
                            isFound = true;
                            foundIterationsCounts = refinedIterationsCounts;
                            minIterationsCounts = refinedIterationsCounts;
                            foundResourceUsage = resourceUsage;
                        }
                        else
                        {
                            maxIterationsCounts = refinedIterationsCounts;
                        }
                    }

                    if(isFound)
                    {
                        Frame* pFrameSplittedByIterations = createPatitionedByIterationFrame(
                            &forStatement,
                            foundResourceUsage,
                            nest,
                            foundIterationsCounts
                        );
                        saveFrame(pFrameSplittedByIterations);

                        m_framesGraph.addEdge(pFrameSplittedByIterations, pFrameSplittedByIterations);
                    }

                    return isFound;
				}

				void saveFrame(Frame* pFrame)
				{
					OPS_ASSERT(pFrame != NULL);

					if(pFrame->m_Nodes.size() == 0)
					{
						return;
					}

					if(m_pFirstFrame == NULL)
					{
						m_pFirstFrame = pFrame;
					}

					m_framesGraph.addNode(pFrame);
					for(std::set<Frame*>::iterator it = m_previousFrames.begin(); it != m_previousFrames.end(); ++it)
					{
						m_framesGraph.addEdge(*it, pFrame);
					}

					m_previousFrames.clear();
					m_previousFrames.insert(pFrame);

					resetCurrents();
				}

				HeaderFrame* createHeaderFrame(StatementBase* pHeaderStatement, double resourceUsage)
				{
					HeaderFrame* pResult= new HeaderFrame();
					pResult->m_Nodes.push_back(pHeaderStatement);
					pResult->m_resourceUsage = resourceUsage;

					return pResult;
				}

				Frame* createFrame()
				{
					Frame* pResult= new Frame();
					pResult->m_Nodes = m_currentNodes;
					pResult->m_resourceUsage = m_currentResourceUsage;

					return pResult;
				}

                PartitionedByIterationsFrame* createPatitionedByIterationFrame(ForStatement* pForStatement, double resourceUsage, const std::vector<ForStatement*>& nest, const std::vector<int>& iterationsCount)
				{
                    OPS_ASSERT(nest.size() == iterationsCount.size());

					PartitionedByIterationsFrame* pResult= new PartitionedByIterationsFrame();
                    pResult->m_Nodes.clear();
                    pResult->m_Nodes.push_back(pForStatement);
                    pResult->m_resourceUsage = resourceUsage;
					for(size_t loopIndex = 0; loopIndex < nest.size(); ++loopIndex)
                    {
                        pResult->m_partitionedNestIterationsCount[nest[loopIndex]] = iterationsCount[loopIndex];
                    }

					return pResult;
				}

			private:
				std::tr1::shared_ptr<IProfiler> m_spProfiler;
				long_long_t m_maxCommandSize;
				long_long_t m_maxDataSize;

				Frame::TNodes m_currentNodes;
				double m_currentResourceUsage;
				long_long_t m_currentFrameCommandSize;
				long_long_t m_currentFrameDataSize;
				FramesGraph m_framesGraph;
				std::set<Frame*> m_previousFrames;
				Frame* m_pFirstFrame;
			};
		}
	}
}

#endif // PROFILER_FRAMES_PARTITIONING_WALKER_H_INCLUDED_
