#include "Transforms/DataDistribution/MPI/MPIDataDistributionIteration.h"

#include "Shared/DataShared.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/SubroutinesShared.h"
#include "OPS_Core/Localization.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using namespace OPS::Transforms::MPIProducer;
            using namespace OPS::Reprise;
            using namespace OPS::Shared::ExpressionHelpers;

            MPIDataDistributionIteration::MPIDataDistributionIteration( BADDParametersFamily distributionParameters, BADDParameters generatorParameters, VariableDeclaration& targetArrayDeclaration, ForStatement& iterationStatement, int leadingDimention ):
                m_pFactory(NULL),
                m_pHelper(NULL),
                m_pRankProducer(NULL),
                m_generatorParameters(generatorParameters),
                m_distributionParameters(distributionParameters),
                m_targetArrayDeclaration(targetArrayDeclaration),
                m_pNewArrayDeclaration(NULL),
                m_iterationStatement(iterationStatement),
                m_leadingDimention(leadingDimention),
                m_pRankDeclaration(NULL),
                m_pArrayElementBasicType(NULL),
                m_analysisRerformed(false)
            {
                m_pFactory = new MPIProducerCFactory();

                m_pHelper = m_pFactory->createMPIHelper(Shared::getTranslationUnit(&m_iterationStatement));
                OPS_ASSERT(m_pHelper != NULL);

                m_pRankProducer = m_pFactory->createMPIRankProducer(Shared::getSubroutineDeclarationByStatement(&m_iterationStatement));
                OPS_ASSERT(m_pRankProducer != NULL);

                TypeBase* pArrayElementType = Shared::getArrayElementBasicType(&m_targetArrayDeclaration.getType());
                if(pArrayElementType != NULL)
                {
                    m_pArrayElementBasicType = pArrayElementType->cast_ptr<BasicType>();
                }

                m_pMallocDeclaration = Shared::findSubroutineByName(Shared::getTranslationUnit(&m_iterationStatement), "malloc");
                m_pFreeDeclaration = Shared::findSubroutineByName(Shared::getTranslationUnit(&m_iterationStatement), "free");
            }

            MPIDataDistributionIteration::~MPIDataDistributionIteration()
            {
                delete m_pHelper;
                delete m_pRankProducer;
                delete m_pFactory;
            }

            void MPIDataDistributionIteration::setNewArrayDeclaration( VariableDeclaration& newArrayDeclaration )
            {
                m_pNewArrayDeclaration = &newArrayDeclaration;
            }

            bool MPIDataDistributionIteration::analyseApplicability()
            {
                m_errors.clear();

                OPS_ASSERT(m_pHelper != NULL);
                if(!m_pHelper->validate())
                {
                    m_errors.push_back(_TL("MPI library is not included or not correct.", "Библиотека MPI не подключена или некорректна."));
                }

                OPS_ASSERT(m_pRankProducer != NULL);
                if(!m_pRankProducer->analyseApplicability())
                {
                    m_errors.push_back(m_pRankProducer->getErrorMessage());
                }

                if(m_pArrayElementBasicType == NULL)
                {
                    m_errors.push_back(_TL("Elements of distributing array must be of basic type.", "Элементы распределяемого массива должны иметь базовый тип."));
                }

                if(m_pMallocDeclaration == NULL)
                {
                    m_errors.push_back(_TL("Cannot find malloc subroutine.", "Не удалось найти функцию malloc."));
                }

                if (m_pFreeDeclaration == NULL)
                {
                    m_errors.push_back(_TL("Cannot find free subroutine.", "Не удалось найти функцию free."));
                }

                if(m_leadingDimention < 0 || m_leadingDimention > (int)m_distributionParameters.dims.size())
                {
                    m_errors.push_back(_TL("Wrong leading dimention.", "Некорректное ведущее измерение."));
                }

                // TODO: Проверка: соотносятся ли m_generatorParameters с m_distributionParameters
                if(!m_generatorParameters.isValid())
                {
                    m_errors.push_back(_TL("Invalid parameters of data distribution.", "Некорректные параметры размещения данных."));
                }

                m_analysisRerformed = true;

                return m_errors.size() == 0;
            }

            std::string MPIDataDistributionIteration::getErrorMessage()
            {
                std::string error = "";

                for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
                {
                    error = error + "\n" + *it;
                }

                return error;
            }

            void MPIDataDistributionIteration::makeTransformation()
            {
                if(!m_analysisRerformed)
                {
                    analyseApplicability();
                }

                OPS_ASSERT(m_errors.size() == 0);

                m_generatorParameters.normalize();

				// Заводим блок, в котором будут находиться все новые операторы
                ReprisePtr<BlockStatement> rpOutputBlock(new BlockStatement());
                m_iterationStatement.getBody().addLast(rpOutputBlock.get());

                // Заводим переменную rank
                m_pRankDeclaration = MPIHelper::getRankDeclaration(*Shared::getSubroutineDeclarationByStatement(&m_iterationStatement));
                if(m_pRankDeclaration == NULL)
                {
                    m_pRankProducer->makeTransformation();
                    m_pRankDeclaration = MPIHelper::getRankDeclaration(*Shared::getSubroutineDeclarationByStatement(&m_iterationStatement));
                }

                // Статус
                TypeBase& statusType = *m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_STATUS);
                VariableDeclaration& status = Editing::createNewVariable(statusType, *rpOutputBlock, "_STATUS");


				std::list<NeightbourRankSendInfo> allNeightbourRankSendInfos = addSendStatements(rpOutputBlock, status);
				addReceiveStatements(rpOutputBlock);

                // Для каждого соседа, которому нужно что-то отправить, освобождаем буфер.
                for (std::list<NeightbourRankSendInfo>::iterator it = allNeightbourRankSendInfos.begin(); it != allNeightbourRankSendInfos.end(); ++it)
                {
                    rpOutputBlock->addLast(createMPIWaitStatement(
                        R_AD() *(new ReferenceExpression(*it->m_pRequestDeclaration)),
                        R_AD() *(new ReferenceExpression(status))
                        ).get());
                    rpOutputBlock->addLast(createMemoryDeallocatingStatement(*(it->m_bufferDeclaration)).get());
                }
            }

			void MPIDataDistributionIteration::addReceiveStatements(ReprisePtr<BlockStatement> rpOutputBlock)
			{
				IntegerHelper ihInt32(BasicType::BT_INT32);

				// Статус
				TypeBase& statusType = *m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_STATUS);
				VariableDeclaration& status = Editing::createNewVariable(statusType, *rpOutputBlock, "_STATUS");

				// Данные о соседних блоках
				std::list<NeightbourRankSendInfo> allNeightbourRankSendInfos = getAllNeightbourRankSendInfo();

				// Для каждого соседа, от которого должно что-то прийти, формируем буфер, и вызываем IRecv.
				for (std::list<NeightbourRankSendInfo>::iterator it = allNeightbourRankSendInfos.begin(); it != allNeightbourRankSendInfos.end(); ++it)
				{
					VariableDeclaration& receivingBufferDeclaration = Editing::createNewVariable(*(new PtrType(m_pArrayElementBasicType)), *rpOutputBlock, "_RECV_BUF_");
					rpOutputBlock->addLast(createMemoryAllocatingStatement(receivingBufferDeclaration, it->m_elementsToSendCount, *m_pArrayElementBasicType).get());
					it->m_bufferDeclaration = &receivingBufferDeclaration;

					it->m_pRequestDeclaration = &Editing::createNewVariable(*m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_REQUEST), *rpOutputBlock, "_REQUEST");

                    IntegerHelper ihCount(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 1)->cast_to<BasicType>());
                    IntegerHelper ihSource(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 3)->cast_to<BasicType>());
                    IntegerHelper ihTag(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 4)->cast_to<BasicType>());

					rpOutputBlock->addLast(createMPIIRecvStatement(
						*(new ReferenceExpression(receivingBufferDeclaration)),
						ihCount(it->m_elementsToSendCount),
						*m_pHelper->createMPIBasicTypeCode(m_pFactory->createMPIBasicTypesHelper()->getMPIBasicTypeByRepriseBasicType(m_pArrayElementBasicType->getKind())),
						(*(new ReferenceExpression(*m_pRankDeclaration)) + ihSource(m_distributionParameters.P - it->m_rankOffset)) % ihSource(m_distributionParameters.P),
						ihTag(0),
						*m_pHelper->createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD),
						R_AD()*(new ReferenceExpression(*it->m_pRequestDeclaration))
					).get());
				}

				int dimentionsCount = m_distributionParameters.dims.size();

				// Формируем код получения данных для каждого узла

				// Обрабатываем каждый процесс, от которых нужно получить данные
				for (std::list<NeightbourRankSendInfo>::iterator nrsIt = allNeightbourRankSendInfos.begin(); nrsIt != allNeightbourRankSendInfos.end(); ++nrsIt)
				{
					NeightbourRankSendInfo nrsi = *nrsIt;

					// Добавляем MPI_Wait
					rpOutputBlock->addLast(createMPIWaitStatement(
						R_AD()*(new ReferenceExpression(*nrsi.m_pRequestDeclaration)),
						R_AD()*(new ReferenceExpression(status))
					).get());

					// Формируем цикл по блокам
					VariableDeclaration& blockCounterDeclaration = Editing::createNewVariable(*BasicType::basicType(BasicType::BT_INT32), *rpOutputBlock, "_BLOCK_IND_");
					ReprisePtr<ReferenceExpression> rpBlockCounterReference(new ReferenceExpression(blockCounterDeclaration));

					ForStatement* pBlockForStatement = new ForStatement();

					pBlockForStatement->setInitExpression( &( op(*rpBlockCounterReference) R_AS ihInt32(0) ) );
					pBlockForStatement->setFinalExpression( &( op(*rpBlockCounterReference) < ihInt32(m_distributionParameters.getBlocksInEachProcessorCount(m_leadingDimention)) ) );
					pBlockForStatement->setStepExpression( &( op(*rpBlockCounterReference) R_AS ( op(*rpBlockCounterReference) + ihInt32(1)) ) );

					rpOutputBlock->addLast(pBlockForStatement);
				
					// Обрабатываем каждую часть получения
					int sendinBufferOffset = 0;
					for (std::list<NeightbourSendInfo>::iterator nsIt = nrsi.m_neightbourInfos.begin(); nsIt != nrsi.m_neightbourInfos.end(); ++nsIt)
					{
						NeightbourSendInfo nsi = *nsIt;

						// Добавляем гнездо цикла по элементам внутри присланого куска. Запоминаем счетчики.
						std::vector<VariableDeclaration*> partsIndexes;
						partsIndexes.resize(dimentionsCount);
						BlockStatement* pBlockToAdd = &pBlockForStatement->getBody();
						for (int dimention = 0; dimention < dimentionsCount; ++dimention)
						{
							IntegerHelper ih(BasicType::BT_INT32);
							VariableDeclaration& counterDeclaration = Editing::createNewVariable(*BasicType::basicType(BasicType::BT_INT32), *pBlockToAdd, "_IND_");
							ReprisePtr<ReferenceExpression> rpCounterReference(new ReferenceExpression(counterDeclaration));

							ForStatement* pForStatement = new ForStatement();

							pForStatement->setInitExpression( &( op(*rpCounterReference) R_AS ih(getReceivingStartIndex(dimention, nsi.m_neightbourInfo)) ) );
							pForStatement->setFinalExpression( &( op(*rpCounterReference) < ih(getReceivingEndIndex(dimention, nsi.m_neightbourInfo)) ) );
							pForStatement->setStepExpression( &( op(*rpCounterReference) R_AS ( op(*rpCounterReference) + ih(1)) ) );

							pBlockToAdd->addLast(pForStatement);

							pBlockToAdd = &pForStatement->getBody();
							partsIndexes[dimention] = &counterDeclaration;
						}

						// Добавляем копирование
						ReprisePtr<ReferenceExpression> rpReceivingBufferReference(new ReferenceExpression(*nrsi.m_bufferDeclaration));
						ReprisePtr<ReferenceExpression> rpTargetArrayReference(new ReferenceExpression(*m_pNewArrayDeclaration));
						pBlockToAdd->addLast(
							new ExpressionStatement(&(
								op(rpTargetArrayReference) R_BK(op(getTargetArrayIndex(blockCounterDeclaration, partsIndexes))) R_AS
								op(rpReceivingBufferReference) R_BK( op(op(getReceivingBufferArrayIndex(blockCounterDeclaration, partsIndexes, nsi) ) + ihInt32(sendinBufferOffset)) )
							))
						);

						sendinBufferOffset += nsi.m_elementsToSendCount;
					}
				}
				
				// Для каждого соседа, от которого нужно что-то получить, освобождаем буфер.
				for (std::list<NeightbourRankSendInfo>::iterator it = allNeightbourRankSendInfos.begin(); it != allNeightbourRankSendInfos.end(); ++it)
				{
					rpOutputBlock->addLast(createMemoryDeallocatingStatement(*(it->m_bufferDeclaration)).get());
				}
			}

			std::list<MPIDataDistributionIteration::NeightbourRankSendInfo> MPIDataDistributionIteration::addSendStatements(ReprisePtr<BlockStatement> rpOutputBlock, VariableDeclaration& status)
			{
				IntegerHelper ihInt32(BasicType::BT_INT32);

				// Данные о соседних блоках
				std::list<NeightbourRankSendInfo> allNeightbourRankSendInfos = getAllNeightbourRankSendInfo();

				// Для каждого соседа, которому нужно что-то отправить, формируем буфер.
				for (std::list<NeightbourRankSendInfo>::iterator it = allNeightbourRankSendInfos.begin(); it != allNeightbourRankSendInfos.end(); ++it)
				{
					VariableDeclaration& sendingBufferDeclaration = Editing::createNewVariable(*(new PtrType(m_pArrayElementBasicType)), *rpOutputBlock, "_SEND_BUF_");
					rpOutputBlock->addLast(createMemoryAllocatingStatement(sendingBufferDeclaration, it->m_elementsToSendCount, *m_pArrayElementBasicType).get());
					it->m_bufferDeclaration = &sendingBufferDeclaration;

					it->m_pRequestDeclaration = &Editing::createNewVariable(*m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_REQUEST), *rpOutputBlock, "_REQUEST");
				}

				int dimentionsCount = m_distributionParameters.dims.size();

				// Формируем код отправки-получения данных для каждого узла

				// Все мультииндексы блоков внутри клетки
				BADDParametersFamily::MultyIndexList allMultyIndexes = m_distributionParameters.getBlocksMultyIndecesInCell();

				IntegerHelper ihRank(m_pRankProducer->getRankType()->getKind());
				for (int processNumber = 0; processNumber < m_distributionParameters.P; ++processNumber)
				{
					// Добавляем if (rank == XXX)
					ReprisePtr<IfStatement> rpIfProcessNumberStatement(new IfStatement());
					rpOutputBlock->addLast(rpIfProcessNumberStatement.get());
					rpIfProcessNumberStatement->setCondition(&(ihRank(processNumber) == *(new ReferenceExpression(*m_pRankDeclaration))));

					// Формируем гнездо циклов по клеткам, запоминаем счетчики
					std::vector<VariableDeclaration*> cellIndexes;
					cellIndexes.resize(dimentionsCount);
					BlockStatement* pBlockToAdd = &rpIfProcessNumberStatement->getThenBody();
					for (int dimention = 0; dimention < dimentionsCount; ++dimention)
					{
						VariableDeclaration& counterDeclaration = Editing::createNewVariable(*BasicType::basicType(BasicType::BT_INT32), *pBlockToAdd, "_CELL_IND_");
						ReprisePtr<ReferenceExpression> rpCounterReference(new ReferenceExpression(counterDeclaration));

						ForStatement* pForStatement = new ForStatement();

						pForStatement->setInitExpression( &( op(*rpCounterReference) R_AS ihInt32(0) ) );
						pForStatement->setFinalExpression( &( op(*rpCounterReference) < ihInt32(m_distributionParameters.getWidthInCells(dimention)) ) );
						pForStatement->setStepExpression( &( op(*rpCounterReference) R_AS ( op(*rpCounterReference) + ihInt32(1)) ) );

						pBlockToAdd->addLast(pForStatement);

						pBlockToAdd = &pForStatement->getBody();
						cellIndexes[dimention] = &counterDeclaration;
					}

					BlockStatement* pCellNestInnerBody = pBlockToAdd;
					BADDParametersFamily::MultyIndexList processMultyIndexes = m_distributionParameters.getBlocksMultyIndecesOfProcessInCell(allMultyIndexes, processNumber);

					// Обрабатываем каждый процесс, которому нужно послать данные
					for (std::list<NeightbourRankSendInfo>::iterator nrsIt = allNeightbourRankSendInfos.begin(); nrsIt != allNeightbourRankSendInfos.end(); ++nrsIt)
					{
						NeightbourRankSendInfo nrsi = *nrsIt;

						// Обрабатываем кадую часть отправки
						int sendinBufferOffset = 0;
						for (std::list<NeightbourSendInfo>::iterator nsIt = nrsi.m_neightbourInfos.begin(); nsIt != nrsi.m_neightbourInfos.end(); ++nsIt)
						{
							NeightbourSendInfo nsi = *nsIt;

							// Добавляем гнездо цикла по элементам внутри отсылаемого куска. Запоминаем счетчики.
							std::vector<VariableDeclaration*> partsIndexes;
							partsIndexes.resize(dimentionsCount);
							pBlockToAdd = pCellNestInnerBody;
							for (int dimention = 0; dimention < dimentionsCount; ++dimention)
							{
								IntegerHelper ih(BasicType::BT_INT32);
								VariableDeclaration& counterDeclaration = Editing::createNewVariable(*BasicType::basicType(BasicType::BT_INT32), *pBlockToAdd, "_IND_");
								ReprisePtr<ReferenceExpression> rpCounterReference(new ReferenceExpression(counterDeclaration));

								ForStatement* pForStatement = new ForStatement();

								pForStatement->setInitExpression( &( op(*rpCounterReference) R_AS ih(getStartIndex(dimention, nsi.m_neightbourInfo)) ) );
								pForStatement->setFinalExpression( &( op(*rpCounterReference) < ih(getEndIndex(dimention, nsi.m_neightbourInfo)) ) );
								pForStatement->setStepExpression( &( op(*rpCounterReference) R_AS ( op(*rpCounterReference) + ih(1)) ) );

								pBlockToAdd->addLast(pForStatement);

								pBlockToAdd = &pForStatement->getBody();
								partsIndexes[dimention] = &counterDeclaration;
							}

							// Добавляем копирование элементов.
							// Копирований будет столько, сколько блоков в клетке.
							ReprisePtr<ReferenceExpression> rpSendingBufferReference(new ReferenceExpression(*nrsi.m_bufferDeclaration));
							ReprisePtr<ReferenceExpression> rpSourceArrayReference(new ReferenceExpression(*m_pNewArrayDeclaration));
							for (BADDParametersFamily::MultyIndexList::iterator multyIndexIt = processMultyIndexes.begin(); multyIndexIt != processMultyIndexes.end(); ++multyIndexIt)
							{
								pBlockToAdd->addLast(
									new ExpressionStatement(&(
										op(rpSendingBufferReference) R_BK(op(op(getSendingBufferArrayIndex(cellIndexes, partsIndexes, *multyIndexIt, nsi)) + ihInt32(sendinBufferOffset))) R_AS	
										op(rpSourceArrayReference) R_BK(op(getSourceArrayIndex(cellIndexes, partsIndexes, *multyIndexIt)))	
									))
								);
							}

							sendinBufferOffset += nsi.m_elementsToSendCount;
						}

						// После того, как заполнили буфер, отсылаем его
                        IntegerHelper ihCount(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 1)->cast_to<BasicType>());
                        IntegerHelper ihDest(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 3)->cast_to<BasicType>());
                        IntegerHelper ihTag(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 4)->cast_to<BasicType>());
						int sendToProcessNumber = (processNumber + nrsi.m_rankOffset) % m_distributionParameters.P;
						rpIfProcessNumberStatement->getThenBody().addLast(createMPIISendStatement(
							*(new ReferenceExpression(*nrsi.m_bufferDeclaration)),
							ihCount(nrsi.m_elementsToSendCount),
							*m_pHelper->createMPIBasicTypeCode(m_pFactory->createMPIBasicTypesHelper()->getMPIBasicTypeByRepriseBasicType(m_pArrayElementBasicType->getKind())),
							ihDest(sendToProcessNumber),
							ihTag(0),
							*m_pHelper->createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD),
							R_AD()*(new ReferenceExpression(*nrsi.m_pRequestDeclaration))
							).get());
					}
				}

                return allNeightbourRankSendInfos;
			}

            ReprisePtr<ExpressionBase> MPIDataDistributionIteration::getSendingBufferArrayIndex(
                const std::vector<VariableDeclaration*>& cellIndexes,
                const std::vector<VariableDeclaration*>& partsIndexes,
                const BADDParametersFamily::MultyIndex& blockMultyIndex,
                const MPIDataDistributionIteration::NeightbourSendInfo& neightbourSendInfo
                )
            {
                std::vector<ExpressionBase*> cellIndexesExpressions;
                cellIndexesExpressions.resize(cellIndexes.size());

                int blocksInCellInEachProcess = m_distributionParameters.getBlocksInEachProcessorCount(m_leadingDimention) / m_distributionParameters.getCellsCount();
				int newBlockNumberInEmbedding = (getBlockIndexInEmbedding(blockMultyIndex) + neightbourSendInfo.m_neightbourInfo[m_leadingDimention] + blocksInCellInEachProcess) % blocksInCellInEachProcess;

                IntegerHelper ih(BasicType::BT_INT32);

                for (std::vector<VariableDeclaration*>::size_type dimention = 0; dimention < cellIndexes.size(); ++dimention)
                {
                    // Нужно отследить смену клетки
                    int blockIndexInSlice = blockMultyIndex[dimention] + neightbourSendInfo.m_neightbourInfo[dimention];
                    int widthInCells = m_distributionParameters.getWidthInCells(dimention);
                    int cellWidthInBlocks = m_distributionParameters.getCellWidthInBlocks(dimention);
                    if(blockIndexInSlice < -1)
                    {
                        OPS_ASSERT(false);
                    }
                    else if(blockIndexInSlice == -1) // Залезли в левый блок
                    {
                        cellIndexesExpressions[dimention] = &( ( *(new ReferenceExpression(*cellIndexes[dimention])) + ih(widthInCells - 1) ) % ih(widthInCells) );
                    }
                    else if (blockIndexInSlice < cellWidthInBlocks) // Остались в той же клетке
                    {
                        cellIndexesExpressions[dimention] = new ReferenceExpression(*cellIndexes[dimention]);
                    }
                    else if (blockIndexInSlice == cellWidthInBlocks) // Залезли в правый блок
                    {
                        cellIndexesExpressions[dimention] = &( ( *(new ReferenceExpression(*cellIndexes[dimention])) + ih(1) ) % ih(widthInCells) );
                    }
                    else
                    {
                        OPS_ASSERT(false);
                    }
                }

                // Формируем индекс

                // Формируем номер клетки в укладке
                ReprisePtr<ExpressionBase> rpCellNumber(&ih(0));
                for (std::vector<VariableDeclaration*>::size_type dimention = 0; dimention < cellIndexesExpressions.size(); ++dimention)
                {
                    rpCellNumber.reset(&(*rpCellNumber * ih(m_distributionParameters.getWidthInCells(dimention)) + *cellIndexesExpressions[dimention]));
                }

                // Формируем номер элемена в отправляемом куске
                ReprisePtr<ExpressionBase> rpElementNumber(&ih(0));
                for (std::vector<VariableDeclaration*>::size_type dimention = 0; dimention < partsIndexes.size(); ++dimention)
                {
                    rpElementNumber.reset(&(*rpElementNumber * ih(getEndIndex(dimention, neightbourSendInfo.m_neightbourInfo) - getStartIndex(dimention, neightbourSendInfo.m_neightbourInfo)) + *(new ReferenceExpression(*partsIndexes[dimention])) - ih(getStartIndex(dimention, neightbourSendInfo.m_neightbourInfo))));
                }

                ReprisePtr<ExpressionBase> rpResult(&(
                    *rpCellNumber * ih(getElementsInCellInEmbeddingCount()) + 
                    ih(newBlockNumberInEmbedding * neightbourSendInfo.m_elementsInBlockToSendCount) +
                    *rpElementNumber));
                return rpResult;
            }

			ReprisePtr<ExpressionBase> MPIDataDistributionIteration::getSourceArrayIndex(
				const std::vector<VariableDeclaration*>& cellIndexes,
				const std::vector<VariableDeclaration*>& partsIndexes,
				const BADDParametersFamily::MultyIndex& blockMultyIndex
				)
			{
				std::vector<ExpressionBase*> cellIndexesExpressions;
				cellIndexesExpressions.resize(cellIndexes.size());

				// Формируем индекс
                IntegerHelper ih(BasicType::BT_INT32);

				// Формируем номер клетки в укладке
				ReprisePtr<ExpressionBase> rpCellNumber(&ih(0));
				for (std::vector<VariableDeclaration*>::size_type dimention = 0; dimention < cellIndexesExpressions.size(); ++dimention)
				{
					rpCellNumber.reset(&(*rpCellNumber * ih(m_distributionParameters.getWidthInCells(dimention)) + *(new ReferenceExpression(*cellIndexes[dimention]))));
				}

				// Формируем номер элемена в отправляемом куске
				ReprisePtr<ExpressionBase> rpElementNumber(&ih(0));
				for (std::vector<VariableDeclaration*>::size_type dimention = 0; dimention < partsIndexes.size(); ++dimention)
				{
					rpElementNumber.reset(&(*rpElementNumber * ih(m_distributionParameters.leftOverlapping[dimention] + m_distributionParameters.d[dimention] + m_distributionParameters.rightOverlapping[dimention]) + *(new ReferenceExpression(*partsIndexes[dimention]))));
				}

				ReprisePtr<ExpressionBase> rpResult(&(
					*rpCellNumber * ih(getElementsInCellInEmbeddingCount()) + 
					ih(getBlockIndexInEmbedding(blockMultyIndex) * m_distributionParameters.getElementsInBlockCount()) +
					*rpElementNumber));
				return rpResult;
			}
			ReprisePtr<ExpressionBase> MPIDataDistributionIteration::getTargetArrayIndex(
				VariableDeclaration& blockIndex,
				const std::vector<VariableDeclaration*>& partsIndexes
				)
			{
				// Формируем индекс
				IntegerHelper ih(BasicType::BT_INT32);

				// Формируем номер элемена в отправляемом куске
				ReprisePtr<ExpressionBase> rpElementNumber(&ih(0));
				for (std::vector<VariableDeclaration*>::size_type dimention = 0; dimention < partsIndexes.size(); ++dimention)
				{
					rpElementNumber.reset(&(*rpElementNumber * ih(m_distributionParameters.leftOverlapping[dimention] + m_distributionParameters.d[dimention] + m_distributionParameters.rightOverlapping[dimention]) + *(new ReferenceExpression(*partsIndexes[dimention]))));
				}

				ReprisePtr<ExpressionBase> rpResult(&(
					*(new ReferenceExpression(blockIndex)) * ih(m_distributionParameters.getElementsInBlockCount()) + 
					*rpElementNumber));
				return rpResult;
			}
			ReprisePtr<ExpressionBase> MPIDataDistributionIteration::getReceivingBufferArrayIndex(
				VariableDeclaration& blockIndex,
				const std::vector<VariableDeclaration*>& partsIndexes,
				const MPIDataDistributionIteration::NeightbourSendInfo& neightbourSendInfo
				)
			{
				IntegerHelper ih(BasicType::BT_INT32);
				
				// Формируем индекс

				// Формируем номер элемена в отправляемом куске
				ReprisePtr<ExpressionBase> rpElementNumber(&ih(0));
				for (std::vector<VariableDeclaration*>::size_type dimention = 0; dimention < partsIndexes.size(); ++dimention)
				{
					rpElementNumber.reset(&(*rpElementNumber * ih(getReceivingEndIndex(dimention, neightbourSendInfo.m_neightbourInfo) - getReceivingStartIndex(dimention, neightbourSendInfo.m_neightbourInfo)) + *(new ReferenceExpression(*partsIndexes[dimention])) - ih(getReceivingStartIndex(dimention, neightbourSendInfo.m_neightbourInfo))));
				}

				ReprisePtr<ExpressionBase> rpResult(&(
					*(new ReferenceExpression(blockIndex)) * ih(neightbourSendInfo.m_elementsToSendCount) + 
					*rpElementNumber));
				return rpResult;
			}

			int MPIDataDistributionIteration::getBlockIndexInEmbedding(BADDParametersFamily::MultyIndex blockMultyIndex)
			{
				if(m_distributionParameters.dims.size() == 1)
				{
					return 0;
				}

				return blockMultyIndex[m_leadingDimention];
			}

			int MPIDataDistributionIteration::getElementsInCellInEmbeddingCount()
			{
				return m_distributionParameters.getElementsInEachProcessorCount(m_leadingDimention) / m_distributionParameters.getCellsCount();
			}

            ReprisePtr<StatementBase> MPIDataDistributionIteration::createMemoryAllocatingStatement(VariableDeclaration& variableDeclaration, int sizeInElements, BasicType& typeOfElements)
            {
                OPS_ASSERT(m_pMallocDeclaration != NULL);

                ReprisePtr<SubroutineReferenceExpression> rpMallocReferenceExpression(new SubroutineReferenceExpression(*m_pMallocDeclaration));
                ReprisePtr<SubroutineCallExpression> rpMallocCallExpression(new SubroutineCallExpression(rpMallocReferenceExpression.get()));

                IntegerHelper ih(BasicType::BT_INT32);
                ReprisePtr<ExpressionBase> rpSizeExpression(&(ih(sizeInElements * typeOfElements.getSizeOf())));

                rpMallocCallExpression->addArgument(rpSizeExpression.get());

                ReprisePtr<ExpressionBase> rpAssign(new BasicCallExpression(
                    BasicCallExpression::BCK_ASSIGN, 
                    new ReferenceExpression(variableDeclaration), 
                    new TypeCastExpression(new PtrType(&typeOfElements), rpMallocCallExpression.get())));

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpAssign.get()));

                return result;
            }

            ReprisePtr<StatementBase> MPIDataDistributionIteration::createMemoryDeallocatingStatement(VariableDeclaration& variableDeclaration)
            {
                OPS_ASSERT(m_pFreeDeclaration != NULL);

                ReprisePtr<SubroutineReferenceExpression> rpFreeReferenceExpression(new SubroutineReferenceExpression(*m_pFreeDeclaration));
                ReprisePtr<SubroutineCallExpression> rpFreeCallExpression(new SubroutineCallExpression(rpFreeReferenceExpression.get()));

                rpFreeCallExpression->addArgument(new ReferenceExpression(variableDeclaration));
                ReprisePtr<StatementBase> result(new ExpressionStatement(rpFreeCallExpression.get()));

                return result;
            }

			ReprisePtr<StatementBase> MPIDataDistributionIteration::createMPIISendStatement( ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& dest, ExpressionBase& tag, ExpressionBase& comm, ExpressionBase& request )
			{
				OPS_ASSERT(m_pHelper != NULL);

				ReprisePtr<SubroutineCallExpression> rpMPIISendCallExpression(m_pHelper->createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_ISEND));

				rpMPIISendCallExpression->addArgument(&buf);
				rpMPIISendCallExpression->addArgument(&count);
				rpMPIISendCallExpression->addArgument(&datatype);
				rpMPIISendCallExpression->addArgument(&dest);
				rpMPIISendCallExpression->addArgument(&tag);
				rpMPIISendCallExpression->addArgument(&comm);
				rpMPIISendCallExpression->addArgument(&request);

				ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPIISendCallExpression.get()));

				return result;
			}
            ReprisePtr<StatementBase> MPIDataDistributionIteration::createMPIWaitStatement( ExpressionBase& request, ExpressionBase& status )
            {
                OPS_ASSERT(m_pHelper != NULL);

                ReprisePtr<SubroutineCallExpression> rpMPIWaitCallExpression(m_pHelper->createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_WAIT));

                rpMPIWaitCallExpression->addArgument(&request);
                rpMPIWaitCallExpression->addArgument(&status);

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPIWaitCallExpression.get()));

                return result;
            }

            ReprisePtr<StatementBase> MPIDataDistributionIteration::createMPIIRecvStatement( ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& source, ExpressionBase& tag, ExpressionBase& comm, ExpressionBase& request )
            {
                OPS_ASSERT(m_pHelper != NULL);

                ReprisePtr<SubroutineCallExpression> rpMPIIRecvCallExpression(m_pHelper->createMPIFunctionCallExpression(MPIFunctionsHelper::MF_MPI_IRECV));

                rpMPIIRecvCallExpression->addArgument(&buf);
                rpMPIIRecvCallExpression->addArgument(&count);
                rpMPIIRecvCallExpression->addArgument(&datatype);
                rpMPIIRecvCallExpression->addArgument(&source);
                rpMPIIRecvCallExpression->addArgument(&tag);
                rpMPIIRecvCallExpression->addArgument(&comm);
                rpMPIIRecvCallExpression->addArgument(&request);

                ReprisePtr<StatementBase> result(new ExpressionStatement(rpMPIIRecvCallExpression.get()));

                return result;
            }

            std::list<MPIDataDistributionIteration::NeightbourRankSendInfo> MPIDataDistributionIteration::getAllNeightbourRankSendInfo()
            {
                std::list<NeightbourSendInfo> allNeightbourSendInfos = getAllNeightbourSendInfo();

                // Индекс = NeightbourSendInfo.m_rankOffset
                std::vector<NeightbourRankSendInfo> allNeightbourRankSendInfos;
                allNeightbourRankSendInfos.resize(m_distributionParameters.P);
                for(int i = 0; i < m_distributionParameters.P; ++i)
                {
                    allNeightbourRankSendInfos[i].m_rankOffset = i;
                    allNeightbourRankSendInfos[i].m_elementsToSendCount = 0;
                }

                for(std::list<NeightbourSendInfo>::iterator it = allNeightbourSendInfos.begin(); it != allNeightbourSendInfos.end(); ++it)
                {
                    NeightbourSendInfo nsi = *it;
                    if(nsi.m_elementsToSendCount > 0)
                    {
                        allNeightbourRankSendInfos[nsi.m_rankOffset].m_elementsToSendCount += nsi.m_elementsToSendCount;
                        allNeightbourRankSendInfos[nsi.m_rankOffset].m_neightbourInfos.push_back(nsi);
                    }
                }

                std::list<NeightbourRankSendInfo> result;
                // Первую итерацию отбрасываем - у нее m_rankOffset == 0. А это значит что элементы нужно послать себе же.
                for(int i = 1; i < m_distributionParameters.P; ++i)
                {
                    if(allNeightbourRankSendInfos[i].m_elementsToSendCount > 0)
                    {
                        result.push_back(allNeightbourRankSendInfos[i]);
                    }
                }

                return result;
            }

            int MPIDataDistributionIteration::getStartIndex(int dimention, const NeightbourInfo& neightbour)
            {
                if(neightbour[dimention] == -1)
                {
                    return m_distributionParameters.leftOverlapping[dimention] + m_generatorParameters.a[dimention];
                }
                else if (neightbour[dimention] == 0)
                {
                    return m_distributionParameters.leftOverlapping[dimention];
                }
                else if (neightbour[dimention] == 1)
                {
                    return m_distributionParameters.d[dimention];
                }
                else
                {
                    OPS_ASSERT(false);
					throw OPS::RuntimeError("Unexpected neightbour[dimention] value");
                }
            }

			int MPIDataDistributionIteration::getReceivingStartIndex(int dimention, const NeightbourInfo& neightbour)
			{
				if(neightbour[dimention] == -1)
				{
					return m_distributionParameters.leftOverlapping[dimention] + m_distributionParameters.d[dimention] + m_generatorParameters.a[dimention];
				}
				else if (neightbour[dimention] == 0)
				{
					return m_distributionParameters.leftOverlapping[dimention];
				}
				else if (neightbour[dimention] == 1)
				{
					return 0;
				}
				else
				{
					OPS_ASSERT(false);
					throw OPS::RuntimeError("Unexpected neightbour[dimention] value");
				}
			}

			int MPIDataDistributionIteration::getReceivingEndIndex(int dimention, const NeightbourInfo& neightbour)
			{
				if(neightbour[dimention] == -1)
				{
					return m_distributionParameters.leftOverlapping[dimention] + m_distributionParameters.d[dimention] + m_distributionParameters.rightOverlapping[dimention];
				}
				else if (neightbour[dimention] == 0)
				{
					return m_distributionParameters.leftOverlapping[dimention] + m_distributionParameters.d[dimention];
				}
				else if (neightbour[dimention] == 1)
				{
					return m_distributionParameters.leftOverlapping[dimention] + m_generatorParameters.a[dimention];
				}
				else
				{
					OPS_ASSERT(false);
					throw OPS::RuntimeError("Unexpected neightbour[dimention] value");
				}
			}

            int MPIDataDistributionIteration::getEndIndex(int dimention, const NeightbourInfo& neightbour)
            {
                if(neightbour[dimention] == -1)
                {
                    return m_distributionParameters.leftOverlapping[dimention] + m_distributionParameters.rightOverlapping[dimention];
                }
                else if (neightbour[dimention] == 0)
                {
                    return m_distributionParameters.leftOverlapping[dimention] + m_distributionParameters.d[dimention];
                }
                else if (neightbour[dimention] == 1)
                {
                    return m_distributionParameters.leftOverlapping[dimention] + m_distributionParameters.d[dimention] + m_generatorParameters.a[dimention];
                }
                else
                {
                    OPS_ASSERT(false);
					throw OPS::RuntimeError("Unexpected neightbour[dimention] value");
                }
            }

            std::list<MPIDataDistributionIteration::NeightbourSendInfo> MPIDataDistributionIteration::getAllNeightbourSendInfo()
            {
                std::list<NeightbourSendInfo> result;

                std::list<NeightbourInfo> allNeightbours = getAllNeightbours();
                for(std::list<NeightbourInfo>::iterator it = allNeightbours.begin(); it != allNeightbours.end(); ++it)
                {
                    NeightbourInfo neightbour = *it;

                    NeightbourSendInfo nsi;
                    nsi.m_neightbourInfo = neightbour;
                    nsi.m_rankOffset = getRankOffset(neightbour);
                    nsi.m_elementsToSendCount = getElementsToSendCount(neightbour);
					nsi.m_elementsInBlockToSendCount = nsi.m_elementsToSendCount / m_distributionParameters.getBlocksInEachProcessorCount(m_leadingDimention);

                    result.push_back(nsi);
                }

                return result;
            }

            int MPIDataDistributionIteration::getRankOffset(const MPIDataDistributionIteration::NeightbourInfo& neightbour)
            {
                int result = 0;

                int dimentionsCount = m_distributionParameters.dims.size();
                for (int dimention = 0; dimention < dimentionsCount; ++dimention)
                {
                    result += m_distributionParameters.P + neightbour[dimention] * m_distributionParameters.s[dimention];
                }

                return result % m_distributionParameters.P;
            }

            int MPIDataDistributionIteration::getElementsToSendCount(const MPIDataDistributionIteration::NeightbourInfo& neightbour)
            {
                int result = 1;
                int dimentionsCount = m_distributionParameters.dims.size();

                for(int dimention = 0; dimention < dimentionsCount; ++dimention)
                {
                    if(neightbour[dimention] == -1) // сосед слева
                    {
                        if(m_distributionParameters.rightOverlapping[dimention] > 0) // есть перекрытие справа
                        {
                            result *= m_distributionParameters.rightOverlapping[dimention] - m_generatorParameters.a[dimention];
                        }
                        else if (m_generatorParameters.a[dimention] < 0)
                        {
                            result *= m_generatorParameters.a[dimention];
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    else if(neightbour[dimention] == 0)
                    {
                        result *= m_distributionParameters.d[dimention];
                    }
                    else if(neightbour[dimention] == 1)
                    {
                        if(m_distributionParameters.leftOverlapping[dimention] > 0)
                        {
                            result *= m_distributionParameters.leftOverlapping[dimention] + m_generatorParameters.a[dimention];
                        }
                        else if(m_generatorParameters.a[dimention] > 0)
                        {
                            result *= m_generatorParameters.a[dimention];
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        OPS_ASSERT(false);
                    }
                }

				result *= m_distributionParameters.getBlocksInEachProcessorCount(m_leadingDimention);

                return result;
            }

            std::list<MPIDataDistributionIteration::NeightbourInfo> MPIDataDistributionIteration::getAllNeightbours()
            {
                int dimentionsCount = m_distributionParameters.dims.size();

                NeightbourInfo startNeightbour;
                startNeightbour.resize(dimentionsCount);
                for(int i = 0; i < dimentionsCount; ++i)
                {
                    startNeightbour[i] = 0;
                }

                std::list<NeightbourInfo> result;
                result.push_back(startNeightbour);

                for (int dimention = 0; dimention < dimentionsCount; ++dimention)
                {
                    result = getNeightboursByDimention(dimention, dimentionsCount, result);
                }

                return result;
            }

            std::list<MPIDataDistributionIteration::NeightbourInfo> MPIDataDistributionIteration::getNeightboursByDimention(int dimention, int dimentionsCount, const std::list<MPIDataDistributionIteration::NeightbourInfo>& oldNeightbours)
            {
                if(dimention == dimentionsCount)
                {
                    return oldNeightbours;
                }

                std::list<NeightbourInfo> newNeightbours;
                for(std::list<NeightbourInfo>::const_iterator it = oldNeightbours.begin(); it != oldNeightbours.end(); ++it)
                {
                    NeightbourInfo neightbour1 = *it;
                    neightbour1[dimention] = -1;
                    newNeightbours.push_back(neightbour1);
                    NeightbourInfo neightbour2 = *it;
                    neightbour2[dimention] = 0;
                    newNeightbours.push_back(neightbour2);
                    NeightbourInfo neightbour3 = *it;
                    neightbour3[dimention] = 1;
                    newNeightbours.push_back(neightbour3);
                }

                return newNeightbours;
            }
        }
    }
}
