#include "Transforms/DataDistribution/DataDistributionAnalizer.h"
#include "DataDistributionDeepWalkers.h"
#include "ReferencesFinder.h"
#include "OPS_Core/Localization.h"
#include "FrontTransforms/ExpressionSimplifier.h"
#include "Shared/DataShared.h"
#include "Shared/StatementsShared.h"
#include "Shared/LinearExpressions.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using namespace OPS::Reprise;
            using namespace OPS::Montego;

            SingleAccessAreaInfo::SingleAccessAreaInfo( StatementBase& statement, int shift )
                :m_pStatement(&statement), m_shift(shift)
            {
                std::tr1::shared_ptr<OccurrenceContainer> spOcuurences(new OccurrenceContainer(statement));
                std::vector<BasicOccurrencePtr> occurences = spOcuurences->getAllBasicOccurrencesIn(&statement);

                std::list<Shared::OccurenceInfo> generatorsInfo;
                for (std::vector<BasicOccurrencePtr>::size_type i = 0; i < occurences.size(); ++i)
                {
                    if(occurences[i]->isGenerator())
                    {
                        m_generators.push_back(occurences[i]->getName().m_varDecl);
                    }
                }
            }

            StatementBase& SingleAccessAreaInfo::getStatement()
            {
                OPS_ASSERT(m_pStatement != NULL);

                return *m_pStatement;
            }

            SingleAccessAreaInfo::VariableDeclarationContainer SingleAccessAreaInfo::getGenerators()
            {
                return m_generators;
            }

            int SingleAccessAreaInfo::getShift()
            {
                return m_shift;
            }

            DistributionAreaInfo::DistributionAreaInfo( StatementBase& statement, VariableDeclaration& declaration, VariableDeclaration* pPartDeclaration, BADDParameters distributionParameters, BADDParameters generatorParameters, bool scatterRequired, bool gatherReuired, bool isBlock )
                :m_pStatement(&statement), m_pDeclaration(&declaration), m_pPartDelaration(pPartDeclaration), m_distributionParameters(distributionParameters), m_generatorBADDParameters(generatorParameters), m_refineErrorsDetected(false), m_isBlock(isBlock), m_scatterRequired(scatterRequired), m_gatherRequired(gatherReuired)
            {
                // По указанным параметрам нужно построить параметры со "смещениями" и семейстао параметров
                ArrayAccessExpressionsFinder aaef(m_pDeclaration->getName(), distributionParameters.dims.size());
                m_pStatement->accept(aaef);

                std::list<BasicCallExpression*> allDistributingArrayReferences = aaef.refList;
                std::list<BasicCallExpression*> generatorsDistributingArrayReferences = aaef.generatorsList;

                BADDParametersFamily::BADDParametersContainer parameters(allDistributingArrayReferences.size() + 1);
                
                int i = 0;
                for(std::list<BasicCallExpression*>::iterator it = allDistributingArrayReferences.begin(); it != allDistributingArrayReferences.end(); ++it)
                {
                    parameters[i] = getDistrParametersForArrayReference(*it);
                    if(!parameters[i].isValid())
                    {
                        m_refineErrorsDetected = true;
                        return;
                    }
                    parameters[i].normalize();
                    i++;
                }
                if(!distributionParameters.isValid())
                {
                    m_refineErrorsDetected = true;
                    return;
                }
                distributionParameters.normalize();
                parameters[i] = distributionParameters;

                if(!BADDParametersFamily::areBelongToOneFamily(parameters))
                {
                    m_refineErrorsDetected = true;
                    return;
                }
                DataDistribution::BADDParametersFamily distributionParametersFamily(parameters);

                m_distributionParametersFamily = distributionParametersFamily;
            }

            BADDParameters DistributionAreaInfo::getDistrParametersForArrayReference(OPS::Reprise::BasicCallExpression* arrayAccessExpr)
            {
                BADDParameters result = m_distributionParameters;

                Shared::Occurrence occurence(arrayAccessExpr);
                for (BADDParameters::ParametersContainer::size_type i = 0; i < m_distributionParameters.dims.size(); i++)
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

            StatementBase& DistributionAreaInfo::getStatement()
            {
                OPS_ASSERT(m_pStatement != NULL);

                return *m_pStatement;
            }

            VariableDeclaration& DistributionAreaInfo::getDeclaration()
            {
                OPS_ASSERT(m_pDeclaration != NULL);

                return *m_pDeclaration;
            }

            BADDParameters DistributionAreaInfo::getDistributionParameters()
            {
                return m_distributionParameters;
            }

            OPS::Transforms::DataDistribution::BADDParametersFamily DistributionAreaInfo::getDistributionParametersFamily()
            {
                return m_distributionParametersFamily;
            }

            BADDParameters DistributionAreaInfo::getGeneratorBADDParameters()
            {
                return m_generatorBADDParameters;
            }

            bool DistributionAreaInfo::hasRefineErrors()
            {
                return m_refineErrorsDetected;
            }

            BADDParameters::ParametersContainer::size_type DistributionAreaInfo::getLeadingDimention()
            {
                if (m_distributionParameters.isValid())
                {
                    m_distributionParameters.normalize();
                    return m_distributionParameters.getLeadingDimentionWithLessMemoryDemand();
                }

                return 0;
            }

            bool DistributionAreaInfo::isScatterRequired()
            {
                return m_scatterRequired;
            }

            bool DistributionAreaInfo::isGatherRequired()
            {
                return m_gatherRequired;
            }

            VariableDeclaration* DistributionAreaInfo::getPartDeclaration()
            {
                return m_pPartDelaration;
            }

            bool DistributionAreaInfo::isIterationalProcess()
            {
                return m_scatterRequired && m_gatherRequired && m_pStatement->is_a<ForStatement>();
            }

            int DistributionAreaInfo::getSourceNodeNumber()
            {
                return m_sourceNodeNumber;
            }

            bool DistributionAreaInfo::isBlock()
            {
                return m_isBlock;
            }

            void DistributionAreaInfo::setSourceNodeNumber( int sourceNodeNumber )
            {
                m_sourceNodeNumber = sourceNodeNumber;
            }

            DistributedDataInfo::DistributedDataInfo( VariableDeclaration& declaration, int sourceNodeNumber )
                :m_pDeclaration(&declaration), m_sourceNodeNumber(sourceNodeNumber)
            {

            }

            VariableDeclaration& DistributedDataInfo::getDeclaration()
            {
                OPS_ASSERT(m_pDeclaration != NULL);

                return *m_pDeclaration;
            }

            int DistributedDataInfo::getSourceNodeNumber()
            {
                return m_sourceNodeNumber;
            }

            IgnoredDataInfo::IgnoredDataInfo( VariableDeclaration& declaration )
                :m_pDeclaration(&declaration)
            {

            }

            VariableDeclaration& IgnoredDataInfo::getDeclaration()
            {
                OPS_ASSERT(m_pDeclaration != NULL);

                return *m_pDeclaration;
            }

            ParallelLoopNestingInfo::ParallelLoopNestingInfo( ForStatement& forStatement, bool isBlock, std::vector<int> blockWidths, std::vector<int> iterationsCount)
                :m_pForStatement(&forStatement), m_isBlock(isBlock), m_blockWidths(blockWidths), m_iterationsCount(iterationsCount)
            {

            }

            ForStatement& ParallelLoopNestingInfo::getForStatement()
            {
                OPS_ASSERT(m_pForStatement != NULL);

                return *m_pForStatement;
            }

            bool ParallelLoopNestingInfo::isBlock()
            {
                return m_isBlock;
            }

            std::vector<int> ParallelLoopNestingInfo::getBlockWidths()
            {
                return m_blockWidths;
            }

            std::vector<int> ParallelLoopNestingInfo::getIterationsCount()
            {
                return m_iterationsCount;
            }

            DataDistributionAnalizer::DataDistributionAnalizer( BlockStatement& blockStatement )
                :m_blockStatement(blockStatement), m_controlFlowGraph(blockStatement)
            {
                m_spOccurenceContainer.reset(new OccurrenceContainer(blockStatement));
                m_spAliasInterface.reset(AliasInterface::create(*m_blockStatement.findProgramUnit(), *m_spOccurenceContainer));
                m_spAliasInterface->runAliasAnalysis();
            }

            bool DataDistributionAnalizer::analize()
            {
                // Собираем данные о размещении

                DistributedDataInfoFinder ddif;
                m_blockStatement.getRootBlock().accept(ddif);
                m_distributedDataInfoContainer = ddif.getDistributedDataInfoContainer();

                IgnoredDataInfoFinder idif;
                m_blockStatement.accept(idif);
                m_ignoredDataInfoContainer = idif.getIgnoredDataInfoContainer();

                SingleAccessAreaInfoFinder saaf;
                m_blockStatement.accept(saaf);
                m_singleAccessAreaInfoContainer = saaf.getSingleAccessAreaInfoContainer();

                DistributionAreaInfoFinder daf(m_distributedDataInfoContainer, m_ignoredDataInfoContainer);
                m_blockStatement.accept(daf);
                m_distributionAreaInfoContainer = daf.getDistributionAreaInfoContainer();

                ParallelLoopNestingInfoFinder plnf;
                m_blockStatement.accept(plnf);
                m_parallelLoopNestingInfoContainer = plnf.getParallelLoopNestingInfoContainer();

                // Проверки

                // 1. Проверка распределяемых данных.
                for (DistributedDataInfoContainer::iterator ddit = m_distributedDataInfoContainer.begin(); ddit!=m_distributedDataInfoContainer.end(); ++ddit)
                {
                    VariableDeclaration& declaration = ddit->getDeclaration();
                    // 1.1. Распределяемые данные должны представлять из себя указатель
                    if (!declaration.getType().is_a<PtrType>())
                    {
                        m_errors.push_back(_TL("Data to be distributed(" + declaration.getName() + ") needs to be a dinamic array", "Данные, которые необходимо разместить(" + declaration.getName() + "), должны быть динамическим массивом"));
                        continue;
                    }
                    // 1.2. Распределяемые данные должны быть базового типа
                    TypeBase* pType = Shared::getArrayElementBasicType(&declaration.getType());
                    OPS_ASSERT(pType != NULL);

                    if (!pType->is_a<BasicType>())
                    {
                        m_errors.push_back(_TL("Elements of distributed array(" + declaration.getName() + ") must have a basic type", "Элементы размещаемого массива(" + declaration.getName() + ") должны быть базового типа"));
                        continue;
                    }
                    // 1.3. У распределяемых данных не должно быть альясов
                    bool hasOnlyTrivialAliases = true;
                    for(Declarations::VarIterator varIt = m_blockStatement.getDeclarations().getFirstVar(); varIt.isValid(); ++varIt)
                    {
                        if(&(*varIt) != &declaration && m_spAliasInterface->isAlias(*varIt, declaration))
                        {
                            hasOnlyTrivialAliases = false;
                        }
                    }
                    if (!hasOnlyTrivialAliases)
                    {
                        m_errors.push_back(_TL("Distributed array must have no aliases", "Размещаемый массив не должен иметь псевдонимов"));
                        continue;
                    }
                    // 1.4. Распределяемый массив должен встречаться либо в своей секции единичного доступа, либо в своей секции распределения
                    std::list<BasicOccurrencePtr> allOcurrences = m_spOccurenceContainer->getAllBasicOccurrencesOf(declaration);
                    bool allReferencesCorrect = true;

                    for(std::list<BasicOccurrencePtr>::iterator allOcurrencesIt = allOcurrences.begin(); allOcurrencesIt != allOcurrences.end(); ++allOcurrencesIt)
                    {
                        bool correctParentFound = false;

                        for(SingleAccessAreaInfoContainer::iterator saait = m_singleAccessAreaInfoContainer.begin(); saait != m_singleAccessAreaInfoContainer.end(); ++saait)
                        {
                            StatementBase& statement = saait->getStatement();
                            if(Shared::contain(&statement, (*allOcurrencesIt)->getParentStatement()) && saait->getShift() == ddit->getSourceNodeNumber())
                            {
                                correctParentFound = true;
                                break;
                            }
                        }

                        if(!correctParentFound)
                        {
                            for (DistributionAreaInfoContainer::iterator dait = m_distributionAreaInfoContainer.begin(); dait != m_distributionAreaInfoContainer.end(); ++dait)
                            {
                                StatementBase& statement = dait->getStatement();
                                if (&dait->getDeclaration() == &declaration && Shared::contain(&statement, (*allOcurrencesIt)->getParentStatement()))
                                {
                                    dait->setSourceNodeNumber(ddit->getSourceNodeNumber());
                                    correctParentFound = true;
                                    break;
                                }
                            }
                        }

                        if(!correctParentFound)
                        {
                            allReferencesCorrect = false;
                            break;
                        }
                    }

                    if(!allReferencesCorrect)
                    {
                        m_errors.push_back(_TL("All references(of " + declaration.getName() + ") must be placed in single access area or in distribution area", "Все вхождения размещаемого массива(" + declaration.getName() + ") должны находиться внутри секции единичного доступа или секции распределения данных"));
                        continue;
                    }
                }
                
                // 2. Проверка секции единичного доступа
                for (SingleAccessAreaInfoContainer::iterator saait = m_singleAccessAreaInfoContainer.begin(); saait != m_singleAccessAreaInfoContainer.end(); ++saait)
                {
                    StatementBase& statement = saait->getStatement();
                    // 2.1. В секции единичного доступа генераторами могут быть только:
                    // 2.1.1. Размещаемые данные
                    // 2.1.2. Игнорируемые данные
                    // 2.1.3. Другие данные при условии что запись ведется в элемент массива.
                    SingleAccessAreaInfo::VariableDeclarationContainer generators = saait->getGenerators();
                    for (SingleAccessAreaInfo::VariableDeclarationContainer::iterator vdcit = generators.begin(); vdcit != generators.end(); ++vdcit)
                    {
                        VariableDeclaration* pDeclaration = *vdcit;
                        if(pDeclaration == NULL) continue;

                        bool declarationFound = false;

                        for (DataDistributionAnalizer::DistributedDataInfoContainer::iterator ddicit = m_distributedDataInfoContainer.begin(); ddicit != m_distributedDataInfoContainer.end(); ++ddicit)
                        {
                            if(pDeclaration == &(ddicit->getDeclaration()))
                            {
                                declarationFound = true;
                                break;
                            }
                        }
                        if(!declarationFound)
                        {
                            for (DataDistributionAnalizer::IgnoredDataInfoContainer::iterator idicit = m_ignoredDataInfoContainer.begin(); idicit != m_ignoredDataInfoContainer.end(); ++idicit)
                            {
                                if(pDeclaration == &(idicit->getDeclaration()))
                                {
                                    declarationFound = true;
                                    break;
                                }
                            }
                        }

                        if (declarationFound) continue;

                        if (pDeclaration->getType().is_a<BasicType>()) continue;

                        if (pDeclaration->getType().is_a<ArrayType>())
                        {
                            TypeBase* pType = Shared::getArrayElementBasicType(&(pDeclaration->getType()));
                            OPS_ASSERT(pType != NULL);

                            if(pType->is_a<BasicType>()) continue;
                        }

                        m_errors.push_back(_TL(
                            "It's forbidden to assign to " + pDeclaration->getName() + " in single access area",
                            "В секции единичного доступа невозможно выполнить присваивание переменной " + pDeclaration->getName()));
                    }
                    
                    // 2.2. Секция единичного доступа не должна быть вложена ни в 1 распараллеливаемый участок(например ParallelLoopNesting)
                    bool allStatementsCorrect = true;
                    for (ParallelLoopNestingInfoContainer::iterator plnit = m_parallelLoopNestingInfoContainer.begin(); plnit != m_parallelLoopNestingInfoContainer.end(); ++plnit)
                    {
                        if(Shared::contain(&plnit->getForStatement(), &statement))
                        {
                            allStatementsCorrect = false;
                            break;
                        }
                    }
                    if(!allStatementsCorrect)
                    {
                        m_errors.push_back(_TL("Single access area can not be placed into distribution area", "Секция единичного доступа не может располагаться внутри секции распределения данных"));
                        continue;
                    }
                    // 2.3. Секция единичного доступа должна иметь не более одной точки входа и одной точки выхода
                    if (m_controlFlowGraph.getInEdges(statement).size() > 1 || m_controlFlowGraph.getOutEdges(statement).size() > 1)
                    {
                        m_errors.push_back(_TL("Single access area must have one entry point and one exit point", "Секция единичного доступа должна иметь не более одной точки входа и одной точки выхода"));
                        continue;
                    }
                }

                // 3. Проверка секции распределения данных
                for (DistributionAreaInfoContainer::iterator dait = m_distributionAreaInfoContainer.begin(); dait != m_distributionAreaInfoContainer.end(); ++dait)
                {
                    StatementBase& statement = dait->getStatement();
                    VariableDeclaration& declaration = dait->getDeclaration();
                    // 3.1. Массив, которому соответствует секция распределения данных должен быть распределяемым
                    bool distributedDataFound = false;
                    for (DistributedDataInfoContainer::iterator ddit = m_distributedDataInfoContainer.begin(); ddit != m_distributedDataInfoContainer.end(); ++ddit)
                    {
                        if(&declaration == &ddit->getDeclaration())
                        {
                            distributedDataFound = true;
                            break;
                        }
                    }
                    if(!distributedDataFound)
                    {
                        m_errors.push_back(_TL("Distribution area must correspond with some distributed data", "Секция распределения должна соответствовать распределяемым данным"));
                        continue;
                    }
                    // 3.1' Целевой массив, которому соответствует секция распределения данных должен быть игнорируемым
                    VariableDeclaration* pPartDeclaration = dait->getPartDeclaration();
                    if(pPartDeclaration != NULL)
                    {
                        distributedDataFound = false;
                        for (IgnoredDataInfoContainer::iterator ddit = m_ignoredDataInfoContainer.begin(); ddit != m_ignoredDataInfoContainer.end(); ++ddit)
                        {
                            if(pPartDeclaration == &ddit->getDeclaration())
                            {
                                distributedDataFound = true;
                                break;
                            }
                        }
                        if(!distributedDataFound)
                        {
                            m_errors.push_back(_TL("Target array must be ignored data", "Целевой массив должен быть игнорируемым"));
                            continue;
                        }
                    }
                    // 3.2. Проверка корректности параметров
                    if(!dait->getDistributionParameters().isValid())
                    {
                        m_errors.push_back(_TL("Parameters are not valid", "Параметры некорректны"));
                        continue;
                    }
                    if(dait->hasRefineErrors())
                    {
                        m_errors.push_back(_TL("Unable to refine parameters or build parameters family", "Не удалось уточнить параметры или построить семейство параметров"));
                        continue;
                    }
                    // 3.3. Секция распределения не должна быть вложена ни в одну секцию единичного доступа
                    bool allStatementsCorrect = true;
                    for (SingleAccessAreaInfoContainer::iterator saait = m_singleAccessAreaInfoContainer.begin(); saait != m_singleAccessAreaInfoContainer.end(); ++saait)
                    {
                        if(Shared::contain(&saait->getStatement(), &statement))
                        {
                            allStatementsCorrect = false;
                            break;
                        }
                    }
                    if(!allStatementsCorrect)
                    {
                        m_errors.push_back(_TL("Distribution area can not be placed into single access area", "Секция распределения данных не может располагаться внутри секции единичного доступа"));
                        continue;
                    }
                    // 3.4. Секция распределения данных должна иметь не более одной точки входа и одной точки выхода
                    if (m_controlFlowGraph.getInEdges(statement).size() > 1 || m_controlFlowGraph.getOutEdges(statement).size() > 1)
                    {
                        m_errors.push_back(_TL("Distribution area must have one entry point and one exit point", "Секция распределения данных должна иметь не более одной точки входа и одной точки выхода"));
                        continue;
                    }
                    // 3.5. Секция распределнния данных должна содержать как минимум 1 распараллеливаемый участок(например ParallelLoopNesting) в случае, если не указан целевой массив
                    ParallelLoopNestingInfoContainer innerParallelLoopNesting;
                    if(pPartDeclaration == NULL)
                    {
                        for (ParallelLoopNestingInfoContainer::iterator plnit = m_parallelLoopNestingInfoContainer.begin(); plnit != m_parallelLoopNestingInfoContainer.end(); ++plnit)
                        {
                            ForStatement& forStatement = (*plnit).getForStatement(); 
                            if(Shared::contain(&statement, &forStatement))
                            {
                                innerParallelLoopNesting.push_back(*plnit);
                            }
                        }
                        if (innerParallelLoopNesting.size() == 0)
                        {
                            m_errors.push_back(_TL("Distribution area must have at least one parallelized area(e.g. ops_loop_nesting)", "Секция распределения данных должна содержать как минимум одну распараллеливаемую область(например loop_nesting_area)"));
                            continue;
                        }
                    }
                    // 3.6. Все обращения к соответствующим распределяемым данным должны находиться внутри дочерних областей распараллеливаемых данных в случае, если не указан целевой массив
                    //      Если целевой массив указан таких обращений быть не должно
                    std::tr1::shared_ptr<OccurrenceContainer> spOccurences(new OccurrenceContainer(statement));
                    std::list<BasicOccurrencePtr> occurences = spOccurences->getAllBasicOccurrencesOf(declaration);

                    if (pPartDeclaration == NULL)
                    {
                        bool allReferencesAreCorrect = true;
                        for (std::list<BasicOccurrencePtr>::iterator ddrit = occurences.begin(); ddrit != occurences.end(); ++ddrit)
                        {
                            StatementBase* pDistributedDataParentStatement = (*ddrit)->getParentStatement();
                            OPS_ASSERT(pDistributedDataParentStatement != NULL);

                            bool referenceInParallelizedArea = false;
                            for (ParallelLoopNestingInfoContainer::iterator plnit = innerParallelLoopNesting.begin(); plnit != innerParallelLoopNesting.end(); ++plnit)
                            {
                                ForStatement& forStatement = plnit->getForStatement();
                                if(Shared::contain(&forStatement, pDistributedDataParentStatement))
                                {
                                    referenceInParallelizedArea = true;
                                    break;
                                }
                            }

                            if (!referenceInParallelizedArea)
                            {
                                allReferencesAreCorrect = false;
                                break;
                            }
                        }

                        if (!allReferencesAreCorrect)
                        {
                            m_errors.push_back(_TL("All references of distributed data inside distributed area must be placed in parallelized area(e.g. ops_loop_nesting)", "Все ссылки на размещаемые данные внутри секции распределения должны располагаться внутри распараллеливаемой области(например loop_nesting_area)"));
                            continue;
                        }
                    }
                    else
                    {
                        if(occurences.size() > 0)
                        {
                            m_errors.push_back(_TL("Distribute area with specified target array must have no references to source array", "Область распределения данных с указанным целевым массивом не должна иметь обращений к исходному массиву"));
                            continue;
                        }
                    }

                    // 3.7. Кол-во скобок [] должно равняться размерности массива
                    bool allReferencesAreCorrect = true;
                    for (std::list<BasicOccurrencePtr>::iterator ddrit = occurences.begin(); ddrit != occurences.end(); ++ddrit)
                    {
                        if((*ddrit)->getBracketCount() != (int)dait->getDistributionParameters().dims.size())
                        {
                            allReferencesAreCorrect = false;
                            break;
                        }
                    }
                    if(!allReferencesAreCorrect)
                    {
                        m_errors.push_back(_TL("Number of brackets must to match with number of dimetions of distributed array.", "Число квадратных скобок должно совпадать с размерностью распределяемого массива."));
                        continue;
                    }
                }

                // 4. Проверка распараллеливания цикла с помощью гнездования
                for (ParallelLoopNestingInfoContainer::iterator plnit = m_parallelLoopNestingInfoContainer.begin(); plnit != m_parallelLoopNestingInfoContainer.end(); ++plnit)
                {
                    ForStatement& forStatement = plnit->getForStatement();
                    // 4.1. Цикл должен находиться внутри секции распределения данных
                    bool distributionAreaFound = false;
                    for (DistributionAreaInfoContainer::iterator dait = m_distributionAreaInfoContainer.begin(); dait != m_distributionAreaInfoContainer.end(); ++dait)
                    {
                        if(Shared::contain(&dait->getStatement(), &forStatement))
                        {
                            distributionAreaFound = true;
                            break;
                        }
                    }
                    if(!distributionAreaFound)
                    {
                        m_errors.push_back(_TL("Parallelized loop must be placed into distribution area", "Распараллеливаемый цикл должен быть помещен в секцию распределения данных"));
                        continue;
                    }
                    // 4.2. В распараллеливаемом цикле генераторами могут быть только размещаемые данные и игнорируемые данные
                    std::tr1::shared_ptr<OccurrenceContainer> spOccurences(new OccurrenceContainer(forStatement));
                    std::vector<BasicOccurrencePtr> occurences = spOccurences->getAllBasicOccurrencesIn(&forStatement);

                    bool allGeneratorsCorrect = true;
                    for (std::vector<BasicOccurrencePtr>::size_type i = 0; i < occurences.size(); ++i)
                    {
                        if(occurences[i]->isGenerator() && Shared::contain(&forStatement, occurences[i]->getParentStatement()))
                        {
                            bool declarationFound = false;

                            for (DistributedDataInfoContainer::iterator ddit = m_distributedDataInfoContainer.begin(); ddit != m_distributedDataInfoContainer.end(); ++ddit)
                            {
                                VariableDeclaration& declaration = ddit->getDeclaration();
                                if(&declaration == Shared::getArrayVariableDeclaration(*occurences[i]->getSourceExpression()))
                                {
                                    declarationFound = true;
                                    break;
                                }
                            }
                            if(!declarationFound)
                            {
                                for (IgnoredDataInfoContainer::iterator idic = m_ignoredDataInfoContainer.begin(); idic != m_ignoredDataInfoContainer.end(); ++idic)
                                {
                                    VariableDeclaration& declaration = idic->getDeclaration();
                                    if(&declaration == Shared::getArrayVariableDeclaration(*occurences[i]->getSourceExpression()))
                                    {
                                        declarationFound = true;
                                        break;
                                    }
                                }
                            }
                            if (!declarationFound)
                            {
                                allGeneratorsCorrect = false;
                                break;
                            }
                        }
                    }

                    if(!allGeneratorsCorrect)
                    {
                        m_errors.push_back(_TL("Parallelized loop must contain only distributed or ignored data as generators", "В качестве генераторов в распараллеливаемом цикле могут выступать только распределяемые или игнорируемые данные"));
                        continue;
                    }
                    // 4.3. Распараллеливаемый цикл должен иметь не более одной точки входа и одной точки выхода
                    if (m_controlFlowGraph.getInEdges(forStatement).size() > 1 || m_controlFlowGraph.getOutEdges(forStatement).size() > 1)
                    {
                        m_errors.push_back(_TL("Parallelized loop must have one entry point and one exit point", "Распараллеливаемый цикл должен иметь не более одной точки входа и одной точки выхода"));
                        continue;
                    }
                }

                return m_errors.size() == 0;
            }

            DataDistributionAnalizer::SingleAccessAreaInfoContainer DataDistributionAnalizer::getSingleAccessAreaInfoContainer()
            {
                return m_singleAccessAreaInfoContainer;
            }

            DataDistributionAnalizer::DistributionAreaInfoContainer DataDistributionAnalizer::getDistributionAreaInfoContainer()
            {
                return m_distributionAreaInfoContainer;
            }

            DataDistributionAnalizer::ParallelLoopNestingInfoContainer DataDistributionAnalizer::getParallelLoopNestingInfoContainer()
            {
                return m_parallelLoopNestingInfoContainer;
            }

            DataDistributionAnalizer::StringContainer DataDistributionAnalizer::getErrors()
            {
                return m_errors;
            }

            DataDistributionAnalizer::DistributedDataInfoContainer DataDistributionAnalizer::getDistributedDataInfoContainer()
            {
                return m_distributedDataInfoContainer;
            }

            DataDistributionAnalizer::IgnoredDataInfoContainer DataDistributionAnalizer::getIgnoredDataInfoContainer()
            {
                return m_ignoredDataInfoContainer;
            }
        }
    }
}
