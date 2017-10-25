#ifndef DATA_DISTRIBUTION_DEEP_WALKERS_H
#define DATA_DISTRIBUTION_DEEP_WALKERS_H

#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/DataDistributionAnalizer.h"

namespace OPS
{
	namespace Transforms
	{
		namespace DataDistribution
		{
			class SingleAccessAreaInfoFinder: public OPS::Reprise::Service::DeepWalker
			{
			public:
				DataDistributionAnalizer::SingleAccessAreaInfoContainer getSingleAccessAreaInfoContainer()
				{
					return m_singleAccessAreaInfoContainer;
				}

				void visit(OPS::Reprise::BlockStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::ForStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::WhileStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::IfStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::PlainSwitchStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::GotoStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::ReturnStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

                void visit(OPS::Reprise::Canto::HirBreakStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

                void visit(OPS::Reprise::Canto::HirContinueStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::ExpressionStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::EmptyStatement& statement)
				{
					checkAndAddIntoSingleAccessAreaContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

			private:
				void checkAndAddIntoSingleAccessAreaContainer(StatementBase& statement)
				{
					if(statement.hasNote("single_access"))
					{
                        int shift = atoi(statement.getNote("single_access").getString().c_str());
						SingleAccessAreaInfo info(statement, shift);
						m_singleAccessAreaInfoContainer.push_back(info);
					}
				}

			private:
				DataDistributionAnalizer::SingleAccessAreaInfoContainer m_singleAccessAreaInfoContainer;
			};

			class DistributedDataInfoFinder: public OPS::Reprise::Service::DeepWalker
			{
			public:
				DataDistributionAnalizer::DistributedDataInfoContainer getDistributedDataInfoContainer()
				{
					return m_distributedDataInfoContainer;
				}

				void visit(ReferenceExpression& referenceExpression)
				{
					if(referenceExpression.getReference().hasNote("distribute"))
					{
						bool declarationAlreadyInContainer = false;
						for (DataDistributionAnalizer::DistributedDataInfoContainer::iterator it = m_distributedDataInfoContainer.begin(); it != m_distributedDataInfoContainer.end(); ++it)
						{
							if (&(referenceExpression.getReference()) == &(it->getDeclaration()))
							{
								declarationAlreadyInContainer = true;
								break;
							}
						}

						if (!declarationAlreadyInContainer)
						{
                            int shift = atoi(referenceExpression.getReference().getNote("distribute").getString().c_str());
							DistributedDataInfo info(referenceExpression.getReference(), shift);
							m_distributedDataInfoContainer.push_back(info);
						}
					}
				}

			private:
				DataDistributionAnalizer::DistributedDataInfoContainer m_distributedDataInfoContainer;

			};
			class DistributionAreaInfoFinder: public OPS::Reprise::Service::DeepWalker
			{
			public:
				explicit DistributionAreaInfoFinder(DataDistributionAnalizer::DistributedDataInfoContainer& distributedDateDeclarationsOwner,
					DataDistributionAnalizer::IgnoredDataInfoContainer& ignoredDateDeclarationsOwner)
					:m_distributedDateDeclarationsOwner(distributedDateDeclarationsOwner),
					m_ignoredDateDeclarationsOwner(ignoredDateDeclarationsOwner)
				{

				}

			public:
				DataDistributionAnalizer::DistributionAreaInfoContainer getDistributionAreaInfoContainer()
				{
					return m_distributionAreaInfoContainer;
				}

				void visit(OPS::Reprise::BlockStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::ForStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::WhileStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::IfStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::PlainSwitchStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::GotoStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::ReturnStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

                void visit(OPS::Reprise::Canto::HirBreakStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

                void visit(OPS::Reprise::Canto::HirContinueStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::ExpressionStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

				void visit(OPS::Reprise::EmptyStatement& statement)
				{
					checkAndAddIntoDistributionAreaInfoContainer(statement);
					OPS::Reprise::Service::DeepWalker::visit(statement);
				}

			private:
				void checkAndAddIntoDistributionAreaInfoContainer(StatementBase& statement)
				{
					if(statement.hasNote("distribute"))
					{
						std::string noteValues = statement.getNote("distribute").getString() + ';';

						int semiIndex = noteValues.find(';');
						while(semiIndex != -1)
						{
							std::string noteValue = noteValues.substr(0, semiIndex);
                            int openParenthesisIndex = noteValue.find_first_of('(');
                            bool isBlock;
                            if(noteValue.substr(0, openParenthesisIndex) == "data")
                            {
                                isBlock = false;
                            }
                            else if(noteValue.substr(0, openParenthesisIndex) == "block")
                            {
                                isBlock = true;
                            }
                            else
                            {
                                return;
                            }
                            noteValue = noteValue.substr(openParenthesisIndex + 1, noteValue.find_last_of(')'));

							if(noteValue.length() == 0)
								return;

							int commaIndex = noteValue.find(',');
							if(commaIndex == -1)
								return;

							std::string declarationName = noteValue.substr(0, commaIndex);
							std::string parametersString = noteValue.substr(commaIndex + 1);

							if(declarationName.length() == 0 || parametersString.length() == 0)
								return;

							VariableDeclaration* pDeclaration = NULL;
							VariableDeclaration* pPartDeclaration = NULL;
							for (DataDistributionAnalizer::DistributedDataInfoContainer::iterator it = m_distributedDateDeclarationsOwner.begin(); it != m_distributedDateDeclarationsOwner.end(); ++it)
							{
								if (it->getDeclaration().getName() == declarationName)
								{
									pDeclaration = &(it->getDeclaration());
									break;
								}
							}
							if(pDeclaration == NULL)
								return;

							commaIndex = parametersString.find(',');
							if(commaIndex == -1)
								return;

							std::string scatterRequiredStr = parametersString.substr(0, commaIndex);
							if(scatterRequiredStr != "0" && scatterRequiredStr != "1")
							{
								for (DataDistributionAnalizer::IgnoredDataInfoContainer::iterator it = m_ignoredDateDeclarationsOwner.begin(); it != m_ignoredDateDeclarationsOwner.end(); ++it)
								{
									if (it->getDeclaration().getName() == scatterRequiredStr)
									{
										pPartDeclaration = &(it->getDeclaration());
										break;
									}
								}
								if(pPartDeclaration == NULL)
									return;

								parametersString = parametersString.substr(commaIndex + 1);
								commaIndex = parametersString.find(',');
								if(commaIndex == -1)
									return;

								scatterRequiredStr = parametersString.substr(0, commaIndex);
							}

							int scatterRequired = atoi(scatterRequiredStr.c_str());
							parametersString = parametersString.substr(commaIndex + 1);

							commaIndex = parametersString.find(',');
							if(commaIndex == -1)
								return;

							std::string gatherRequiredStr = parametersString.substr(0, commaIndex);
							int gatherRequired = atoi(gatherRequiredStr.c_str());
							parametersString = parametersString.substr(commaIndex + 1);

							BADDParameters generatorParameters;
							if(!BADDParameters::Parse(parametersString, generatorParameters))
								return;

							BADDParameters parameters;
							parameters.d = generatorParameters.d;
							parameters.dims = generatorParameters.dims;
							parameters.s = generatorParameters.s;
							parameters.P = generatorParameters.P;
							parameters.a.resize(parameters.dims.size());

                            DistributionAreaInfo info( statement, *pDeclaration, pPartDeclaration, parameters, generatorParameters, scatterRequired == 1, gatherRequired == 1, isBlock );
							m_distributionAreaInfoContainer.push_back(info);

							noteValues = noteValues.substr(semiIndex + 1);
							semiIndex = noteValues.find(';');
						}
					}
				}

			private:
				DataDistributionAnalizer::DistributedDataInfoContainer& m_distributedDateDeclarationsOwner;
				DataDistributionAnalizer::IgnoredDataInfoContainer& m_ignoredDateDeclarationsOwner;

				DataDistributionAnalizer::DistributionAreaInfoContainer m_distributionAreaInfoContainer;
			};

			class IgnoredDataInfoFinder: public OPS::Reprise::Service::DeepWalker
			{
			public:
				DataDistributionAnalizer::IgnoredDataInfoContainer getIgnoredDataInfoContainer()
				{
					return m_ignoredDataInfoContainer;
				}

				void visit(ReferenceExpression& referenceExpression)
				{
					if(referenceExpression.getReference().hasNote("ignore"))
					{
						bool declarationAlreadyInContainer = false;
						for (DataDistributionAnalizer::IgnoredDataInfoContainer::iterator it = m_ignoredDataInfoContainer.begin(); it != m_ignoredDataInfoContainer.end(); ++it)
						{
							if (&(referenceExpression.getReference()) == &(it->getDeclaration()))
							{
								declarationAlreadyInContainer = true;
								break;
							}
						}

						if (!declarationAlreadyInContainer)
						{
							IgnoredDataInfo info(referenceExpression.getReference());
							m_ignoredDataInfoContainer.push_back(info);
						}
					}
				}

			private:
				DataDistributionAnalizer::IgnoredDataInfoContainer m_ignoredDataInfoContainer;

			};

			class ParallelLoopNestingInfoFinder: public OPS::Reprise::Service::DeepWalker
			{
			public:
				DataDistributionAnalizer::ParallelLoopNestingInfoContainer getParallelLoopNestingInfoContainer()
				{
					return m_parallelLoopNestingInfoContainer;
				}

				void visit(ForStatement& forStatement)
				{
                    std::vector<int> blockWidths;
                    std::vector<int> iterarionCounts;
					if(forStatement.hasNote("nesting"))
					{
                        ParallelLoopNestingInfo info(forStatement, false, blockWidths, iterarionCounts);
						m_parallelLoopNestingInfoContainer.push_back(info);
					}
                    else if(forStatement.hasNote("block"))
                    {
                        std::string noteValue = forStatement.getNote("block").getString();
                        noteValue = noteValue.substr(1, noteValue.length() - 2) + ',';
                        std::list<int> parameters;
                        while (1)
                        {
                            int semiIndex = noteValue.find(',');
                            if(semiIndex == -1)
                            {
                                break;
                            }

                            parameters.push_back(atoi(noteValue.substr(0, semiIndex).c_str()));
                            noteValue = noteValue.substr(semiIndex + 1);
                        }
                        blockWidths.resize(parameters.size()/2);
                        iterarionCounts.resize(parameters.size()/2);
                        std::list<int>::iterator it = parameters.begin();
						for(size_t i = 0; i < iterarionCounts.size(); ++i)
                        {
                            iterarionCounts[i] = *it;
                            it++;
                        }
						for(size_t i = 0; i < blockWidths.size(); ++i)
                        {
                            blockWidths[i] = *it;
                            it++;
                        }
                        ParallelLoopNestingInfo info(forStatement, true, blockWidths, iterarionCounts);
                        m_parallelLoopNestingInfoContainer.push_back(info);
                    }

					OPS::Reprise::Service::DeepWalker::visit(forStatement);
				}
			private:
				DataDistributionAnalizer::ParallelLoopNestingInfoContainer m_parallelLoopNestingInfoContainer;
			};
			class ReferencesFinder: public OPS::Reprise::Service::DeepWalker
			{
			public:
				typedef std::list<OPS::Reprise::ReferenceExpression*> ReferencesContainer;

			public:
				explicit ReferencesFinder(OPS::Reprise::VariableDeclaration& declaration)
					:m_pDeclaration(&declaration)
				{

				}

			public: 
				ReferencesContainer GetReferences()
				{
					return m_references;
				}

				void visit(OPS::Reprise::ReferenceExpression& expression)
				{
					if(&expression.getReference() == m_pDeclaration)
					{
						m_references.push_back(&expression);
					}
				}

			private:
				OPS::Reprise::VariableDeclaration* m_pDeclaration;

				ReferencesContainer m_references;
			};
		}
	}
}

#endif // DATA_DISTRIBUTION_DEEP_WALKERS_H
