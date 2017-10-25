#include "Transforms/DataDistribution/MPI/Block/BlockScatter.h"

#include "OPS_Core/Localization.h"
#include "OPS_Core/MemoryHelper.h"
#include "Shared/DataShared.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/SubroutinesShared.h"
#include "Transforms/DataDistribution/MPI/Block/BufferCopyingBuilder.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
    using namespace OPS::Transforms::MPIProducer;
    using namespace OPS::Reprise;
    using namespace OPS::Shared::ExpressionHelpers;

    BlockScatter::BlockScatter(
            BADDParametersFamily distributionParameters,
            VariableDeclaration& targetArrayDeclaration,
            StatementBase& insertionPointStatement,
            int sourceNodeNumber
        ): m_distributionParameters(distributionParameters),
            m_targetArrayDeclaration(targetArrayDeclaration),
            m_pNewArrayDeclaration(NULL),
            m_insertionPointStatement(insertionPointStatement),
            m_pRankDeclaration(NULL),
            m_sourceNodeNumber(sourceNodeNumber),
            m_analysisRerformed(false)
    {
        m_pFactory = new MPIProducerCFactory();

        m_pHelper = m_pFactory->createMPIHelper(Shared::getTranslationUnit(&m_insertionPointStatement));
        OPS_ASSERT(m_pHelper != NULL);

        m_pRankProducer = m_pFactory->createMPIRankProducer(Shared::getSubroutineDeclarationByStatement(&m_insertionPointStatement));
        OPS_ASSERT(m_pRankProducer != NULL);

        TypeBase* pArrayElementType = Shared::getArrayElementBasicType(&m_targetArrayDeclaration.getType());
        if(pArrayElementType == NULL) return;

        m_pArrayElementBasicType = pArrayElementType->cast_ptr<BasicType>();

        m_pMallocDeclaration = Shared::findSubroutineByName(Shared::getTranslationUnit(&m_insertionPointStatement), "malloc");
        m_pFreeDeclaration = Shared::findSubroutineByName(Shared::getTranslationUnit(&m_insertionPointStatement), "free");
    }

    BlockScatter::~BlockScatter()
    {
        delete m_pHelper;
        delete m_pRankProducer;
        delete m_pFactory;
    }

    bool BlockScatter::analyseApplicability()
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

        m_analysisRerformed = true;

        return m_errors.size() == 0;
    }

    std::string BlockScatter::getErrorMessage()
    {
        std::string error = "";

        for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
        {
            error = error + "\n" + *it;
        }

        return error;
    }

    void BlockScatter::makeTransformation()
    {
        if(!m_analysisRerformed)
        {
            analyseApplicability();
        }
        OPS_ASSERT(m_errors.size() == 0);

        OPS_ASSERT(m_pNewArrayDeclaration != NULL);

        initRankDeclaration();
        OPS_ASSERT(m_pRankDeclaration != NULL);

        ReprisePtr<BlockStatement> rpOutputBlock(new BlockStatement());
        m_insertionPointStatement.getParentBlock().addBefore(m_insertionPointStatement.getParentBlock().convertToIterator(&m_insertionPointStatement), rpOutputBlock.get());

        // if(rank == <sourceNodeNumber>)
        ReprisePtr<IfStatement> rpIfStatement(new IfStatement());
        rpOutputBlock->addLast(rpIfStatement.get());
        IntegerHelper ihRank(m_pRankProducer->getRankType()->getKind());
        rpIfStatement->setCondition(&(ihRank(m_sourceNodeNumber % m_distributionParameters.P) == *(new ReferenceExpression(*m_pRankDeclaration))));
        populateSendingBlock(rpIfStatement->getThenBody());
        populateReceivingBlock(rpIfStatement->getElseBody());
    }

    void BlockScatter::initRankDeclaration()
    {
        m_pRankDeclaration = MPIHelper::getRankDeclaration(*Shared::getSubroutineDeclarationByStatement(&m_insertionPointStatement));
        if(m_pRankDeclaration == NULL)
        {
            m_pRankProducer->makeTransformation();
            m_pRankDeclaration = MPIHelper::getRankDeclaration(*Shared::getSubroutineDeclarationByStatement(&m_insertionPointStatement));
        }
    }

    void BlockScatter::populateSendingBlock(BlockStatement& sendingBlock)
    {
        VariableDeclaration& processesCounter = Editing::createNewVariable(*BasicType::int32Type(), sendingBlock, "_processes_counter_");

        VariableDeclaration& buffersDeclaration = createBuffer2DimArray(sendingBlock);
        sendingBlock.addLast(buildBufferInitializingStatement(buffersDeclaration, processesCounter).get());

        VariableDeclaration& requestsDeclaration = createMpiRequestArray(sendingBlock);

        std::vector<VariableDeclaration*> blockRankDeclarations = createBlockRankDeclarations(sendingBlock);
        BlockStatement& assignToBufferBlock = buildLoopNestByBlocks(sendingBlock, blockRankDeclarations);

        std::tr1::shared_ptr<BufferCopyingBuilder> spBufferCopyingBuilder(new BufferCopyingBuilder(
                *Shared::getSubroutineDeclarationByStatement(&m_insertionPointStatement),
                m_distributionParameters,
                m_targetArrayDeclaration
            ));
        ReprisePtr<ExpressionBase> rpBufferReference(&(
                *(new ReferenceExpression(buffersDeclaration)) R_BK (op(buildRankByBlockRanks(blockRankDeclarations)))
            ));
        ReprisePtr<StatementBase> rpSingleBlockCopyStatement = spBufferCopyingBuilder->buildBufferCopying(
                rpBufferReference,
                blockRankDeclarations
            );
        assignToBufferBlock.addLast(rpSingleBlockCopyStatement.get());
        assignToBufferBlock.addLast(buildISendStatement(
                blockRankDeclarations,
                buffersDeclaration,
                requestsDeclaration
            ).get());

        VariableDeclaration& statusDeclaration = Editing::createNewVariable(*m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_STATUS), sendingBlock, "_status");
        sendingBlock.addLast(buildWaitStatements(requestsDeclaration, statusDeclaration, processesCounter).get());
        sendingBlock.addLast(buildBufferFreeStatements(buffersDeclaration, processesCounter).get());
    }

    VariableDeclaration& BlockScatter::createBuffer2DimArray(BlockStatement& sendingBlock)
    {
        ReprisePtr<TypeBase> rpBuffersArrayType(new ArrayType(m_distributionParameters.dims.size(), new PtrType(m_pArrayElementBasicType)));
        return Editing::createNewVariable(*rpBuffersArrayType, sendingBlock, "_buffers_");
    }

    ReprisePtr<StatementBase> BlockScatter::buildBufferInitializingStatement(VariableDeclaration& buffersDeclaration, VariableDeclaration& processesCounter)
    {
        ReprisePtr<ForStatement> rpResult = buildLoop(0, m_distributionParameters.dims.size(), processesCounter);

        ReprisePtr<IfStatement> rpIfStatement(new IfStatement(&(
                 *(new ReferenceExpression(processesCounter)) != *(new ReferenceExpression(*m_pRankDeclaration))
            )));
        rpResult->getBody().addLast(rpIfStatement.get());

        ReprisePtr<ExpressionBase> rpBufferExpression(&(
                *(new ReferenceExpression(buffersDeclaration)) R_BK (*(new ReferenceExpression(processesCounter)))
            ));
        rpIfStatement->getThenBody().addLast(
                createMemoryAllocatingStatement(
                        rpBufferExpression,
                        m_distributionParameters.getElementsInBlockCount(),
                        *m_pArrayElementBasicType
                    ).get()
            );

        rpIfStatement->getElseBody().addLast(new ExpressionStatement(&(
                op(rpBufferExpression) R_AS *(new ReferenceExpression(*m_pNewArrayDeclaration))
            )));

        return rpResult;
    }

    VariableDeclaration& BlockScatter::createMpiRequestArray(BlockStatement& sendingBlock)
    {
        ReprisePtr<TypeBase> rpMpiRequestArrayType(new ArrayType(m_distributionParameters.dims.size(), m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_REQUEST)));
        return Editing::createNewVariable(*rpMpiRequestArrayType, sendingBlock, "_requests_");
    }

    std::vector<VariableDeclaration*> BlockScatter::createBlockRankDeclarations(BlockStatement& sendingBlock)
    {
        std::vector<VariableDeclaration*> blockRankDeclarations(m_distributionParameters.dims.size());
        ReprisePtr<TypeBase> indexType(BasicType::int32Type());
        for(std::vector<int>::size_type i = 0; i < m_distributionParameters.dims.size(); ++i)
        {
            blockRankDeclarations[i] = &Editing::createNewVariable(*indexType, sendingBlock, "_block_index_");
        }

        return blockRankDeclarations;
    }

    BlockStatement& BlockScatter::buildLoopNestByBlocks(BlockStatement& parent, const std::vector<VariableDeclaration*>& blockRankDeclarations, int dimension)
    {
		if(dimension >= int(m_distributionParameters.dims.size()))
        {
            return parent;
        }

        ReprisePtr<ForStatement> rpForStatement = buildLoop(
                0,
                m_distributionParameters.dims[dimension] / m_distributionParameters.d[dimension],
                *blockRankDeclarations[dimension]
            );
        parent.addLast(rpForStatement.get());
        return buildLoopNestByBlocks(
                rpForStatement->getBody(),
                blockRankDeclarations,
                dimension + 1
            );
    }

    ReprisePtr<StatementBase> BlockScatter::buildISendStatement(const std::vector<VariableDeclaration*>& blockRankDeclarations, VariableDeclaration& buffersDeclaration, VariableDeclaration& requestsDeclaration)
    {
        ReprisePtr<ExpressionBase> rpTargetRank = buildRankByBlockRanks(blockRankDeclarations);

        ReprisePtr<IfStatement> rpResult(new IfStatement(&(
                op(rpTargetRank) != *(new ReferenceExpression(*m_pRankDeclaration))
            )));

        IntegerHelper ihCount(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_SEND), 1)->cast_to<BasicType>());
        IntegerHelper ihDest(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_SEND), 3)->cast_to<BasicType>());
        IntegerHelper ihTag(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_SEND), 4)->cast_to<BasicType>());
        rpResult->getThenBody().addLast(m_pHelper->createMPIISendStatement(
                *(new ReferenceExpression(buffersDeclaration)) R_BK (op(rpTargetRank)),
                ihCount((sdword)m_distributionParameters.getElementsInBlockCount()),
                *m_pHelper->createMPIBasicTypeCode(m_pFactory->createMPIBasicTypesHelper()->getMPIBasicTypeByRepriseBasicType(m_pArrayElementBasicType->getKind())),
                op(rpTargetRank),
                ihTag(0),
                *m_pHelper->createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD),
                R_AD() (*(new ReferenceExpression(requestsDeclaration)) R_BK (op(rpTargetRank)))
            ).get());

        return rpResult;
    }

    ReprisePtr<StatementBase> BlockScatter::buildWaitStatements(VariableDeclaration& requestsDeclaration, VariableDeclaration& statusDeclaration, VariableDeclaration& processesCounter)
    {
        ReprisePtr<ForStatement> rpResult = buildLoop(0, m_distributionParameters.dims.size(), processesCounter);

        ReprisePtr<IfStatement> rpIfStatement(new IfStatement(&(
                 *(new ReferenceExpression(processesCounter)) != *(new ReferenceExpression(*m_pRankDeclaration))
            )));
        rpResult->getBody().addLast(rpIfStatement.get());

        ReprisePtr<ExpressionBase> rpRequestExpression(&(
                R_AD()(*(new ReferenceExpression(requestsDeclaration)) R_BK (*(new ReferenceExpression(processesCounter))))
            ));
        rpIfStatement->getThenBody().addLast(m_pHelper->createMPIWaitStatement(
                *rpRequestExpression,
                R_AD()*(new ReferenceExpression(statusDeclaration))).get()
            );

        return rpResult;
    }

    ReprisePtr<StatementBase> BlockScatter::buildBufferFreeStatements(VariableDeclaration& buffersDeclaration, VariableDeclaration& processesCounter)
    {
        ReprisePtr<ForStatement> rpResult = buildLoop(0, m_distributionParameters.dims.size(), processesCounter);

        ReprisePtr<IfStatement> rpIfStatement(new IfStatement(&(
                 *(new ReferenceExpression(processesCounter)) != *(new ReferenceExpression(*m_pRankDeclaration))
            )));
        rpResult->getBody().addLast(rpIfStatement.get());

        ReprisePtr<ExpressionBase> rpBufferExpression(&(
                *(new ReferenceExpression(buffersDeclaration)) R_BK (*(new ReferenceExpression(processesCounter)))
            ));
        rpIfStatement->getThenBody().addLast(
                createMemoryDeallocatingStatement(
                        rpBufferExpression
                    ).get()
            );

        return rpResult;
    }

    void BlockScatter::populateReceivingBlock(BlockStatement& receivingBlock)
    {
        // Calling MPI_Recv
        TypeBase& statusType = *m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_STATUS);
        VariableDeclaration& status = Editing::createNewVariable(statusType, receivingBlock, "_status");

        IntegerHelper ihCount(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 1)->cast_to<BasicType>());
        IntegerHelper ihSource(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 3)->cast_to<BasicType>());
        IntegerHelper ihTag(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 4)->cast_to<BasicType>());
        receivingBlock.addLast(m_pHelper->createMPIRecvStatement(
                *(new ReferenceExpression(*m_pNewArrayDeclaration)),
                ihCount((sdword)m_distributionParameters.getElementsInBlockCount()),
                *m_pHelper->createMPIBasicTypeCode(m_pFactory->createMPIBasicTypesHelper()->getMPIBasicTypeByRepriseBasicType(m_pArrayElementBasicType->getKind())),
                ihSource(m_sourceNodeNumber % m_distributionParameters.P),
                ihTag(0),
                *m_pHelper->createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD),
                R_AD()*(new ReferenceExpression(status))
            ).get());
    }

    ReprisePtr<ExpressionBase> BlockScatter::buildRankByBlockRanks(const std::vector<VariableDeclaration*>& blockRankDeclarations)
    {
        ReprisePtr<ExpressionBase> rpResult(
                new ReferenceExpression(*blockRankDeclarations[0])
            );
        IntegerHelper ih(BasicType::BT_INT32);
		for(size_t dimension = 1; dimension < m_distributionParameters.dims.size(); ++dimension)
        {
            rpResult.reset(&(
                    op(rpResult) * ih(m_distributionParameters.dims[dimension] / m_distributionParameters.d[dimension]) + *(new ReferenceExpression(*blockRankDeclarations[dimension]))
                ));
        }

        return rpResult;
    }

    ReprisePtr<ForStatement> BlockScatter::buildLoop(int initCounterValue, int finalCounterValue, VariableDeclaration& counter)
    {
        IntegerHelper ih(BasicType::BT_INT32);

        ReprisePtr<ExpressionBase> rpCounterReference(new ReferenceExpression(counter));
        ReprisePtr<ForStatement> rpForStatement(new ForStatement());
        rpForStatement->setInitExpression(&(
                op(rpCounterReference) R_AS ih(initCounterValue)
            ));
        rpForStatement->setFinalExpression(&(
                op(rpCounterReference) < ih(finalCounterValue)
            ));
        rpForStatement->setStepExpression(&(
                op(rpCounterReference) R_AS op(rpCounterReference) + ih(1)
            ));

        return rpForStatement;
    }

    ReprisePtr<StatementBase> BlockScatter::createMemoryAllocatingStatement(ReprisePtr<ExpressionBase> pointerExpression, int sizeInElements, BasicType& typeOfElements)
    {
        OPS_ASSERT(m_pMallocDeclaration != NULL);

        ReprisePtr<SubroutineReferenceExpression> rpMallocReferenceExpression(new SubroutineReferenceExpression(*m_pMallocDeclaration));
        ReprisePtr<SubroutineCallExpression> rpMallocCallExpression(new SubroutineCallExpression(rpMallocReferenceExpression.get()));

        IntegerHelper ih(BasicType::BT_INT32);
        ReprisePtr<ExpressionBase> rpSizeExpression(&(ih(sizeInElements) * ih(typeOfElements.getSizeOf())));

        rpMallocCallExpression->addArgument(rpSizeExpression.get());

        ReprisePtr<ExpressionBase> rpAssign(new BasicCallExpression(
            BasicCallExpression::BCK_ASSIGN,
            pointerExpression->clone(),
            new TypeCastExpression(new PtrType(&typeOfElements), rpMallocCallExpression.get())));

        ReprisePtr<StatementBase> result(new ExpressionStatement(rpAssign.get()));

        return result;
    }

    ReprisePtr<StatementBase> BlockScatter::createMemoryDeallocatingStatement(ReprisePtr<ExpressionBase> pointerExpression)
    {
        ReprisePtr<SubroutineReferenceExpression> rpFreeReferenceExpression(new SubroutineReferenceExpression(*m_pFreeDeclaration));
        ReprisePtr<SubroutineCallExpression> rpFreeCallExpression(new SubroutineCallExpression(rpFreeReferenceExpression.get()));

        rpFreeCallExpression->addArgument(pointerExpression->clone());
        ReprisePtr<StatementBase> result(new ExpressionStatement(rpFreeCallExpression.get()));

        return result;
    }
}
}
}
