//		STL headers
#include <sstream>
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stack>
#include <cassert>
#include <list>
#include <stdexcept>

//		OPS headers
#include "OPS_Core/IO.h"
#include "Shared/LoopShared.h"
#include "Shared/RepriseClone.h"
#include "Transforms/DataDistribution/Shared/DataDistributionForSharedMemory.h"
#include "Transforms/DataDistribution/Shared/BDParameters.h"
#include "Transforms/Loops/LoopNesting/LoopNesting.h"

#include "Shared/SubroutinesShared.h"
#include "Shared/DataShared.h"
#include "Shared/LinearExpressions.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/StatementsShared.h"

//#include "../ReferencesFinder.h"
//#include "DimExpressionConanizer.h"

#include "Reprise/ServiceFunctions.h"
#include "Reprise/Canto/HirFTypes.h"
#include "Reprise/ServiceFunctions.h"

#include "include/RenameDistributionArrayVisitor.h"
#include "include/ChangeIndexesVisitor.h"
#include "include/shared_helpers.h"
#include "include/ArrayDistributionInfo.h"
#include "include/ReferenceTable.h"
#include "include/change_references_algorithms.h"


using namespace std;
using namespace OPS::Shared;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Editing;
using namespace OPS::Reprise::Canto;
using namespace OPS::Transforms;
using namespace OPS::TransformationsHub;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Transforms::Loops;

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{



void print_table(ReferenceTable& dim_table, string path)
{
    std::stringstream ss;

    {
        auto rows = dim_table.table_rows();
        auto cols = dim_table.table_cols();
        for (auto col = cols.begin(); col != cols.end(); ++col)
        {
            ReferenceTable::Column c = *col;
            if (c.loopStmt != 0)
            {
                ForStatement* stmt = c.loopStmt;
                cout << "!!!!!!!!!!!!!!" << endl;
                cout << stmt->dumpState() << endl;
                BasicCallExpression* init = c.loopStmt->getInitExpression().cast_ptr<BasicCallExpression>();

                ss << ";" << getLoopCounter(c.loopStmt)->getName();

                if (c.loopStmt->hasNote("Kind") == true)
                {
                    ss << OPS::Strings::format(" kind=%s", c.loopStmt->getNote("Kind").getInt() == LoopByBlockElements ? "by elems" : "by blocks");
                    ss << OPS::Strings::format(" d=%s", c.loopStmt->getNote("BlockSize").getReprise().cast_to<ExpressionBase>().dumpState().c_str());
                }
            }
            else
                ss << ";" << "EMPTY";
        }
        ss << "\n";

        for (auto row = rows.begin(); row != rows.end(); ++row)
        {
            ReferenceTable::Row r1 = *row;

            ss << r1.ref.source->dumpState();
            ss << ": " << r1.dimensionIndex;

            for (auto col = cols.begin(); col != cols.end(); ++col)
            {
                ss << ";";

                if (dim_table.contains_table_item(*row, *col) == true)
                    ss << dim_table.get_table_item(*row, *col).toString();
                else
                    ss << 0;
            }
            ss << "\n";
        }

        ss << "\n\n";
        {
            for (auto col = cols.begin(); col != cols.end(); ++col)
            {
                ReferenceTable::Column c = *col;

                if (c.loopStmt == 0)
                    continue;

                ss << getLoopCounter(c.loopStmt)->getName() << ": ";
                auto dat = Strings::split(c.loopStmt->dumpState(), "\n");
                if (dat.empty() == false)
                {
                    auto dd1 = dat.front();
                    ss << dd1;
                }

                ss << "\n";
            }
        }

        ss << "\n\n";
    }

    {
        auto rows = dim_table.not_optimizable_rows();
        for (auto row = rows.begin(); row != rows.end(); ++row)
        {
            ReferenceTable::Row r1 = *row;
            ss << r1.ref.source->dumpState();
            ss << ": " << r1.dimensionIndex;
            ss << "\n";
        }
    }


    OPS::IO::writeFile(path, ss.str());
}

void DataDistributionForSharedMemory::makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params)
{
    makeTransform(program);
}


ExpressionStatement* createAllocationStatement(SubroutineDeclaration* malloc_func, std::shared_ptr<ArrayDistributionInfo> info)
{
    Note note;
    note.setBool(true);

    SubroutineCallExpression* funcCallExpr = new SubroutineCallExpression(new SubroutineReferenceExpression(*malloc_func));

    IntegerHelper ih(BasicType::BT_INT32);
    funcCallExpr->addArgument( &((*info->calculateDistributedArraySize()) * ih(info->arrayType().getSizeOf())) );
    ReprisePtr<ExpressionBase> leftPartExpr(new ReferenceExpression(*info->arrayInfo()));
    leftPartExpr->setNote("IndicesAreUpdated", note);
    return new ExpressionStatement( &(op(leftPartExpr) R_AS op(funcCallExpr)));
}
ExpressionStatement* createReleaseStatement(SubroutineDeclaration* free_func, std::shared_ptr<ArrayDistributionInfo>& info)
{
    Note note;
    note.setBool(true);

    SubroutineCallExpression* funcCallExpr = new SubroutineCallExpression(new SubroutineReferenceExpression(*free_func));
    ReprisePtr<ExpressionBase> arrayExpr(new ReferenceExpression(*info->arrayInfo()));
    arrayExpr->setNote("IndicesAreUpdated", note);
    funcCallExpr->addArgument(&(op(arrayExpr)));
    return new ExpressionStatement(funcCallExpr);
}

/*
class BloclArrayReferencesCollector : public Service::DeepWalker
{
public:
    list<BlockArrayRefInfo> result;
    list<shared_ptr<ArrayDistributionInfo> > info_list;
    void visit(BasicCallExpression& node)
    {
        if (node.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
        {
            ExpressionBase& expr = node.getArgument(0);
            if (expr.is_a<ReferenceExpression>() == true)
            {
                ReferenceExpression& ref = expr.cast_to<ReferenceExpression>();
                shared_ptr<ArrayDistributionInfo> info;
                if (isBlockArrayReference(ref, info_list, info) == true)
                {
                    BlockArrayRefInfo ri;
                    ri.info = info;
                    ri.expr = &node;
                    result.push_back(ri);
                }
            }
        }
        Service::DeepWalker::visit(node);
    }
};*/

class LinearExpressionCollector : public Service::DeepWalker
{
    int m_minusSignCounter;
    std::list<ReprisePtr<ExpressionBase> > exprs;
public:
    LinearExpressionCollector()
        : m_minusSignCounter(0)
    {
    }

    std::vector<ReprisePtr<ExpressionBase> > exprList()
    {
        std::vector<ReprisePtr<ExpressionBase> > res;
        for (std::list<ReprisePtr<ExpressionBase> >::iterator it = exprs.begin(); it != exprs.end(); ++it)
            res.push_back(*it);
        return res;

    }
    void processFactorExpr(ExpressionBase& expr)
    {
        IntegerHelper ih(BasicType::BT_INT32);
        ExpressionBase* result = expr.clone();
        if (m_minusSignCounter % 2 != 0)
            result = &(op(*result) * op(ih(-1)));
        exprs.push_back(ReprisePtr<ExpressionBase>(result));
    }

#define LinearExpressionCollectorVisitMethod(ExprType) \
    void visit(ExprType& node) { processFactorExpr(node); }

    LinearExpressionCollectorVisitMethod(BasicLiteralExpression)
    LinearExpressionCollectorVisitMethod(CompoundLiteralExpression)
    LinearExpressionCollectorVisitMethod(SubroutineReferenceExpression)
    LinearExpressionCollectorVisitMethod(StructAccessExpression)
    LinearExpressionCollectorVisitMethod(EnumAccessExpression)
    LinearExpressionCollectorVisitMethod(SubroutineCallExpression)
    LinearExpressionCollectorVisitMethod(EmptyExpression)
    LinearExpressionCollectorVisitMethod(ReferenceExpression)
    LinearExpressionCollectorVisitMethod(StrictLiteralExpression)


    void visit(BasicCallExpression& expr)
    {
        switch (expr.getKind())
        {
        case BasicCallExpression::BCK_BINARY_PLUS:
            Service::DeepWalker::visit(expr);
            break;
        case BasicCallExpression::BCK_BINARY_MINUS:
            expr.getArgument(0).accept(*this);

            ++m_minusSignCounter;
            expr.getArgument(1).accept(*this);
            --m_minusSignCounter;
            break;
        default:
            processFactorExpr(expr);
        };
    }
};

//
//      Класс проходит по выражению и проверяет, что оно
//      имеет вид a*Nk*..*Nn, k - входной параметр ('index').
//      Если это так, то в coef записывается выражение 'a'.
class MulExpressionWalker : public Service::DeepWalker
{
    ExpressionBase* m_coef;
    ArrayDistributionInfo m_info;
    std::list<ExpressionBase*> m_expectedFactors;

    void populateCoef(ExpressionBase* factor)
    {
        if (m_coef == 0)
            m_coef = factor->clone();
        else
            m_coef = &(op(*m_coef) * op(*factor));
    }
public:
    MulExpressionWalker(ArrayDistributionInfo& info, std::list<ExpressionBase*> expectedFactors)
        : m_info(info), m_coef(0)
    {
        m_expectedFactors = expectedFactors;
    }

    void visit(BasicCallExpression& node)
    {
        if (node.getKind() == BasicCallExpression::BCK_MULTIPLY)
            Service::DeepWalker::visit(node);
        else
            populateCoef(&node);
    }

#define MulExpressionWalkerVisitMethod(ExprType) \
    void visit(ExprType& node) { populateCoef(&node); }

    MulExpressionWalkerVisitMethod(BasicLiteralExpression)
    MulExpressionWalkerVisitMethod(CompoundLiteralExpression)
    MulExpressionWalkerVisitMethod(SubroutineReferenceExpression)
    MulExpressionWalkerVisitMethod(StructAccessExpression)
    MulExpressionWalkerVisitMethod(EnumAccessExpression)
    MulExpressionWalkerVisitMethod(SubroutineCallExpression)
    MulExpressionWalkerVisitMethod(EmptyExpression)

    void visit(ReferenceExpression& node)
    {
        bool isExpectedFactor = false;
        for (std::list<ExpressionBase*>::iterator it = m_expectedFactors.begin(); it != m_expectedFactors.end();)
        {
            ExpressionBase* nextExpectedFactorExpr = *it;
            if (nextExpectedFactorExpr->is_a<ReferenceExpression>() &&
                    &node.getReference() == &nextExpectedFactorExpr->cast_to<ReferenceExpression>().getReference())
            {
                isExpectedFactor = true;
                it = m_expectedFactors.erase(it);
                break;
            }
            else
                ++it;
        }
        if (isExpectedFactor == false)
            populateCoef(&node);

    }
    void visit(StrictLiteralExpression& node)
    {
        bool isExpectedFactor = false;
        for (std::list<ExpressionBase*>::iterator it = m_expectedFactors.begin(); it != m_expectedFactors.end();)
        {
            ExpressionBase* expr = *it;
            if (expr->is_a<StrictLiteralExpression>())
            {
                StrictLiteralExpression* l = expr->cast_ptr<StrictLiteralExpression>();
                if (l->getInt32() == node.getInt32())
                {
                    isExpectedFactor = true;
                    it = m_expectedFactors.erase(it);
                    break;
                }
                else
                    ++it;
            }
            else
                ++it;
        }
        if (isExpectedFactor == false)
            populateCoef(&node);
    }

    bool allFactorsAreFounded()
    {
        return m_expectedFactors.empty() == true;
    }
    ReprisePtr<ExpressionBase> coef()
    {
        if (m_coef == 0)
            m_coef = StrictLiteralExpression::createInt32(1);
        return ReprisePtr<ExpressionBase>(m_coef);
    }
};

std::list<DistributedArrayReferenceInfo> references_without_multi_index;
ReferenceTable dim_table;

void preprocess_distributed_array_references(std::list<DistributedArrayReferenceInfo>& references_without_multi_index, ReferenceTable& dim_table, std::list<DistributedArrayReferenceInfo>& array_refs,
                                             SubroutineDeclaration* current_function)
{
    for (auto it=array_refs.begin(); it!=array_refs.end(); ++it)
    {
        bool refCanBeOptimized = true;
        //  результирующий мультииндекс

        DistributedArrayReferenceInfo distributedRef = *it;
        vector<ReprisePtr<ExpressionBase> > dimExprs(distributedRef.distributionInfo->dimensionCount());


        //  раскладываем исходный индекс на слагаемые
        LinearExpressionCollector visitor;
        distributedRef.source->getArgument(1).accept(visitor);
        vector<ReprisePtr<ExpressionBase> > exprList = visitor.exprList();

        std::list<int> notViewed;
        for (int i = 0; i < exprList.size(); i++)
            notViewed.push_back(i);

        int currentDimensionIndexIndex = 0;

        bool can_be_optimized = false;
        while (currentDimensionIndexIndex < distributedRef.distributionInfo->dimensionCount())
        {
            std::list<ExpressionBase*> expectedFactors;
            for (int i = currentDimensionIndexIndex+1; i < distributedRef.distributionInfo->dimensionCount(); i++)
                expectedFactors.push_back(&distributedRef.distributionInfo->dimensionSize(i));

            can_be_optimized = false;
            for (std::list<int>::iterator it2 = notViewed.begin(); it2 != notViewed.end();)
            {
                MulExpressionWalker visitor(*distributedRef.distributionInfo, expectedFactors);
                exprList[*it2]->accept(visitor);
                if (visitor.allFactorsAreFounded() == true)
                {
                    ReprisePtr<ExpressionBase> coef = visitor.coef();
                    if (dimExprs[currentDimensionIndexIndex].get() == 0)
                        dimExprs[currentDimensionIndexIndex] = coef;
                    else
                        dimExprs[currentDimensionIndexIndex] = ReprisePtr<ExpressionBase>(&(op(*dimExprs[currentDimensionIndexIndex]) + op(*coef)));
                    it2 = notViewed.erase(it2);
                    can_be_optimized = true;
                }
                else
                    ++it2;
            }
            if (can_be_optimized == false)
                break;
            ++currentDimensionIndexIndex;
        }

        if (can_be_optimized == false)
        {
            // не удалось получить мультиндекс
            refCanBeOptimized = false;
            references_without_multi_index.push_back(distributedRef);
            continue;
        }

        OPS_ASSERT(notViewed.empty() == true);
        auto allLoopsList = OPS::Shared::getEmbracedLoopsNest(*distributedRef.source, current_function);
        std::vector<ForStatement*> allLoops(allLoopsList.begin(), allLoopsList.end());
        ParametricLinearExpression::VariablesDeclarationsVector loopCounters;
        for (int j = 0; j < allLoops.size(); j++)
        {
            cout <<"-----------------\n";
            cout << allLoops[j]->dumpState() << "\n";

            if (check_loop(*allLoops[j]) == false)
                throw std::runtime_error("for is not basic");
            loopCounters.push_back(getLoopCounter(allLoops[j]));
        }

        for (int i = 0; i < distributedRef.distributionInfo->dimensionCount(); i++)
        {
            ParametricLinearExpression* linExpr = ParametricLinearExpression::createByListOfVariables(dimExprs[i].get(), loopCounters);
            ParametricLinearExpression::Coefficient freeCoef = linExpr->getFreeCoefficient();

            bool dim_can_be_optimized = true;
            for (int j = 0; j < loopCounters.size(); j++)
            {
                VariableDeclaration* counter = loopCounters[j];
                ParametricLinearExpression::Coefficient counterCoef = linExpr->getCoefficient(counter);

                if (counterCoef.get() != 0
                        && counterCoef->cast_ptr<StrictLiteralExpression>() == 0
                        && counterCoef->cast_ptr<ReferenceExpression>() == 0)
                {
                    cout << counterCoef->dumpState() << endl;
                    dim_can_be_optimized = false;
                    break;
                }
            }

            if (freeCoef.get() != 0
                    && freeCoef->cast_ptr<StrictLiteralExpression>() == 0
                    && freeCoef->cast_ptr<ReferenceExpression>() == 0)
            {
                cout << freeCoef->dumpState() << endl;
                dim_can_be_optimized = false;
            }

            // индексное выражение, входящее в мультииндекс, нельзя соптимизировать
            if (dim_can_be_optimized == false)
                dim_table.add_not_optimizable_dim(ReferenceTable::Row(distributedRef, i), dimExprs[i]);
            else
            {
                for (int j = 0; j < loopCounters.size(); j++)
                {
                    VariableDeclaration* counter = loopCounters[j];
                    ParametricLinearExpression::Coefficient counterCoef = linExpr->getCoefficient(counter);

                    if (counterCoef.get() != 0)
                    {
                        if (counterCoef->cast_ptr<StrictLiteralExpression>() != 0)
                        {
                            int c = counterCoef->cast_ptr<StrictLiteralExpression>()->getInt32();
                            if (c != 0)
                                dim_table.insert_table_item(ReferenceTable::Row(distributedRef, i),
                                                            ReferenceTable::Column(allLoops[j]),
                                                            ReferenceTable::Value::makeInt(c));
                        }
                        else
                        {
                            auto refExpr = counterCoef->cast_ptr<ReferenceExpression>();
                            OPS_ASSERT(refExpr != 0);
                            dim_table.insert_table_item(ReferenceTable::Row(distributedRef, i),
                                                        ReferenceTable::Column(allLoops[j]),
                                                        ReferenceTable::Value::makeRef(refExpr));
                        }


                    }
                }
                if (freeCoef.get() != 0)
                {
                    if (freeCoef->cast_ptr<StrictLiteralExpression>() != 0)
                    {
                        int c = freeCoef->cast_ptr<StrictLiteralExpression>()->getInt32();
                        if (c != 0)
                            dim_table.insert_table_item(ReferenceTable::Row(distributedRef, i),
                                                        ReferenceTable::Column(0),
                                                        ReferenceTable::Value::makeInt(c));
                    }
                    else
                    {
                        auto refExpr = freeCoef->cast_ptr<ReferenceExpression>();
                        OPS_ASSERT(refExpr != 0);
                        dim_table.insert_table_item(ReferenceTable::Row(distributedRef, i),
                                                    ReferenceTable::Column(0),
                                                    ReferenceTable::Value::makeRef(refExpr));
                    }
                }
            }
        }
    }
}




class SubstitudeVariableWalker : public Service::DeepWalker
{
    OPS::Reprise::ReprisePtr<ReferenceExpression> m_old_ref;
    OPS::Reprise::ReprisePtr<ExpressionBase> m_new_expr;
public:
    SubstitudeVariableWalker(OPS::Reprise::ReprisePtr<ReferenceExpression> old_ref, OPS::Reprise::ReprisePtr<ExpressionBase> new_expr)
    {
        m_old_ref = old_ref;
        m_new_expr = new_expr;
    }

    void visit(ReferenceExpression& expr)
    {
        if (expr.getReference().getNCID() == m_old_ref->getReference().getNCID())
            OPS::Reprise::Editing::replaceExpression(expr, ReprisePtr<ExpressionBase>(m_new_expr->clone()));

    }
};

bool DataDistributionForSharedMemory::nest_loops(ReferenceTable& dim_table)
{
    auto floor_func = m_currentTranslationUnit->getGlobals().findSubroutine("floor_kernel_func");
    auto ceil_func = m_currentTranslationUnit->getGlobals().findSubroutine("ceil_kernel_func");
    if (! (floor_func != 0 && ceil_func != 0) )
        return false;

    auto rows = dim_table.table_rows();
    auto cols = dim_table.table_cols();


    std::set<OPS::Reprise::ForStatement*> viewed_loops;
    std::list<OPS::Reprise::ForStatement*> loops_for_nesting;
    std::list<OPS::Reprise::ExpressionBase*> nesting_block_size;
    for (auto itc = cols.begin(); itc != cols.end(); ++itc)
    {
        ReferenceTable::Column c = *itc;
        if (c.loopStmt == 0 || check_loop(*c.loopStmt) == false)
            continue;

        for (auto itr = rows.begin(); itr != rows.end(); ++itr)
        {
            ReferenceTable::Row r = *itr;
            if (dim_table.contains_table_item(r, c) == false)
                continue;
            bool is_good;
            if (row_needs_optimization(r, dim_table, is_good) == false)
                continue;

            auto v = dim_table.get_table_item(r, c);
            if (expression_values_are_equal(v.getValue(), r.ref.distributionInfo->blockSize(r.dimensionIndex)) == true)
                continue;


            {
                ExpressionBase& block_size = r.ref.distributionInfo->blockSize(r.dimensionIndex);
                auto it = loops_for_nesting.begin();
                auto it2 = nesting_block_size.begin();
                for (; it != loops_for_nesting.end(); ++it, ++it2)
                {
                    if (*it == c.loopStmt)
                        break;
                }

                if (viewed_loops.count(c.loopStmt) == 0)
                {
                    // добавляем цикл в список циклов, предназначеннных для гнездования
                    loops_for_nesting.push_back(c.loopStmt);
                    nesting_block_size.push_back(&block_size);
                    viewed_loops.insert(c.loopStmt);
                }
                else
                {
                    if (it != loops_for_nesting.end() && expression_values_are_equal(*(*it2), block_size) == false)
                    {
                        // В случае, когда цикл нужно гнездовать по нескольким параметрам мы ничего не делаем.
                        // Пусть будет как есть. Сгененируем плохие индексы.
                        loops_for_nesting.erase(it);
                        nesting_block_size.erase(it2);
                    }
                }
            }
        }
    }

    OPS_ASSERT(loops_for_nesting.size() == nesting_block_size.size());
    int n = loops_for_nesting.size();

    std::vector<ForStatement*> loops1(n);
    std::vector<ExpressionBase*> blocks_sizes1(n);

    int k = 0;
    for (auto it = loops_for_nesting.begin(); it != loops_for_nesting.end(); ++it)
        loops1[k++] = *it;
    k = 0;
    for (auto it = nesting_block_size.begin(); it != nesting_block_size.end(); ++it)
        blocks_sizes1[k++] = *it;


    for (int i = 0; i < n-1; i++)
        for (int j = i+1; j < n; j++)
            if (OPS::Shared::contain(*loops1[i], *loops1[j]) == true)
            {
                ForStatement* b1 = loops1[i];
                loops1[i] = loops1[j];
                loops1[j] = b1;

                ExpressionBase* b2 = blocks_sizes1[i];
                blocks_sizes1[i] = blocks_sizes1[j];
                blocks_sizes1[j] = b2;
            }

    {
        for (int ind = 0; ind < n; ind++)
        {
            ForStatement& loop = *loops1[ind];
            ExpressionBase* d = blocks_sizes1[ind];

            OPS_ASSERT(check_loop(loop));

            OPS::Reprise::Declarations& declarations = loop.getRootBlock().getDeclarations();
            ReprisePtr<BlockStatement> body(&loop.getBody());
            ReprisePtr<ReferenceExpression> i(getBasicForCounter(loop).clone());
            const std::string iName = OPS::Strings::format("_d%s", i->getReference().getName().c_str());
            ReprisePtr<ExpressionBase> a(getBasicForInitExpression(loop).clone());
            ReprisePtr<ExpressionBase> b(getBasicForFinalExpression(loop).clone());

            BlockStatement* resultBlock = new BlockStatement();
            //updateLabel(replaceStatement(loop, ReprisePtr<StatementBase>(resultBlock)), *resultBlock);
            replaceStatement(loop, ReprisePtr<StatementBase>(resultBlock));
            ReprisePtr<ReferenceExpression> new_counter(new ReferenceExpression(createNewVariable(*i->getResultType(),
                     *resultBlock, iName)));


            ReprisePtr<ExpressionBase> f1cnt, f2cnt;
            f1cnt = new_counter;
            f2cnt = i;

            ReprisePtr<ExpressionBase> f1init, f2init;
            ReprisePtr<ExpressionBase> f1lim, f2lim;

            IntegerHelper c(i->getResultType()->cast_to<BasicType>());
            {
                {
                    SubroutineReferenceExpression* floor_func_ref = new SubroutineReferenceExpression(*floor_func);
                    SubroutineCallExpression* call_floor = new SubroutineCallExpression(floor_func_ref);
                    call_floor->addArgument(& (op(a) * (*StrictLiteralExpression::createFloat32(1.0)) / op(d)) );

                    f1init = ReprisePtr<ExpressionBase>(call_floor);
                    f2init = ReprisePtr<ExpressionBase>(&(Max(op(a) - (op(f1cnt) * op(d)) , c(0))) );
                }

                {
                    SubroutineReferenceExpression* ceil_func_ref = new SubroutineReferenceExpression(*ceil_func);
                    SubroutineCallExpression* call_ceil = new SubroutineCallExpression(ceil_func_ref);
                    call_ceil->addArgument(& (op(b) * (*StrictLiteralExpression::createFloat32(1.0)) / op(d)) );

                    f1lim = ReprisePtr<ExpressionBase>(call_ceil);
                    f2lim = ReprisePtr<ExpressionBase>(&(Min(op(b) - (op(f1cnt) * op(d)) , op(d))) );
                }
            }

            // 	{
             // 		...
             // 		> for (i1 = ...)
             // 		> {
             // 		> }
             // 	}
             VariableDeclaration* v1 = &(OPS::Reprise::Editing::createNewVariable(*BasicType::int32Type(), *resultBlock));
             ReprisePtr<ReferenceExpression> r1 = ReprisePtr<ReferenceExpression>(new ReferenceExpression(*v1));
             ReprisePtr<ExpressionBase> e1 = ReprisePtr<ExpressionBase>(&(op(*r1) R_AS op(*f1init)));
             resultBlock->addLast(new ExpressionStatement(e1->clone()));

             VariableDeclaration* v2 = &(OPS::Reprise::Editing::createNewVariable(*BasicType::int32Type(), *resultBlock));
             ReprisePtr<ReferenceExpression> r2 = ReprisePtr<ReferenceExpression>(new ReferenceExpression(*v2));
             ReprisePtr<ExpressionBase> e2 = ReprisePtr<ExpressionBase>(&(op(*r2) R_AS op(*f1lim)));
             resultBlock->addLast(new ExpressionStatement(e2->clone()));


             ForStatement* forStatement1 = new ForStatement(&(op(f1cnt) R_AS op(*r1)), &(op(f1cnt) < op(*r2)),
                 &(op(f1cnt) R_AS op(f1cnt) + c(1)));
             ReprisePtr<RepriseBase> e1_ptr(d->clone());
             forStatement1->setNote("BlockSize", Note::newReprise(e1_ptr));
             forStatement1->setNote("Kind", Note::newInt(LoopKinds::LoopByBlocks));
             OPS_ASSERT(check_loop(*forStatement1));

             resultBlock->addLast(forStatement1);
             BlockStatement* body1 = new BlockStatement();

             forStatement1->setBody(body1);
             //outerForStatement = forStatement1;

             // 	{
             // 		...
             // 		for (i1 = ...)
             // 		{
             // 			> for (i2 = ...)
             // 			> {
             // 			>	 ...
             // 			> }
             // 		}
             // 	}

             VariableDeclaration* v11 = &(OPS::Reprise::Editing::createNewVariable(*BasicType::int32Type(), *resultBlock));
             ReprisePtr<ReferenceExpression> r11 = ReprisePtr<ReferenceExpression>(new ReferenceExpression(*v11));
             ReprisePtr<ExpressionBase> e11 = ReprisePtr<ExpressionBase>(&(op(*r11) R_AS op(*f2init)));
             body1->addLast(new ExpressionStatement(e11->clone()));

             VariableDeclaration* v12 = &(OPS::Reprise::Editing::createNewVariable(*BasicType::int32Type(), *resultBlock));
             ReprisePtr<ReferenceExpression> r12 = ReprisePtr<ReferenceExpression>(new ReferenceExpression(*v12));
             ReprisePtr<ExpressionBase> e12 = ReprisePtr<ExpressionBase>(&(op(*r12) R_AS op(*f2lim)));
             body1->addLast(new ExpressionStatement(e12->clone()));

             ForStatement* forStatement2 = new ForStatement(&(op(f2cnt) R_AS op(*r11)), &(op(f2cnt) < op(*r12)),
                 &(op(f2cnt) R_AS op(f2cnt) + c(1)));
             ReprisePtr<RepriseBase> e2_ptr(d->clone());
             forStatement2->setNote("BlockSize", Note::newReprise(e2_ptr));
             forStatement2->setNote("Kind", Note::newInt(LoopKinds::LoopByBlockElements));
             body1->addLast(forStatement2);
             OPS_ASSERT(check_loop(*forStatement2));

             ReprisePtr<BlockStatement> body2 = OPS::Shared::cloneStatement(*body, declarations, declarations);
             //generateNewLabels(*body2);
             forStatement2->setBody(body2.get());
             //innerForStatement = forStatement2;

             SubstitudeVariableWalker substitute_walker(i, ReprisePtr<ExpressionBase>(&(op(i) + (op(f1cnt) * op(d)))));
             body2->accept(substitute_walker);
        }
    }

    return true;
}



void DataDistributionForSharedMemory::makeTransform(OPS::Reprise::ProgramUnit *program)
{
    m_currentTranslationUnit = &program->getUnit(0);

    Declarations::SubrIterator nextSubroutine = m_currentTranslationUnit->getGlobals().getFirstSubr();

    auto floor_func = m_currentTranslationUnit->getGlobals().findSubroutine("floor_kernel_func");
    auto ceil_func = m_currentTranslationUnit->getGlobals().findSubroutine("ceil_kernel_func");

    if (! (floor_func != 0 && ceil_func != 0) )
        return;

//    ArrayDistributionInfoList global_info_list = collectDataDistirubitionInfo();
//    for (auto it = global_info_list.begin(); it != global_info_list.end(); ++it)
//    {

//    }
    while (nextSubroutine.isValid())
    {
        SubroutineDeclaration* current_function = &*nextSubroutine;
        if (current_function->hasImplementation() == false)
        {
            ++nextSubroutine;
            continue;
        }


        //  собрать описание блочно размещенных массивов, объявленных в процедуре
        ArrayDistributionInfoList info_list = collectDataDistirubitionInfo(current_function);
        checkDataDistributionInfoCorrectness(info_list, current_function);
//        for (auto it = global_info_list.begin(); it != global_info_list.end(); ++it)
//            info_list.insert(*it);
        if (info_list.empty() == true)
        {
            ++nextSubroutine;
            continue;
        }

        SubroutineDeclaration* malloc_func = m_currentTranslationUnit->getGlobals().findSubroutine("malloc");
        SubroutineDeclaration* free_func = m_currentTranslationUnit->getGlobals().findSubroutine("free");
        for (auto it=info_list.begin(); it!=info_list.end(); ++it)
        {
            std::shared_ptr<ArrayDistributionInfo> info = *it;

            //  преобразуем директивы выделения памяти
            for (int i = 0; i < info->allocStmtCount(); i++)
                info->replaceAllocStmt(i, createAllocationStatement(malloc_func, info));

            //  преобразуем директивы освобождения памяти
            for (int i = 0; i < info->releaseStmtCount(); i++)
                info->replaceReleaseStmt(i, createReleaseStatement(free_func, info));
        }

        // те обращения к многомерным массивам, для которых не удалось выделить вектор-индексы
        {
            std::list<DistributedArrayReferenceInfo> references_without_multi_index;
            ReferenceTable dim_table;

            std::list<DistributedArrayReferenceInfo> array_refs = collectAllBlockArrayReferences(info_list, current_function);
            preprocess_distributed_array_references(references_without_multi_index, dim_table, array_refs, current_function);

            //print_table(dim_table, "/home/misha/my_temp/dump_A.csv");
            nest_loops(dim_table);
            // OPS::IO::writeFile("/home/misha/my_temp/dump_1.csv", m_currentTranslationUnit->dumpState());
        }

        {
            std::list<DistributedArrayReferenceInfo> references_without_multi_index;
            ReferenceTable dim_table;

            std::list<DistributedArrayReferenceInfo> array_refs = collectAllBlockArrayReferences(info_list, current_function);
            preprocess_distributed_array_references(references_without_multi_index, dim_table, array_refs, current_function);
            // print_table(dim_table, "/home/misha/my_temp/dump_B.csv");

            change_references(references_without_multi_index, dim_table, array_refs, current_function);
            // print_table(dim_table, "/home/misha/my_temp/dump_B.csv");
        }


        ++nextSubroutine;
    }
}
}
}
}

