
//		STL headers
#include <math.h>
#include <vector>

//		OPS headers
#include "Transforms/DataDistribution/MPI/Block/BlockScatter.h"
#include "Transforms/DataDistribution/MPI/Block/BlockGather.h"
#include "Transforms/DataDistribution/MPI/Block/BlockIteration.h"
#include "Transforms/DataDistribution/MPI/Block/BlockLoopNesting.h"
#include "Transforms/DataDistribution/MPI/Block/BlocksHelper.h"
#include "Transforms/MPI/MPIGlobalsProducer/MPIGlobalsProducer.h"
#include "Transforms/DataDistribution/BlockAffineDataDistribution.h"
#include "Transforms/DataDistribution/MPI/MPIDataDistributionGather.h"
#include "Transforms/DataDistribution/MPI/MPIDataDistributionScatter.h"
#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"
#include "Transforms/DataDistribution/MPI/MPISingleAccessAreaProducer.h"
#include "Transforms/DataDistribution/MPI/MPILoopNesting.h"
#include "Transforms/DataDistribution/MPI/MPIDataDistributionIteration.h"
#include "Transforms/DataDistribution/DataDistributionAnalizer.h"
#include "FrontTransforms/ExpressionSimplifier.h"
#include "Reprise/Reprise.h"
#include "Reprise/Canto/HirFTypes.h"
#include "Shared/SubroutinesShared.h"
#include "Shared/DataShared.h"
#include "Shared/LinearExpressions.h"
#include "Shared/ExpressionHelpers.h"
#include "OPS_Core/Localization.h"
#include "ReferencesFinder.h"


using namespace std;
using namespace OPS::Transforms::MPIProducer;
using namespace OPS::Shared;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Transforms;
using namespace OPS::TransformationsHub;
using namespace OPS::Shared::ExpressionHelpers;


namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
    BADD::BADD(): OPS::TransformationsHub::TransformBase()
    {
        ArgumentInfo firstArgInfo(ArgumentValue::StmtBlock, _TL("Block containing data to be distributed", "Блок, содержащий данные, которые нужно разместить"));
        this->m_argumentsInfo.push_back(firstArgInfo);
    }

    void BADD::makeTransformImpl(ProgramUnit* pProgram, const OPS::TransformationsHub::ArgumentValues& params)
    {
        if(pProgram == NULL || !getArgumentsInfo().validate(params))
        {
            throw BADDException(_TL("Invalid arguments.", "Некорректные параметры."));
        }

        std::list<std::string> errors;
        DataDistributionAnalizer analizer(*params[0].getAsBlock());
        if (analizer.analize())
        {
            // Инициализируем преобразования
            IMPIGlobalsProducer* pMPIGlobalsProducer = new MPIGlobalsProducer(Shared::getSubroutineDeclarationByStatement(params[0].getAsBlock()));

            DataDistributionAnalizer::SingleAccessAreaInfoContainer singleAccessAreaInfoContainer = analizer.getSingleAccessAreaInfoContainer();
            DataDistributionAnalizer::DistributionAreaInfoContainer distributionAreaInfoContainer = analizer.getDistributionAreaInfoContainer();
            DataDistributionAnalizer::ParallelLoopNestingInfoContainer parallelLoopNestingInfoContainer = analizer.getParallelLoopNestingInfoContainer();

            std::vector<IMPISingleAccessAreaProducer*> singleAccessAreaTransformsContainer(singleAccessAreaInfoContainer.size());
            std::vector<int>::size_type i = 0;
            for (DataDistributionAnalizer::SingleAccessAreaInfoContainer::iterator sait = singleAccessAreaInfoContainer.begin(); sait != singleAccessAreaInfoContainer.end(); ++sait)
            {
                IntegerHelper ih(BasicType::BT_INT32);
                ReprisePtr<ExpressionBase> rpShiftExpression(&ih(sait->getShift()));
                singleAccessAreaTransformsContainer[i] = new MPISingleAccessAreaProducer(sait->getStatement(), rpShiftExpression);

                SingleAccessAreaInfo::VariableDeclarationContainer generators = sait->getGenerators();
                DataDistributionAnalizer::DistributedDataInfoContainer distributedData = analizer.getDistributedDataInfoContainer();
                DataDistributionAnalizer::IgnoredDataInfoContainer ignoredData = analizer.getIgnoredDataInfoContainer();

                IMPISingleAccessAreaProducer::VariableDeclarationContainer container;

                for (SingleAccessAreaInfo::VariableDeclarationContainer::iterator vdcit = generators.begin(); vdcit != generators.end(); ++vdcit)
                {
                    VariableDeclaration* pDeclaration = *vdcit;
                    if(pDeclaration == NULL) continue;

                    bool declarationFound = false;

                    for (DataDistributionAnalizer::DistributedDataInfoContainer::iterator ddicit = distributedData.begin(); ddicit != distributedData.end(); ++ddicit)
                    {
                        if(pDeclaration == &(ddicit->getDeclaration()))
                        {
                            declarationFound = true;
                            break;
                        }
                    }
                    if(!declarationFound)
                    {
                        for (DataDistributionAnalizer::IgnoredDataInfoContainer::iterator idicit = ignoredData.begin(); idicit != ignoredData.end(); ++idicit)
                        {
                            if(pDeclaration == &(idicit->getDeclaration()))
                            {
                                declarationFound = true;
                                break;
                            }
                        }
                    }

                    if(!declarationFound)
                    {
                        container.push_back(pDeclaration);
                    }
                }

                singleAccessAreaTransformsContainer[i]->setDeclarationsToBcast(container);

                ++i;
            }
            std::vector<IMPIDataDistributionGather*> mpiDataDistributionGatherContainer(distributionAreaInfoContainer.size());
            std::vector<IMPIDataDistributionScatter*> mpiDataDistributionScatterContainer(distributionAreaInfoContainer.size());
            std::vector<BADDIndexModifier*> baddIndexModifierContainer(distributionAreaInfoContainer.size());
            std::vector<IMPIDataDistributionIteration*> mpiDataDistributionIterationContainer(distributionAreaInfoContainer.size());
            i = 0;
            for (DataDistributionAnalizer::DistributionAreaInfoContainer::iterator dait = distributionAreaInfoContainer.begin(); dait != distributionAreaInfoContainer.end(); ++dait)
            {
                if(dait->getPartDeclaration() == NULL)
                {
                    BADDIndexModifier* pBADDIndexModifier = new BADDIndexModifier();
                    pBADDIndexModifier->setParameters(dait->getDistributionParameters(), &dait->getDeclaration(), &dait->getStatement(), dait->getLeadingDimention(), pProgram->getUnit(0), true);
                    baddIndexModifierContainer[i] = pBADDIndexModifier;
                }
                else
                {
                    baddIndexModifierContainer[i] = NULL;
                }

                if(dait->isScatterRequired())
                {
                    mpiDataDistributionScatterContainer[i] = dait->isBlock() ?
                            (IMPIDataDistributionScatter*)(new BlockScatter(dait->getDistributionParametersFamily(), dait->getDeclaration(), dait->getStatement(), dait->getSourceNodeNumber()))
                              : (IMPIDataDistributionScatter*)(new MPIDataDistributionScatter(dait->getDistributionParametersFamily(), dait->getDeclaration(), dait->getStatement(), dait->getLeadingDimention(), dait->getSourceNodeNumber()));
                    if(dait->getPartDeclaration() != NULL)
                    {
                        mpiDataDistributionScatterContainer[i]->setNewArrayDeclaration(*dait->getPartDeclaration());
                    }
                }
                else
                {
                    mpiDataDistributionScatterContainer[i] = NULL;
                }

                if(dait->isGatherRequired())
                {
                    mpiDataDistributionGatherContainer[i] = dait->isBlock() ?
                            (IMPIDataDistributionGather*)(new BlockGather(dait->getDistributionParametersFamily(), dait->getGeneratorBADDParameters(), dait->getDeclaration(), dait->getStatement(), dait->getSourceNodeNumber()))
                            : (IMPIDataDistributionGather*)(new MPIDataDistributionGather(dait->getDistributionParametersFamily(), dait->getGeneratorBADDParameters(), dait->getDeclaration(), dait->getStatement(), dait->getLeadingDimention(), dait->getSourceNodeNumber()));
                    if(dait->getPartDeclaration() != NULL)
                    {
                        mpiDataDistributionGatherContainer[i]->setNewArrayDeclaration(*dait->getPartDeclaration());
                    }
                }
                else
                {
                    mpiDataDistributionGatherContainer[i] = NULL;
                }

                if(dait->isIterationalProcess())
                {
                    mpiDataDistributionIterationContainer[i] = dait->isBlock() ?
                            (IMPIDataDistributionIteration*)(new BlockIteration(dait->getDistributionParametersFamily(), dait->getGeneratorBADDParameters(), dait->getDeclaration(), dait->getStatement().cast_to<ForStatement>()))
                              : (IMPIDataDistributionIteration*)(new MPIDataDistributionIteration(dait->getDistributionParametersFamily(), dait->getGeneratorBADDParameters(), dait->getDeclaration(), dait->getStatement().cast_to<ForStatement>(), dait->getLeadingDimention()));
                    if(dait->getPartDeclaration() != NULL)
                    {
                        mpiDataDistributionIterationContainer[i]->setNewArrayDeclaration(*dait->getPartDeclaration());
                    }
                }
                else
                {
                    mpiDataDistributionIterationContainer[i] = NULL;
                }

                ++i;
            }
            std::vector<ITransformation*> mpiLoopNestingContainer(parallelLoopNestingInfoContainer.size());
            i = 0;
            for (DataDistributionAnalizer::ParallelLoopNestingInfoContainer::iterator plnit = parallelLoopNestingInfoContainer.begin(); plnit != parallelLoopNestingInfoContainer.end(); ++plnit)
            {
                mpiLoopNestingContainer[i] = plnit->isBlock() ?
                            (ITransformation*)(new BlockLoopNesting(plnit->getForStatement(), plnit->getBlockWidths(), plnit->getIterationsCount()))
                  : (ITransformation*)(new MPILoopNesting(plnit->getForStatement()));
            }

            // Проверка применимости

            if (!pMPIGlobalsProducer->analyseApplicability())
            {
                errors.push_back(pMPIGlobalsProducer->getErrorMessage());
            }

            for (i = 0; i < singleAccessAreaTransformsContainer.size(); ++i)
            {
                OPS_ASSERT(singleAccessAreaTransformsContainer[i] != NULL);

                if(!singleAccessAreaTransformsContainer[i]->analyseApplicability())
                {
                    errors.push_back(singleAccessAreaTransformsContainer[i]->getErrorMessage());
                }
            }
            for (i = 0; i < mpiDataDistributionGatherContainer.size(); ++i)
            {
                if(mpiDataDistributionGatherContainer[i] != NULL && !mpiDataDistributionGatherContainer[i]->analyseApplicability())
                {
                    errors.push_back(mpiDataDistributionGatherContainer[i]->getErrorMessage());
                }
                if(mpiDataDistributionScatterContainer[i] != NULL && !mpiDataDistributionScatterContainer[i]->analyseApplicability())
                {
                    errors.push_back(mpiDataDistributionScatterContainer[i]->getErrorMessage());
                }
                if(baddIndexModifierContainer[i] != NULL && !baddIndexModifierContainer[i]->isValid())
                {
                    errors.push_back(_TL("Index modifier is not applicable", "Преобразование индексов неприменимо"));
                }
                if(mpiDataDistributionIterationContainer[i] != NULL && !mpiDataDistributionIterationContainer[i]->analyseApplicability())
                {	
                    errors.push_back(mpiDataDistributionIterationContainer[i]->getErrorMessage());
                }
            }
            for (i = 0; i < mpiLoopNestingContainer.size(); ++i)
            {
                OPS_ASSERT(mpiLoopNestingContainer[i] != NULL);

                if(!mpiLoopNestingContainer[i]->analyseApplicability())
                {
                    errors.push_back(mpiLoopNestingContainer[i]->getErrorMessage());
                }
            }

            if(errors.size() == 0)
            {
                // Применение преобразований
                for (i = 0; i < singleAccessAreaTransformsContainer.size(); ++i)
                {
                    singleAccessAreaTransformsContainer[i]->makeTransformation();
                }
                for (i = 0; i < mpiDataDistributionGatherContainer.size(); ++i)
                {
                    VariableDeclaration* pPartArrayDeclaration = NULL;
                    if(baddIndexModifierContainer[i] != NULL)
                    {
                        baddIndexModifierContainer[i]->changeIndexes();
                        pPartArrayDeclaration = baddIndexModifierContainer[i]->getNewArrayDeclaration();
                    }

                    if (mpiDataDistributionIterationContainer[i] != NULL)
                    {
                        if(baddIndexModifierContainer[i] != NULL)
                        {
                            mpiDataDistributionIterationContainer[i]->setNewArrayDeclaration(*pPartArrayDeclaration);
                        }
                        mpiDataDistributionIterationContainer[i]->makeTransformation();
                    }

                    if(mpiDataDistributionGatherContainer[i] != NULL)
                    {
                        if(baddIndexModifierContainer[i] != NULL)
                        {
                            mpiDataDistributionGatherContainer[i]->setNewArrayDeclaration(*pPartArrayDeclaration);
                        }
                        mpiDataDistributionGatherContainer[i]->makeTransformation();
                    }

                    if(mpiDataDistributionScatterContainer[i] != NULL)
                    {
                        if(baddIndexModifierContainer[i] != NULL)
                        {
                            mpiDataDistributionScatterContainer[i]->setNewArrayDeclaration(*pPartArrayDeclaration);
                        }
                        mpiDataDistributionScatterContainer[i]->makeTransformation();
                    }
                }
                for (i = 0; i < mpiLoopNestingContainer.size(); ++i)
                {
                    mpiLoopNestingContainer[i]->makeTransformation();
                }
                pMPIGlobalsProducer->makeTransformation();
            }

            // Освобождение памяти
            delete pMPIGlobalsProducer;
            for (i = 0; i < singleAccessAreaTransformsContainer.size(); ++i)
            {
                delete singleAccessAreaTransformsContainer[i];
            }
            for (i = 0; i < mpiDataDistributionGatherContainer.size(); ++i)
            {
                delete mpiDataDistributionGatherContainer[i];
                delete mpiDataDistributionScatterContainer[i];
                delete baddIndexModifierContainer[i];
                delete mpiDataDistributionIterationContainer[i];
            }
            for (i = 0; i < mpiLoopNestingContainer.size(); ++i)
            {
                delete mpiLoopNestingContainer[i];
            }
        }
        else
        {
            errors = analizer.getErrors();
        }

        if (errors.size() > 0)
        {
            throw BADDException(errors.front());
        }
    }


//
//		DataDistributionIndexModifier
//

void BADDIndexModifier::setParameters(BADDParameters distrArgs, 
    OPS::Reprise::VariableDeclaration* arrayDeclaration, 
    OPS::Reprise::StatementBase* changingIndexesBlock,
    int fDim, 
    OPS::Reprise::TranslationUnit& translationUnit,
    bool saveOldExprInNode)
{
    m_distrArgs = distrArgs;

    m_arrayDeclaration = arrayDeclaration;
    m_changingIndexesStmtList.push_back(changingIndexesBlock);	
    m_fDim = fDim;
    m_translationUnit = &translationUnit;
    m_saveOldExprInNode = saveOldExprInNode;
}

void BADDIndexModifier::setParameters2(BADDParameters& distrArgs, 
    OPS::Reprise::VariableDeclaration* arrayDeclaration, 
    std::list<OPS::Reprise::StatementBase*> changingIndexesStmtList,
    int fDim, 
    OPS::Reprise::TranslationUnit& translationUnit)
{
    m_distrArgs = distrArgs;

    m_arrayDeclaration = arrayDeclaration;
    m_changingIndexesStmtList = changingIndexesStmtList;
    m_fDim = fDim;
    m_translationUnit = &translationUnit;
    m_saveOldExprInNode = false;
}

bool BADDIndexModifier::isValid()
{
    if (! (m_arrayDeclaration != 0) ) 
        return false;
    if (! (m_distrArgs.P > 0) )
        return false;
    if (! (m_distrArgs.d.size() + 1 == m_distrArgs.s.size()) )
        return false;
    if (! (m_arrayDeclaration->hasDefinedBlock() == true) )
        return false;
    if (!m_arrayDeclaration->getType().is_a<PtrType>() && !m_arrayDeclaration->getType().is_a<ArrayType>())
        return false;
    
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
        if (! (m_distrArgs.d[i] > 0) )
            return false;		
    
    return true;
}

std::list<OPS::Reprise::BasicCallExpression*> BADDIndexModifier::getAllDistribingArrayReferences()
{
    ArrayAccessExpressionsFinder refFinder(m_arrayDeclaration->getName(), m_distrArgs.dims.size());	
    list<StatementBase*>::iterator it;
    for (it = m_changingIndexesStmtList.begin(); it != m_changingIndexesStmtList.end(); ++it)
        (*it)->accept(refFinder);

    return refFinder.refList;
}

std::list<OPS::Reprise::BasicCallExpression*> BADDIndexModifier::getGeneratorsDistribingArrayReferences()
{
    ArrayAccessExpressionsFinder refFinder(m_arrayDeclaration->getName(), m_distrArgs.dims.size());	
    list<StatementBase*>::iterator it;
    for (it = m_changingIndexesStmtList.begin(); it != m_changingIndexesStmtList.end(); ++it)
        (*it)->accept(refFinder);

    return refFinder.generatorsList;
}

std::list<OPS::Reprise::BasicCallExpression*> BADDIndexModifier::getUsingsDistribingArrayReferences()
{
    ArrayAccessExpressionsFinder refFinder(m_arrayDeclaration->getName(), m_distrArgs.dims.size());	
    list<StatementBase*>::iterator it;
    for (it = m_changingIndexesStmtList.begin(); it != m_changingIndexesStmtList.end(); ++it)
        (*it)->accept(refFinder);

    return refFinder.usingsList;
}

BADDParameters BADDIndexModifier::getDistrParametersForArrayReference(OPS::Reprise::BasicCallExpression* arrayAccessExpr)
{
    BADDParameters result = m_distrArgs;

    Occurrence occurence(arrayAccessExpr);
    for (size_t i = 0; i < m_distrArgs.dims.size(); i++)
    {
        result.a[i] = 0;
        ExpressionBase* freeCoef = occurence.getFreeCoefficient(i).get();		
        if (freeCoef == 0)
            continue;

        ExpressionBase* simplifiedExpr = OPS::ExpressionSimplifier::Simplifier().simplify(freeCoef);
        if (simplifiedExpr == 0 || simplifiedExpr->is_a<StrictLiteralExpression>() == false)
            continue;

        StrictLiteralExpression* simpLiteral = simplifiedExpr->cast_ptr<StrictLiteralExpression>();
        if (simpLiteral->isInteger() == false)
            continue;

        result.a[i] = simpLiteral->getInt32();
    }

    return result;
}

bool BADDIndexModifier::getBAADParametersFamily(BADDParametersFamily& result)
{
    std::list<BasicCallExpression*> allDistributingArrayReferences = getAllDistribingArrayReferences();
    std::list<BasicCallExpression*> generatorsDistributingArrayReferences = getGeneratorsDistribingArrayReferences();

    BADDParametersFamily::BADDParametersContainer parameters(allDistributingArrayReferences.size());
    std::list<BasicCallExpression*> allDistrArrayReferences = getAllDistribingArrayReferences();
    
    int i = 0;
    for(std::list<BasicCallExpression*>::iterator it = allDistrArrayReferences.begin(); it != allDistrArrayReferences.end(); ++it)
    {
        parameters[i] = getDistrParametersForArrayReference(*it);
        if(!parameters[i].isValid())
        {
            return false;
        }
        parameters[i].normalize();

        i++;
    }

    BADDParametersFamily distrParamsFamily(parameters);
    result = distrParamsFamily;	
    return true;
}

void BADDIndexModifier::changeIndexes()
{
    if (isValid() == false)
        return;

    getBAADParametersFamily(m_distrParamsFamily);

    
    //		создаем новый массив в который будем пересылать данные
    m_newArrayDeclaration = internal_createNewArrayDeclaration(m_arrayDeclaration);

    //		создаем оператор выделения памяти для нового массива
    StatementBase* newArrayAllocationStmt = createNewArrayAllocationStatement();

    //		... и добавляем его в блок 
    m_arrayDeclaration->getDefinedBlock().addFirst(newArrayAllocationStmt);
    
    list<BasicCallExpression*> refList = getAllDistribingArrayReferences();
    for (list<BasicCallExpression*>::iterator iRef = refList.begin(); iRef != refList.end(); ++iRef)
    {
        ReprisePtr<RepriseBase> rpOldExpr((*iRef)->clone());
        BasicCallExpression* nextRef = *iRef;
        
        //		находим параметры распределения для конкретного вхождения
        BADDParameters distrArgs = getDistrParametersForArrayReference(nextRef);
                
        //		вытаскиваем мультииндекс вхождения для его последующего преобразования
        RepriseList<ExpressionBase> indexExprs;
        for (int i = 1; i < nextRef->getArgumentCount(); i++)
            indexExprs.add(&nextRef->getArgument(i));

        // заменяем X на XX
        nextRef->removeArguments();
        nextRef->addArgument(new ReferenceExpression(*m_newArrayDeclaration));

        ExpressionBase* newIndexExpr = tryGetOptimizedLocalIndexExprByGlobalIndexExr(indexExprs, distrArgs);
        if (newIndexExpr == 0)
            newIndexExpr = getLocalIndexExprByGlobalIndexExr(indexExprs, distrArgs);
        nextRef->addArgument(newIndexExpr);

        if(m_saveOldExprInNode)
        {
            nextRef->setNote(BlocksHelper::OLD_EXPRESSION_NOTE_NAME, Note::newReprise(rpOldExpr));
            nextRef->setNote(BlocksHelper::PARAMETERS_FAMILY_NOTE_NAME, Note::newString(m_distrParamsFamily.toString()));
        }
    }
}

VariableDeclaration* BADDIndexModifier::getNewArrayDeclaration()
{
    return m_newArrayDeclaration;
}

StatementBase* BADDIndexModifier::createNewArrayAllocationStatement()
{
    TypeBase* arrayBasicType = getArrayElementBasicType(&m_arrayDeclaration->getType());
    if (arrayBasicType->is_a<BasicType>() == false)
        throw RuntimeError("Basic type expected");

    int elemSize = arrayBasicType->cast_to<BasicType>().getSizeOf();
    int newArrayElemCount = internal_getNewArrayLength();
    
    SubroutineDeclaration* mallocDeclaration = OPS::Shared::findSubroutineByName(m_translationUnit, "malloc");
    if (mallocDeclaration == 0)
        throw RuntimeError("malloc function wasn't found!");
    
    SubroutineCallExpression* mallocCallExpr = new SubroutineCallExpression(
        new SubroutineReferenceExpression(*mallocDeclaration));			
    
    mallocCallExpr->addArgument(StrictLiteralExpression::createInt32(newArrayElemCount * elemSize));
    
    BasicCallExpression* mallocAssignExpr = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN);
    mallocAssignExpr->addArgument(new ReferenceExpression(*m_newArrayDeclaration));
    mallocAssignExpr->addArgument(new TypeCastExpression(new PtrType(arrayBasicType), mallocCallExpr));
    
    return new ExpressionStatement(mallocAssignExpr);
}

ExpressionBase* BADDIndexModifier::tryGetOptimizedLocalIndexExprByGlobalIndexExr(RepriseList<ExpressionBase>& globalIndexExprList, 
    BADDParameters distrArg)
{
    ExpressionBase* result = 0;
    if (distrArg.P > 1)
        return 0;
    

    int dd = 1;
    for (size_t i = 0; i < distrArg.d.size(); i++)
        dd *= distrArg.d[i];

    // TODO fix that and add checks
    for (int i = 0; i < globalIndexExprList.size(); i++)	
    {
        if (globalIndexExprList[i].is_a<BasicCallExpression>())
        {
            BasicCallExpression* expr = globalIndexExprList[i].cast_ptr<BasicCallExpression>();
            if (expr->getKind() == BasicCallExpression::BCK_BINARY_PLUS && expr->getArgumentCount() == 2)
            {
                ExpressionBase* leftExpr = &expr->getArgument(0);
                ExpressionBase* rightExpr = &expr->getArgument(1);

                if (leftExpr->is_a<BasicCallExpression>())
                {
                    BasicCallExpression* leftExprAsBCKExpr = leftExpr->cast_ptr<BasicCallExpression>();
                    if (leftExprAsBCKExpr->getKind() == BasicCallExpression::BCK_MULTIPLY && leftExprAsBCKExpr->getArgumentCount() == 2)
                    {
                        ExpressionBase* leftOfLeftExpr = &leftExprAsBCKExpr->getArgument(0);
                        ExpressionBase* rightOfLeftExpr = &leftExprAsBCKExpr->getArgument(1);
                        
                        ExpressionBase* simplifiedRightOfLeftExpr = OPS::ExpressionSimplifier::Simplifier().simplify(rightOfLeftExpr);
                        if (simplifiedRightOfLeftExpr == 0 || simplifiedRightOfLeftExpr->is_a<StrictLiteralExpression>() == false)
                            return 0;
                    
                        int value = simplifiedRightOfLeftExpr->cast_ptr<StrictLiteralExpression>()->getInt32();
                        if (value != distrArg.d[i])
                            return 0;

                        // УРАААА!!! Почти доказали корректность! На этом остановимся
                        
                        ExpressionBase* additionalExpr = 0;
                        
                        IntegerHelper ih(BasicType::BT_INT32);
                        int koef1 = dd;
                        int koef2 = 1;
                        for (size_t j = 0; j < distrArg.dims.size() - i - 1; j++)
                        {
                            int ind = distrArg.dims.size() - j - 1;
                            koef1 *= ceil((double)distrArg.dims[ind]/distrArg.d[ind]);
                            koef2 = distrArg.d[ind];
                        }
                        
                        ExpressionBase* buf1 = &( (*(leftOfLeftExpr) * (ih(koef1))) );
                        ExpressionBase* buf2 = &( (*(rightExpr) * (ih(koef2))) );
                        additionalExpr = &( (*(buf1) + (*(buf2))) );
                        if (result == 0)
                            result = additionalExpr;
                        else 
                        {
                            BasicCallExpression* buf = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
                            buf->addArgument(result);
                            buf->addArgument(additionalExpr);
                            result = buf;
                        }
                    }
                    else 
                        return 0;
                }
                else
                    return 0;

            }
            else
                return 0;
        }
        else
            return 0;
    }
    return result;
}

ExpressionBase* BADDIndexModifier::getLocalIndexExprByGlobalIndexExr(RepriseList<ExpressionBase>& globalIndexExprList,
    BADDParameters distrArg)
{	
    if (distrArg.P == 1)
        return getLocalIndexExprByGlobalIndexExrForOneProcessor(globalIndexExprList, distrArg);

    vector<int> k = internal_getK(); 

    // количество блоков в клетке
    int blockCount = m_distrParamsFamily.getCellWidthInBlocks(m_fDim);

    // размер блока
    int blockSize = m_distrParamsFamily.getElementsInBlockCount();
    
    ExpressionBase* cellIndexExpr = 0;
    ExpressionBase* blockIndexExpr = 0;
    ExpressionBase* indexExpr = 0;

    // считаем индекс клетки
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
    {		
        int b = 1;
        for (size_t j = i + 1; j < m_distrArgs.d.size(); j++)
        {
            b = b * m_distrParamsFamily.getWidthInCells(j);
        }
                    
        ExpressionBase* bufferExpr = &globalIndexExprList[i];
        if (distrArg.a[i] != 0)
        {
            ExpressionBase* bufferExpr_2 = bufferExpr;

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
            bufferExpr_3->addArgument(bufferExpr_2);
            bufferExpr_3->addArgument(StrictLiteralExpression::createInt32(distrArg.a[i]));

            bufferExpr = bufferExpr_3;
        }
        {
            StrictLiteralExpression* bufferExpr_2 = StrictLiteralExpression::createInt32(m_distrParamsFamily.getCellWidthInBlocks(i) *  k[i]*m_distrArgs.d[i]);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_DIVISION);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (b != 1)
        {
            StrictLiteralExpression* bufferExpr_2 = new StrictLiteralExpression(BasicType::BT_INT32);
            bufferExpr_2->setInt32(b);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (cellIndexExpr == 0)
            cellIndexExpr = bufferExpr;
        else
        {
            BasicCallExpression* bufferExpr_2 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
            bufferExpr_2->addArgument(cellIndexExpr);
            bufferExpr_2->addArgument(bufferExpr);

            cellIndexExpr = bufferExpr_2;				
        }
    }


    // потом находим индекс блока-проекции в клетке

    {
        if (m_distrArgs.dims.size() == 1)
        {
            IntegerHelper ih(BasicType::BT_INT32);
            blockIndexExpr = &ih(0);
        } 
        else
        {
            if (m_distrArgs.d[m_fDim] != 0)
            {
                blockIndexExpr = &globalIndexExprList[m_fDim];
                if (distrArg.a[m_fDim] != 0)
                {
                    ExpressionBase* bufferExpr_2 = blockIndexExpr;

                    BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
                    bufferExpr_3->addArgument(bufferExpr_2);
                    bufferExpr_3->addArgument(StrictLiteralExpression::createInt32(distrArg.a[m_fDim]));

                    blockIndexExpr = bufferExpr_3;
                }

                StrictLiteralExpression* bufferExpr_1 = StrictLiteralExpression::createInt32(m_distrArgs.d[m_fDim]);

                BasicCallExpression* bufferExpr_2 = new BasicCallExpression(BasicCallExpression::BCK_DIVISION);
                bufferExpr_2->addArgument(blockIndexExpr);
                bufferExpr_2->addArgument(bufferExpr_1);

                blockIndexExpr = bufferExpr_2;

                {
                    bufferExpr_1 = StrictLiteralExpression::createInt32(k[m_fDim]);

                    bufferExpr_2 = new BasicCallExpression(BasicCallExpression::BCK_INTEGER_MOD);
                    bufferExpr_2->addArgument(blockIndexExpr);
                    bufferExpr_2->addArgument(bufferExpr_1);

                    blockIndexExpr = bufferExpr_2;
                }
            }
        }				
        //blockIndex = (globalIndex[fDim]/d[fDim]) % k[fDim];
    }


    // находим index
    indexExpr = 0;
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
    {
        int b = 1;
        for (size_t j = i+1; j < m_distrArgs.d.size(); j++)
            b *= m_distrArgs.d[j] + m_distrParamsFamily.leftOverlapping[j] + m_distrParamsFamily.rightOverlapping[j];
        
        ExpressionBase* bufferExpr = &globalIndexExprList[i];
        if (distrArg.a[i] != 0)
        {
            ExpressionBase* bufferExpr_2 = bufferExpr;

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
            bufferExpr_3->addArgument(bufferExpr_2);
            bufferExpr_3->addArgument(StrictLiteralExpression::createInt32(distrArg.a[i]));

            bufferExpr = bufferExpr_3;
        }

        
        {
            StrictLiteralExpression* bufferExpr_2 = new StrictLiteralExpression(BasicType::BT_INT32);
            bufferExpr_2->setInt32(m_distrArgs.d[i]);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_INTEGER_MOD);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (m_distrParamsFamily.leftOverlapping[i] != 0)
        {
            StrictLiteralExpression* bufferExpr_2 = new StrictLiteralExpression(BasicType::BT_INT32);
            bufferExpr_2->setInt32(m_distrParamsFamily.leftOverlapping[i]);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (distrArg.a[i] != 0)
        {
            StrictLiteralExpression* bufferExpr_2 = new StrictLiteralExpression(BasicType::BT_INT32);
            bufferExpr_2->setInt32(distrArg.a[i]);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (b != 1)
        {
            StrictLiteralExpression* bufferExpr_2 = new StrictLiteralExpression(BasicType::BT_INT32);
            bufferExpr_2->setInt32(b);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (indexExpr == 0)
            indexExpr = bufferExpr;
        else
        {
            BasicCallExpression* bufferExpr_2 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
            bufferExpr_2->addArgument(indexExpr);
            bufferExpr_2->addArgument(bufferExpr);

            indexExpr = bufferExpr_2;				
        }
        //index += (globalIndex[i] % d[i]) * b;
    }

    // результирующая сумма
    ExpressionBase* resultExpr = 0;
    
    if (cellIndexExpr != 0)
    {
        ExpressionBase* expr1 =  cellIndexExpr;
        ExpressionBase* expr2 = StrictLiteralExpression::createInt32(blockCount);
        
        BasicCallExpression* expr3 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
        expr3->addArgument(expr1);
        expr3->addArgument(expr2);

        resultExpr = expr3;
    }

    if (blockIndexExpr != 0)
    {
        ExpressionBase* expr1 = resultExpr;
        ExpressionBase* expr2 = blockIndexExpr;
        BasicCallExpression* expr3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);

        expr3->addArgument(expr1);
        expr3->addArgument(expr2);

        resultExpr = expr3;
    }

    {
        ExpressionBase* expr1 = resultExpr;
        ExpressionBase* expr2 = StrictLiteralExpression::createInt32(blockSize);
        BasicCallExpression* expr3 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);

        expr3->addArgument(expr1);
        expr3->addArgument(expr2);

        resultExpr = expr3;
    }

    if (indexExpr != 0)
    {
        ExpressionBase* expr1 = resultExpr;
        ExpressionBase* expr2 = indexExpr;
        BasicCallExpression* expr3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);

        expr3->addArgument(expr1);
        expr3->addArgument(expr2);

        resultExpr = expr3;
    }

    return resultExpr;
}



ExpressionBase* BADDIndexModifier::getLocalIndexExprByGlobalIndexExrForOneProcessor(RepriseList<ExpressionBase>& globalIndexExprList,
    BADDParameters distrArg)
{	
    vector<int> k = internal_getK(); 

    // количество блоков в клетке
    int blockCount =  k[m_fDim];

    // размер блока
    int blockSize = 1;
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
        blockSize *= m_distrArgs.d[i] + m_distrParamsFamily.leftOverlapping[i] + m_distrParamsFamily.rightOverlapping[i];
    
    ExpressionBase* cellIndexExpr = 0;
    ExpressionBase* blockIndexExpr = 0;
    ExpressionBase* indexExpr = 0;

    // считаем индекс клетки
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
    {		
        int b = 1;
        for (size_t j = i+1; j < m_distrArgs.d.size(); j++)
            b = b * ceil((double)m_distrArgs.dims[j]/(k[j]*m_distrArgs.d[j]));
                    
        ExpressionBase* bufferExpr = &globalIndexExprList[i];
        //if (distrArg.a[i] != 0)
        //{
        //	ExpressionBase* bufferExpr_2 = bufferExpr;

        //	BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
        //	bufferExpr_3->addArgument(bufferExpr_2);
        //	bufferExpr_3->addArgument(StrictLiteralExpression::createInt32(distrArg.a[i]));

        //	bufferExpr = bufferExpr_3;
        //}
        {
            StrictLiteralExpression* bufferExpr_2 = StrictLiteralExpression::createInt32(k[i]*m_distrArgs.d[i]);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_DIVISION);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (b != 1)
        {
            StrictLiteralExpression* bufferExpr_2 = new StrictLiteralExpression(BasicType::BT_INT32);
            bufferExpr_2->setInt32(b);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (cellIndexExpr == 0)
            cellIndexExpr = bufferExpr;
        else
        {
            BasicCallExpression* bufferExpr_2 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
            bufferExpr_2->addArgument(cellIndexExpr);
            bufferExpr_2->addArgument(bufferExpr);

            cellIndexExpr = bufferExpr_2;				
        }
        
    }


    // потом находим индекс блока-проекции в клетке

    {
        if (m_distrArgs.d[m_fDim] != 0)
        {
            blockIndexExpr = &globalIndexExprList[m_fDim];
            //if (distrArg.a[m_fDim] != 0)
            //{
            //	ExpressionBase* bufferExpr_2 = blockIndexExpr;

            //	BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
            //	bufferExpr_3->addArgument(bufferExpr_2);
            //	bufferExpr_3->addArgument(StrictLiteralExpression::createInt32(distrArg.a[m_fDim]));

            //	blockIndexExpr = bufferExpr_3;
            //}

            StrictLiteralExpression* bufferExpr_1 = StrictLiteralExpression::createInt32(m_distrArgs.d[m_fDim]);
            
            BasicCallExpression* bufferExpr_2 = new BasicCallExpression(BasicCallExpression::BCK_DIVISION);
            bufferExpr_2->addArgument(blockIndexExpr);
            bufferExpr_2->addArgument(bufferExpr_1);

            blockIndexExpr = bufferExpr_2;
            
            {
                bufferExpr_1 = StrictLiteralExpression::createInt32(k[m_fDim]);

                bufferExpr_2 = new BasicCallExpression(BasicCallExpression::BCK_INTEGER_MOD);
                bufferExpr_2->addArgument(blockIndexExpr);
                bufferExpr_2->addArgument(bufferExpr_1);

                blockIndexExpr = bufferExpr_2;
            }
        }						
        //blockIndex = (globalIndex[fDim]/d[fDim]) % k[fDim];
    }


    // находим index
    indexExpr = 0;
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
    {
        int b = 1;
        for (size_t j = i+1; j < m_distrArgs.d.size(); j++)
            b *= m_distrArgs.d[j] + m_distrParamsFamily.leftOverlapping[j] + m_distrParamsFamily.rightOverlapping[j];
        
        ExpressionBase* bufferExpr = &globalIndexExprList[i];
        //if (distrArg.a[i] != 0)
        //{
        //	ExpressionBase* bufferExpr_2 = bufferExpr;

        //	BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
        //	bufferExpr_3->addArgument(bufferExpr_2);
        //	bufferExpr_3->addArgument(StrictLiteralExpression::createInt32(distrArg.a[i]));

        //	bufferExpr = bufferExpr_3;
        //}

        
        {
            StrictLiteralExpression* bufferExpr_2 = new StrictLiteralExpression(BasicType::BT_INT32);
            bufferExpr_2->setInt32(m_distrArgs.d[i] + m_distrArgs.a[i]);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_INTEGER_MOD);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (b != 1)
        {
            StrictLiteralExpression* bufferExpr_2 = new StrictLiteralExpression(BasicType::BT_INT32);
            bufferExpr_2->setInt32(b);

            BasicCallExpression* bufferExpr_3 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
            bufferExpr_3->addArgument(bufferExpr);
            bufferExpr_3->addArgument(bufferExpr_2);

            bufferExpr = bufferExpr_3;
        }

        if (indexExpr == 0)
            indexExpr = bufferExpr;
        else
        {
            BasicCallExpression* bufferExpr_2 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
            bufferExpr_2->addArgument(indexExpr);
            bufferExpr_2->addArgument(bufferExpr);

            indexExpr = bufferExpr_2;				
        }
        //index += (globalIndex[i] % d[i]) * b;
    }

    // результирующая сумма
    ExpressionBase* resultExpr = 0;
    
    if (cellIndexExpr != 0)
    {
        ExpressionBase* expr1 =  cellIndexExpr;
        ExpressionBase* expr2 = StrictLiteralExpression::createInt32(blockCount);
        
        BasicCallExpression* expr3 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
        expr3->addArgument(expr1);
        expr3->addArgument(expr2);

        resultExpr = expr3;
    }

    if (blockIndexExpr != 0)
    {
        ExpressionBase* expr1 = resultExpr;
        ExpressionBase* expr2 = blockIndexExpr;
        BasicCallExpression* expr3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);

        expr3->addArgument(expr1);
        expr3->addArgument(expr2);

        resultExpr = expr3;
    }

    {
        ExpressionBase* expr1 = resultExpr;
        ExpressionBase* expr2 = StrictLiteralExpression::createInt32(blockSize);
        BasicCallExpression* expr3 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);

        expr3->addArgument(expr1);
        expr3->addArgument(expr2);

        resultExpr = expr3;
    }

    if (indexExpr != 0)
    {
        ExpressionBase* expr1 = resultExpr;
        ExpressionBase* expr2 = indexExpr;
        BasicCallExpression* expr3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);

        expr3->addArgument(expr1);
        expr3->addArgument(expr2);

        resultExpr = expr3;
    }

    return resultExpr;
}

//
//			BADDIndexModifier Private methods
//

vector<int> BADDIndexModifier::internal_getK()
{
    vector<int> k;
    k.resize(m_distrArgs.d.size());
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
    {
        int val = 1;
        int t = m_distrArgs.s[i] % m_distrArgs.P;
        while (t != 0) {
            val++;
            t = (t + m_distrArgs.s[i]) % m_distrArgs.P;
        } 
        k[i] = val;
    }
    return k;
}

int BADDIndexModifier::internal_getProcIdByGlobalIndex(vector<int> globalIndex)
{
    int pp = m_distrArgs.s[m_distrArgs.d.size()];
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
        pp += globalIndex[i]/m_distrArgs.d[i]*m_distrArgs.s[i];
    return pp % m_distrArgs.P;
}

int BADDIndexModifier::internal_getNewArrayLength()
{


    vector<int> k = internal_getK();

    // считаем количество клеток
    int cellCount = 1;
    for (size_t i = 0; i < m_distrArgs.dims.size(); i++)
        cellCount *= ceil(((double)m_distrArgs.dims[i]) / (k[i] * m_distrArgs.d[i]));

    // количество блоков в клетке
    int blockCount = m_distrArgs.dims.size() == 1 ? 1 : k[m_fDim];

    // размер блока
    int blockSize = 1;
    for (size_t i = 0; i < m_distrArgs.d.size(); i++)
        blockSize *= (m_distrArgs.d[i] + m_distrParamsFamily.leftOverlapping[i] + m_distrParamsFamily.rightOverlapping[i]);

    //	находим размер результируещего массива
    int result = cellCount * blockCount * blockSize;
    return result;	
}

VariableDeclaration* BADDIndexModifier::internal_createNewArrayDeclaration(OPS::Reprise::VariableDeclaration* arrayDeclaration)
{	
    TypeBase& arrayType = arrayDeclaration->getType();
    TypeBase* newArrayBasicType = getArrayElementBasicType(&arrayType);
    PtrType* newArrayType = new PtrType(newArrayBasicType);

    return &OPS::Reprise::Editing::createNewVariable(*newArrayType, 
        arrayDeclaration->getDefinedBlock(), arrayDeclaration->getName());
}

//int BADDIndexModifier::internal_getLocalIndexByGlobalIndex(vector<int> globalIndex)
//{	
//
//	int i, j;
//	int cellIndex;
//	int blockCount;
//	int blockIndex; 
//	int blockSize;
//	int index;
//	int result;
//
//	vector<int> k = internal_getK(); 
//	int dims_count = m_distrArgs.d.size();
//
//	// считаем индекс клетки
//	cellIndex = 0;
//	for (i = 0; i < dims_count; i++)
//	{
//		int b = 1;
//		for (j = i+1; j < dims_count; j++)
//			b = b * ceil((double)m_distrArgs.dims[j]/(k[j]*m_distrArgs.d[j]));
//		cellIndex += ((globalIndex[i]-<a[i]>)/(k[i]*m_distrArgs.d[i])) * b;
//	}
//
//	// количество блоков в клетке
//	blockCount = k[m_fDim];
//
//	// потом находим индекс блока-проекции в клетке
//	blockIndex = ((globalIndex[m_fDim]-<a[m_fDim]>)/m_distrArgs.d[m_fDim]) % k[m_fDim];
//
//	// размер блока
//	blockSize = 1;
//	for (i = 0; i < dims_count; i++)
//		blockSize *= (m_distrArgs.d[i] + <добавки по бокам для измерения i>);
//
//	// находим относительный индекс в блоке
//	index = 0;
//	for (i = 0; i < dims_count; i++)
//	{
//		int b = 1;
//		for (j = i+1; j < dims_count; j++)
//			b *= m_distrArgs.d[j] + <добавки по бокам для измерения i>;
//		index += ((globalIndex[i]-<a[i]>) % (m_distrArgs.d[i]) + <a[i]>) * b;
//	}
//
//	//	находим полный индес:	
//	return cellIndex * blockCount * blockSize + blockIndex * blockSize + index;
//
//}

}
}
}
