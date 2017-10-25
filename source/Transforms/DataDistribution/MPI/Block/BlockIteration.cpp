#include "Transforms/DataDistribution/MPI/Block/BlockIteration.h"

#include "OPS_Core/Localization.h"
#include "Shared/DataShared.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/SubroutinesShared.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace Shared::ExpressionHelpers;
using namespace OPS::Transforms::MPIProducer;
using namespace OPS::Reprise;

BlockIteration::BlockIteration(
    BADDParametersFamily distributionParameters,
    BADDParameters generatorParameters,
    VariableDeclaration& targetArrayDeclaration,
    ForStatement& iterationStatement) :
        m_distributionParameters(distributionParameters),
        m_generatorParameters(generatorParameters),
        m_targetArrayDeclaration(targetArrayDeclaration),
        m_iterationStatement(iterationStatement)
{
    m_pFactory = new MPIProducerCFactory();

    m_pHelper = m_pFactory->createMPIHelper(Shared::getTranslationUnit(&m_iterationStatement));
    OPS_ASSERT(m_pHelper != NULL);

    m_pRankProducer = m_pFactory->createMPIRankProducer(Shared::getSubroutineDeclarationByStatement(&m_iterationStatement));
    OPS_ASSERT(m_pRankProducer != NULL);

    TypeBase* pArrayElementType = Shared::getArrayElementBasicType(&m_targetArrayDeclaration.getType());
    if(pArrayElementType == NULL) return;

    m_pArrayElementBasicType = pArrayElementType->cast_ptr<BasicType>();

    m_pMallocDeclaration = Shared::findSubroutineByName(Shared::getTranslationUnit(&m_iterationStatement), "malloc");
    m_pFreeDeclaration = Shared::findSubroutineByName(Shared::getTranslationUnit(&m_iterationStatement), "free");
}

BlockIteration::~BlockIteration()
{
    delete m_pHelper;
    delete m_pRankProducer;
    delete m_pFactory;
}

bool BlockIteration::analyseApplicability()
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

std::string BlockIteration::getErrorMessage()
{
    std::string error = "";

    for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
    {
        error = error + "\n" + *it;
    }

    return error;
}

void BlockIteration::makeTransformation()
{
    if(!m_analysisRerformed)
    {
        analyseApplicability();
    }
    OPS_ASSERT(m_errors.size() == 0);

    OPS_ASSERT(m_pNewArrayDeclaration != NULL);

    initRankDeclaration();
    OPS_ASSERT(m_pRankDeclaration != NULL);

    for(int dimentionNumber = m_distributionParameters.dims.size() - 1; dimentionNumber >=0; --dimentionNumber)
    {
        if(!isSendToLeftRequired(dimentionNumber) && !isSendToRightRequired(dimentionNumber))
        {
            continue;
        }

        ReprisePtr<BlockStatement> rpExchangeByDimentionBlock(new BlockStatement());
        m_iterationStatement.getBody().addLast(rpExchangeByDimentionBlock.get());

        initBlockIndexDeclarations(*rpExchangeByDimentionBlock);
        populateExchangeByDimensionBlock(*rpExchangeByDimentionBlock, dimentionNumber);
    }
}

void BlockIteration::initRankDeclaration()
{
    m_pRankDeclaration = MPIHelper::getRankDeclaration(*Shared::getSubroutineDeclarationByStatement(&m_iterationStatement));
    if(m_pRankDeclaration == NULL)
    {
        m_pRankProducer->makeTransformation();
        m_pRankDeclaration = MPIHelper::getRankDeclaration(*Shared::getSubroutineDeclarationByStatement(&m_iterationStatement));
    }
}

void BlockIteration::initBlockIndexDeclarations(BlockStatement& blockStatement)
{
    m_blockIndexDeclarations.resize(m_distributionParameters.dims.size());
    for(int dimensionNumber = 0; dimensionNumber < m_distributionParameters.dims.size(); ++dimensionNumber)
    {
        m_blockIndexDeclarations[dimensionNumber] = &Editing::createNewVariable(*BasicType::int32Type(), blockStatement, "_block_index_");
    }
}

void BlockIteration::populateExchangeByDimensionBlock(BlockStatement& exchangeByDimentionBlock, int dimensionNumber)
{
    ReprisePtr<BlockStatement> rpSendingBlock(new BlockStatement());
    ReprisePtr<BlockStatement> rpReceivingBlock(new BlockStatement());
    ReprisePtr<BlockStatement> rpSendBufferFreeBlock(new BlockStatement());

    exchangeByDimentionBlock.addLast(rpSendingBlock.get());
    exchangeByDimentionBlock.addLast(rpReceivingBlock.get());
    exchangeByDimentionBlock.addLast(rpSendBufferFreeBlock.get());

    VariableDeclaration* pStatusDeclaration = &Editing::createNewVariable(*m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_STATUS), exchangeByDimentionBlock, "_status_");

    if(isSendToLeftRequired(dimensionNumber))
    {
        VariableDeclaration* pLeftSendingBufferDeclaration = &Editing::createNewVariable(*(new PtrType(m_pArrayElementBasicType)), exchangeByDimentionBlock, "_LEFT_SEND_BUF_");
        VariableDeclaration* pRightReceivingBufferDeclaration = &Editing::createNewVariable(*(new PtrType(m_pArrayElementBasicType)), exchangeByDimentionBlock, "_RIGHT_RECV_BUF_");
        VariableDeclaration* pLeftSendRequestDeclaration = &Editing::createNewVariable(*m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_REQUEST), exchangeByDimentionBlock, "_left_send_request_");

        addSendStatements(*rpSendingBlock, *rpSendBufferFreeBlock, *pLeftSendingBufferDeclaration, *pLeftSendRequestDeclaration, *pStatusDeclaration, dimensionNumber, true);
        addReceiveStatements(*rpReceivingBlock, *pRightReceivingBufferDeclaration, *pStatusDeclaration, dimensionNumber, false);
    }

    if(isSendToRightRequired(dimensionNumber))
    {
        VariableDeclaration* pRightSendingBufferDeclaration = &Editing::createNewVariable(*(new PtrType(m_pArrayElementBasicType)), exchangeByDimentionBlock, "_RIGHT_SEND_BUF_");
        VariableDeclaration* pLeftReceivingBufferDeclaration = &Editing::createNewVariable(*(new PtrType(m_pArrayElementBasicType)), exchangeByDimentionBlock, "_LEFT_RECV_BUF_");
        VariableDeclaration* pRightSendRequestDeclaration = &Editing::createNewVariable(*m_pHelper->getMPITypeDeclaration(MPITypesHelper::MT_MPI_REQUEST), exchangeByDimentionBlock, "_right_send_request_");

        addSendStatements(*rpSendingBlock, *rpSendBufferFreeBlock, *pRightSendingBufferDeclaration, *pRightSendRequestDeclaration, *pStatusDeclaration, dimensionNumber, false);
        addReceiveStatements(*rpReceivingBlock, *pLeftReceivingBufferDeclaration, *pStatusDeclaration, dimensionNumber, true);
    }
}

bool BlockIteration::isSendToLeftRequired(int dimensionNumber)
{
    return m_distributionParameters.rightOverlapping[dimensionNumber] - m_generatorParameters.a[dimensionNumber] > 0;
}

bool BlockIteration::isSendToRightRequired(int dimensionNumber)
{
    return m_distributionParameters.leftOverlapping[dimensionNumber] + m_generatorParameters.a[dimensionNumber] > 0;
}

void BlockIteration::addSendStatements(BlockStatement& sendBlock, BlockStatement& bufferFreeBlock, VariableDeclaration& bufferDeclaration, VariableDeclaration& requestDeclaration, VariableDeclaration& statusDeclaration, int sendDimensionNumber, bool isLeft)
{
    std::vector<int> inits(m_distributionParameters.dims.size());
    std::vector<int> finals(m_distributionParameters.dims.size());
	for(size_t dimensionNumber = 0; dimensionNumber < m_distributionParameters.dims.size(); ++dimensionNumber)
    {
        if(dimensionNumber < sendDimensionNumber)
        {
            inits[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_generatorParameters.a[dimensionNumber];
            finals[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_generatorParameters.a[dimensionNumber] + m_distributionParameters.d[dimensionNumber];
        }
        else if(dimensionNumber == sendDimensionNumber)
        {
            if(isLeft)
            {
                inits[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_generatorParameters.a[dimensionNumber];
                finals[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_distributionParameters.rightOverlapping[dimensionNumber];
            }
            else
            {
                inits[dimensionNumber] = m_distributionParameters.d[dimensionNumber];
                finals[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_distributionParameters.d[dimensionNumber] + m_generatorParameters.a[dimensionNumber];
            }
        }
        else
        {
            inits[dimensionNumber] = 0;
            finals[dimensionNumber] = m_distributionParameters.d[dimensionNumber] + m_distributionParameters.leftOverlapping[dimensionNumber] + m_distributionParameters.rightOverlapping[dimensionNumber];
        }
    }
    int bufferSize = getBufferSize(inits, finals);

    ReprisePtr<ReferenceExpression> rpBufferReference(new ReferenceExpression(bufferDeclaration));
    sendBlock.addLast(createMemoryAllocatingStatement(rpBufferReference, bufferSize, *m_pArrayElementBasicType).get());
    addBufferCopyingStatemens(inits, finals, bufferDeclaration, sendBlock, true);
    sendBlock.addLast(buildISendStatement(
              bufferDeclaration,
              bufferSize,
              buildRankExpression(sendDimensionNumber, isLeft),
              requestDeclaration
        ).get());

    bufferFreeBlock.addLast(buildWaitStatement(requestDeclaration, statusDeclaration).get());
    bufferFreeBlock.addLast(createMemoryDeallocatingStatement(rpBufferReference).get());
}

void BlockIteration::addReceiveStatements(BlockStatement& receiveBlock, VariableDeclaration& bufferDeclaration, VariableDeclaration& statusDeclaration, int receiveDimensionNumber, bool isLeft)
{
    std::vector<int> inits(m_distributionParameters.dims.size());
    std::vector<int> finals(m_distributionParameters.dims.size());
	for(size_t dimensionNumber = 0; dimensionNumber < m_distributionParameters.dims.size(); ++dimensionNumber)
    {
        if(dimensionNumber < receiveDimensionNumber)
        {
            inits[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_generatorParameters.a[dimensionNumber];
            finals[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_generatorParameters.a[dimensionNumber] + m_distributionParameters.d[dimensionNumber];
        }
        else if(dimensionNumber == receiveDimensionNumber)
        {
            if(isLeft)
            {
                inits[dimensionNumber] = 0;
                finals[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_generatorParameters.a[dimensionNumber];
            }
            else
            {
                inits[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_distributionParameters.d[dimensionNumber] + m_generatorParameters.a[dimensionNumber];
                finals[dimensionNumber] = m_distributionParameters.leftOverlapping[dimensionNumber] + m_distributionParameters.d[dimensionNumber] + m_distributionParameters.rightOverlapping[dimensionNumber];
            }
        }
        else
        {
            inits[dimensionNumber] = 0;
            finals[dimensionNumber] = m_distributionParameters.d[dimensionNumber] + m_distributionParameters.leftOverlapping[dimensionNumber] + m_distributionParameters.rightOverlapping[dimensionNumber];
        }
    }

    int bufferSize = getBufferSize(inits, finals);

    ReprisePtr<ReferenceExpression> rpBufferReference(new ReferenceExpression(bufferDeclaration));
    receiveBlock.addLast(createMemoryAllocatingStatement(rpBufferReference, bufferSize, *m_pArrayElementBasicType).get());
    receiveBlock.addLast(buildRecvStatement(
            bufferDeclaration, bufferSize, buildRankExpression(receiveDimensionNumber, isLeft), statusDeclaration
        ).get());
    addBufferCopyingStatemens(inits, finals, bufferDeclaration, receiveBlock, false);

    receiveBlock.addLast(createMemoryDeallocatingStatement(rpBufferReference).get());
}

void BlockIteration::addBufferCopyingStatemens(const std::vector<int>& inits, const std::vector<int> finals, VariableDeclaration& bufferDeclaration, BlockStatement& blockToAdd, bool isSend)
{
    IntegerHelper ih(BasicType::BT_INT32);
    BlockStatement* pCurrentBlock = &blockToAdd;
	for(size_t dimensionNumber = 0; dimensionNumber < m_distributionParameters.dims.size(); ++dimensionNumber)
    {
        ReprisePtr<ExpressionBase> rpCounterReference(new ReferenceExpression(*m_blockIndexDeclarations[dimensionNumber]));
        ReprisePtr<ForStatement> rpForStatement(new ForStatement(
                &(op(rpCounterReference) R_AS ih(inits[dimensionNumber])),
                &(op(rpCounterReference) < ih(finals[dimensionNumber])),
                &(op(rpCounterReference) R_AS op(rpCounterReference) + ih(1))
            ));
        pCurrentBlock->addLast(rpForStatement.get());
        pCurrentBlock = &rpForStatement->getBody();
    }

    ReprisePtr<ExpressionBase> rpBufferIndex(&( *(new ReferenceExpression(*m_blockIndexDeclarations[0])) - ih(inits[0])));
    ReprisePtr<ExpressionBase> rpArrayIndex(new ReferenceExpression(*m_blockIndexDeclarations[0]));

	for(size_t dimensionNumber = 1; dimensionNumber < m_distributionParameters.dims.size(); ++dimensionNumber)
    {
        rpBufferIndex.reset(&(op(rpBufferIndex) * ih(finals[dimensionNumber] - inits[dimensionNumber]) + *(new ReferenceExpression(*m_blockIndexDeclarations[dimensionNumber])) - ih(inits[dimensionNumber])));
        rpArrayIndex.reset(&(
                op(rpArrayIndex)
                    *
                ih(m_distributionParameters.d[dimensionNumber] + m_distributionParameters.leftOverlapping[dimensionNumber] + m_distributionParameters.rightOverlapping[dimensionNumber])
                    +
                *(new ReferenceExpression(*m_blockIndexDeclarations[dimensionNumber]))
            ));
    }

    if(isSend)
    {
        pCurrentBlock->addLast(new ExpressionStatement(&(
                (*(new ReferenceExpression(bufferDeclaration)) R_BK (op(rpBufferIndex))) R_AS (*(new ReferenceExpression(*m_pNewArrayDeclaration)) R_BK (op(rpArrayIndex)))
            )));
    }
    else
    {
        pCurrentBlock->addLast(new ExpressionStatement(&(
                (*(new ReferenceExpression(*m_pNewArrayDeclaration)) R_BK (op(rpArrayIndex))) R_AS (*(new ReferenceExpression(bufferDeclaration)) R_BK (op(rpBufferIndex)))
            )));
    }
}

int BlockIteration::getBufferSize(const std::vector<int>& inits, const std::vector<int> finals)
{
    int result = 1;
	for(size_t dimensionNumber = 0; dimensionNumber < m_distributionParameters.dims.size(); ++dimensionNumber)
    {
        result = result * (finals[dimensionNumber] - inits[dimensionNumber]);
    }
    return result;
}

ReprisePtr<StatementBase> BlockIteration::createMemoryAllocatingStatement(ReprisePtr<ExpressionBase> pointerExpression, int sizeInElements, BasicType& typeOfElements)
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

ReprisePtr<StatementBase> BlockIteration::createMemoryDeallocatingStatement(ReprisePtr<ExpressionBase> pointerExpression)
{
    ReprisePtr<SubroutineReferenceExpression> rpFreeReferenceExpression(new SubroutineReferenceExpression(*m_pFreeDeclaration));
    ReprisePtr<SubroutineCallExpression> rpFreeCallExpression(new SubroutineCallExpression(rpFreeReferenceExpression.get()));

    rpFreeCallExpression->addArgument(pointerExpression->clone());
    ReprisePtr<StatementBase> result(new ExpressionStatement(rpFreeCallExpression.get()));

    return result;
}

ReprisePtr<StatementBase> BlockIteration::buildWaitStatement(VariableDeclaration& requestDeclaration, VariableDeclaration& statusDeclaration)
{
    return m_pHelper->createMPIWaitStatement(
            R_AD()*(new ReferenceExpression(requestDeclaration)),
            R_AD()*(new ReferenceExpression(statusDeclaration))
        );
}

ReprisePtr<StatementBase> BlockIteration::buildISendStatement(VariableDeclaration& bufferDeclaration, int bufferSize, ReprisePtr<ExpressionBase> rpRankExpression, VariableDeclaration& requestDeclaration)
{
    IntegerHelper ihCount(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_SEND), 1)->cast_to<BasicType>());
    IntegerHelper ihTag(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_SEND), 4)->cast_to<BasicType>());
    return m_pHelper->createMPIISendStatement(
            *(new ReferenceExpression(bufferDeclaration)),
            ihCount((sdword)bufferSize),
            *m_pHelper->createMPIBasicTypeCode(m_pFactory->createMPIBasicTypesHelper()->getMPIBasicTypeByRepriseBasicType(m_pArrayElementBasicType->getKind())),
            op(rpRankExpression),
            ihTag(0),
            *m_pHelper->createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD),
            *(new ReferenceExpression(requestDeclaration))
        );
}

ReprisePtr<StatementBase> BlockIteration::buildRecvStatement(VariableDeclaration& bufferDeclaration, int bufferSize, ReprisePtr<ExpressionBase> rpRankExpression, VariableDeclaration& statusDeclaration)
{
    IntegerHelper ihCount(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 1)->cast_to<BasicType>());
    IntegerHelper ihTag(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_RECV), 4)->cast_to<BasicType>());
    return m_pHelper->createMPIRecvStatement(
            *(new ReferenceExpression(bufferDeclaration)),
            ihCount((sdword)bufferSize),
            *m_pHelper->createMPIBasicTypeCode(m_pFactory->createMPIBasicTypesHelper()->getMPIBasicTypeByRepriseBasicType(m_pArrayElementBasicType->getKind())),
            op(rpRankExpression),
            ihTag(0),
            *m_pHelper->createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD),
            R_AD()*(new ReferenceExpression(statusDeclaration))
        );
}

ReprisePtr<ExpressionBase> BlockIteration::buildRankExpression(int dimensionNumber, bool isLeft)
{
    int blocksCount = 1;
    for(int i = m_distributionParameters.dims.size() - 1; i > dimensionNumber; --i)
    {
        blocksCount = blocksCount * m_distributionParameters.dims[dimensionNumber] / m_distributionParameters.d[dimensionNumber];
    }
    if(isLeft)
    {
        blocksCount = m_distributionParameters.P - blocksCount;
    }
    IntegerHelper ih(BasicType::BT_INT32);
    ReprisePtr<ExpressionBase> rpResult(&(
            (*(new ReferenceExpression(*m_pRankDeclaration)) + ih(blocksCount)) % ih(m_distributionParameters.P)
        ));
    return rpResult;
}

}
}
}
