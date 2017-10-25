#include "include/change_references_algorithms.h"


#include "Shared/SubroutinesShared.h"
#include "Shared/DataShared.h"
#include "Shared/LinearExpressions.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/StatementsShared.h"

#include <iterator>

using namespace std;
using namespace OPS::Shared;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Editing;
using namespace OPS::Reprise::Canto;
using namespace OPS::Transforms;
using namespace OPS::Shared::ExpressionHelpers;

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

struct LoopInfluenceVector
{
    // A[a1*d1 + b1, a2*d2 + b2, ..., an*dn + bn]
    VariableDeclaration* counter;
    int n;
    std::vector<bool> by_blocks;
    std::vector<ReprisePtr<ExpressionBase> > e;
};

void printLoopInfluenceVector(LoopInfluenceVector& v1)
{
    cout << "LoopInfluenceVector: " << v1.counter->dumpState() << endl;
    cout << "n = " << v1.n << endl;
    for (int i = 0; i < v1.n; i++)
    {
        if (v1.e[i].get() != 0)
            cout << "i=" << i << "; by_blocks=" << v1.by_blocks[i] << "; e=" << v1.e[i]->dumpState() << endl;
    }
}

LoopInfluenceVector get_loop_influence_vector(ReferenceTable& dim_table, ForStatement* loop, const DistributedArrayReferenceInfo& ref)
{
    LoopInfluenceVector result;
    result.counter = getLoopCounter(loop);
    result.n = ref.distributionInfo->dimensionCount();
    result.e.resize(result.n);
    result.by_blocks.resize(result.n, false);

    ReferenceTable::Column col(loop);
    auto rows = dim_table.table_rows();
    for (auto r=rows.begin(); r!=rows.end(); ++r)
    {
        ReferenceTable::Row row = *r;
        if (row.ref != ref || dim_table.contains_table_item(row, col) == false || row_is_good(row, dim_table) == false)
            continue;

        ReferenceExpression& d = row.ref.distributionInfo->blockSize(row.dimensionIndex);
        auto v = dim_table.get_table_item(row, col);
        if (expression_is_devided_on(v.getValue(), d) == true)
        {
            OPS_ASSERT(result.e[row.dimensionIndex].get() == 0);
            result.e[row.dimensionIndex] = ReprisePtr<ExpressionBase>(get_quotient_expression(v.getValue(), d));
            result.by_blocks[row.dimensionIndex] = true;
        }
        else
        {
            OPS_ASSERT(col.loopStmt->hasNote("Kind") == true);
            auto kind = (LoopKinds)col.loopStmt->getNote("Kind").getInt();
            OPS_ASSERT(kind == LoopKinds::LoopByBlockElements);

            ExpressionBase& buf = col.loopStmt->getNote("BlockSize").getReprise().cast_to<ExpressionBase>();
            OPS_ASSERT(expression_values_are_equal(buf, d));

            result.e[row.dimensionIndex] = ReprisePtr<ExpressionBase>(v.getValue().clone());
            result.by_blocks[row.dimensionIndex] = false;
        }
    }

    return result;

}

bool influence_vectors_are_equal(LoopInfluenceVector& v1, LoopInfluenceVector& v2)
{
    if (v1.n != v2.n)
        return false;

    for (int i=0; i<v1.n; i++)
        if (v1.by_blocks[i] != v2.by_blocks[i])
            return false;

    for (int i=0; i<v1.n; i++)
    {
        bool p1 = v1.e[i].get() != 0;
        bool p2 = v1.e[i].get() != 0;

        if (p1 != p2)
            return false;
        if (p1 == true && expression_values_are_equal(*v1.e[i], *v2.e[i]) == false)
            return false;
    }

    return true;
}

ExpressionBase* calculate_column_influence_vector(LoopInfluenceVector& v1, std::shared_ptr<ArrayDistributionInfo> info)
{
    ExpressionBase* result = 0;
    for (int i = 0; i < v1.n; i++)
    {
        if (v1.e[i].get() == 0)
            continue;

        if (v1.by_blocks[i] == true)
            result = &(result == 0 ? op(*v1.e[i]) * op(info->linear_d_coef(i)) : (*result) + op(*v1.e[i]) * op(info->linear_d_coef(i)));
        else
            result = &(result == 0 ? op(*v1.e[i]) * op(info->linear_i_coef(i)) : (*result) + op(*v1.e[i]) * op(info->linear_i_coef(i)));

    }
    if (result != 0)
    {
        ReferenceExpression* counter = new ReferenceExpression(*v1.counter);
        result = &((*result) * (*counter));
    }

    return result;
}

ExpressionBase* create_row_influence_expr(ReferenceTable& dim_table, ReferenceTable::Row& row)
{
    ExpressionBase* result = 0;

    auto info = row.ref.distributionInfo;
    ExpressionBase& a = info->linear_d_coef(row.dimensionIndex);
    ExpressionBase& b = info->linear_i_coef(row.dimensionIndex);


    ReferenceExpression& d = info->blockSize(row.dimensionIndex);

    ExpressionBase *d_comp = 0, *i_comp = 0;
    if (dim_table.contains_not_optimizable_item(row) == true)
    {

        ExpressionBase& e = dim_table.not_optimizable_item(row);

        d_comp = &(op(e) / op(d));
        i_comp = &(op(e) % op(d));
    }
    else
    {
        auto cols = dim_table.table_cols();
        for (auto it = cols.begin(); it != cols.end(); ++it)
        {
            ReferenceTable::Column col = *it;
            if (dim_table.contains_table_item(row, col) == false)
                continue;

            bool by_blocks = false;
            if (col.loopStmt != 0 && col.loopStmt->hasNote("Kind") == true)
            {
                auto kind = (LoopKinds)col.loopStmt->getNote("Kind").getInt();
                if (kind == LoopKinds::LoopByBlocks)
                    by_blocks = true;
            }

            ExpressionBase& next_item = dim_table.get_table_item(row, col).getValue();
            ExpressionBase* add_value = 0;
            if (by_blocks == true)
            {
                OPS_ASSERT(expression_is_devided_on(next_item, d) == true);
                add_value = get_quotient_expression(next_item, d);
            }
            else
                add_value = &(op(next_item));

            if (col.loopStmt != 0)
                add_value = &((*add_value)*op(*(new ReferenceExpression(*getLoopCounter(col.loopStmt)))));
            if (by_blocks == true)
                d_comp = &(d_comp == 0 ? (*add_value) : (*d_comp) + (*add_value));
            else
                i_comp = &(i_comp == 0 ? (*add_value) : (*i_comp) + (*add_value));
        }

    }

    if (i_comp != 0 && row_is_good(row, dim_table) == false)
    {
        ExpressionBase* quo_value = &(op(i_comp) / op(d));
        ExpressionBase* rest_value = &(op(i_comp) % op(d));

        d_comp = &(d_comp == 0 ? (*quo_value) : (*d_comp) + (*quo_value));

        delete i_comp;
        i_comp = rest_value;
    }

    if (d_comp != 0)
        result = result == 0 ? &((*d_comp)*op(a)) : &((*result) + (*d_comp)*op(a));
    if (i_comp != 0)
        result = result == 0 ? &((*i_comp)*op(b)) : &((*result) + (*i_comp)*op(b));
    return result;
}

void change_references(std::list<DistributedArrayReferenceInfo>& references_without_multi_index, ReferenceTable& dim_table, std::list<DistributedArrayReferenceInfo>& array_distrib_refs_list,
                                             SubroutineDeclaration* current_function)
{
    auto cols = dim_table.table_cols();
    auto rows = dim_table.table_rows();
    auto not_optimizable_rows = dim_table.not_optimizable_rows();

    int k = 0;
    std::vector<ForStatement*> sorted_loops(cols.size());
    for (auto it = cols.begin(); it != cols.end(); ++it)
    {
        ReferenceTable::Column col = *it;
        if (col.loopStmt != 0)
            sorted_loops[k++] = col.loopStmt;
    }
    sorted_loops.resize(k);
    if (sorted_loops.size() > 0)
    {
        for (int i = 0; i < sorted_loops.size()-1; i++)
            for (int j = i+1; j < sorted_loops.size(); j++)
                if (OPS::Shared::contain(*sorted_loops[j], *sorted_loops[i]) == true)
                {
                    ForStatement* b1 = sorted_loops[i];
                    sorted_loops[i] = sorted_loops[j];
                    sorted_loops[j] = b1;
                }
    }


    map<DistributedArrayReferenceInfo, VariableDeclaration*> ref_2_last_representitive;
    for (auto r=rows.begin(); r != rows.end(); ++r)
    {
        ReferenceTable::Row row = *r;
        ref_2_last_representitive[row.ref] = row.ref.distributionInfo->arrayInfo();
    }
    for (auto r=not_optimizable_rows.begin(); r != not_optimizable_rows.end(); ++r)
    {
        ReferenceTable::Row row = *r;
        ref_2_last_representitive[row.ref] = row.ref.distributionInfo->arrayInfo();
    }

    for (auto loop_it=sorted_loops.begin(); loop_it != sorted_loops.end(); ++loop_it)
    {
        ForStatement* loop = *loop_it;

        set<DistributedArrayReferenceInfo> loop_affected_refs;
        for (auto r=rows.begin(); r!=rows.end(); ++r)
        {
            ReferenceTable::Row row = *r;
            if (row_is_good(row, dim_table) == false)
                continue;
            if (dim_table.contains_table_item(row, ReferenceTable::Column(loop)))
                loop_affected_refs.insert(row.ref);
        }

        set<DistributedArrayReferenceInfo> viewed_loop_affected_refs;
        while (viewed_loop_affected_refs.size() < loop_affected_refs.size())
        {
            set<DistributedArrayReferenceInfo> diff;
            //  следующее непросмотренное обращение (Next Ref) к массиву, индексное выражение которого зависит от текущего цикла
            set_difference(loop_affected_refs.begin(), loop_affected_refs.end(), viewed_loop_affected_refs.begin(), viewed_loop_affected_refs.end(), inserter(diff, diff.end()));
            DistributedArrayReferenceInfo not_viewed_loop_affected_ref = *diff.begin();

            auto v1 = get_loop_influence_vector(dim_table, loop,  not_viewed_loop_affected_ref);
            printLoopInfluenceVector(v1);

            set<DistributedArrayReferenceInfo> refs_group;
            refs_group.insert(not_viewed_loop_affected_ref);
            for (auto it = diff.begin(); it != diff.end(); ++it)
            {
                if (ref_2_last_representitive[*it] != ref_2_last_representitive[not_viewed_loop_affected_ref])
                    continue;
                auto v2 = get_loop_influence_vector(dim_table, loop, *it);
                if (influence_vectors_are_equal(v1, v2) == false)
                    continue;
                refs_group.insert(*it);
            }

            VariableDeclaration* prev_representitive = ref_2_last_representitive[not_viewed_loop_affected_ref];
            VariableDeclaration& new_representitive = Editing::createNewVariable(prev_representitive->getType(), loop->getBody());
            ExpressionStatement* assign = new ExpressionStatement(&(*(new ReferenceExpression(new_representitive))
                R_AS *(new ReferenceExpression(*prev_representitive)) + *(calculate_column_influence_vector(v1, not_viewed_loop_affected_ref.distributionInfo)) ));
            loop->getBody().addFirst(assign);

            for (auto it2 = refs_group.begin(); it2 != refs_group.end(); ++it2)
                ref_2_last_representitive[*it2] = &new_representitive;

            std::set_union(viewed_loop_affected_refs.begin(), viewed_loop_affected_refs.end(),
                           refs_group.begin(), refs_group.end(),
                           inserter(viewed_loop_affected_refs, viewed_loop_affected_refs.end()));
        }
    }


    set<DistributedArrayReferenceInfo> refs_for_updating;
    for (auto r=rows.begin(); r!=rows.end(); ++r)
    {
        ReferenceTable::Row row = *r;
        if (refs_for_updating.count(row.ref) == 0)
            refs_for_updating.insert(row.ref);
    }
    for (auto r=not_optimizable_rows.begin(); r!=not_optimizable_rows.end(); ++r)
    {
        ReferenceTable::Row row = *r;
        if (refs_for_updating.count(row.ref) == 0)
            refs_for_updating.insert(row.ref);
    }

    for (auto refi=refs_for_updating.begin(); refi != refs_for_updating.end(); ++refi)
    {
        DistributedArrayReferenceInfo next_ref = *refi;
        ExpressionBase* new_ref = new ReferenceExpression(*ref_2_last_representitive[next_ref]);

        for (auto r=rows.begin(); r!=rows.end(); ++r)
        {
            ReferenceTable::Row row = *r;
            if (row.ref != next_ref)
                continue;
            if (row_is_good(row, dim_table) == true)
                continue;

            ExpressionBase* row_influence_expr = create_row_influence_expr(dim_table, row);
            new_ref = &(op(new_ref) + op(row_influence_expr));
        }

        for (auto r=not_optimizable_rows.begin(); r!=not_optimizable_rows.end(); ++r)
        {
            ReferenceTable::Row row = *r;
            if (row.ref != next_ref)
                continue;
            ExpressionBase* row_influence_expr = create_row_influence_expr(dim_table, row);
            new_ref = &(op(new_ref) + op(row_influence_expr));
        }



        ref_2_last_representitive.erase(next_ref);
        new_ref = new BasicCallExpression(BasicCallExpression::BCK_DE_REFERENCE, new_ref);
        Editing::replaceExpression(*next_ref.source, ReprisePtr<ExpressionBase>(new_ref));
    }
    
    for (auto refi = references_without_multi_index.begin(); refi != references_without_multi_index.end(); ++refi)
    {
        DistributedArrayReferenceInfo next_ref = *refi;
        int dim = next_ref.distributionInfo->dimensionCount();


        std::vector<ReprisePtr<ExpressionBase> > di_list(dim);
        std::vector<ReprisePtr<ExpressionBase> > ii_list(dim);

        ReprisePtr<ExpressionBase> current_expr = ReprisePtr<ExpressionBase>(next_ref.source->getArgument(1).clone());
        for (int i = 0; i < dim; i++)
        {
            ExpressionBase* div_expr = 0;
            for (int j = i+1; j< dim; j++)
                if (div_expr == 0)
                    div_expr = next_ref.distributionInfo->dimensionSize(j).clone();
                else
                    div_expr = &(op(*div_expr) * op(*next_ref.distributionInfo->dimensionSize(j).clone()));

            if (div_expr != 0)
            {
                ExpressionBase* index_expr = &(op(*current_expr) / op(*div_expr));
                di_list[i] = ReprisePtr<ExpressionBase>( &(op(*index_expr) / op(next_ref.distributionInfo->blockSize(i))) );
                ii_list[i] = ReprisePtr<ExpressionBase>( &(op(*index_expr) % op(next_ref.distributionInfo->blockSize(i))) );

                current_expr = ReprisePtr<ExpressionBase>( &(op(*current_expr) % op(*div_expr)) );
                delete index_expr;
                delete div_expr;
            }
            else
            {
                di_list[i] = ReprisePtr<ExpressionBase>( &(op(*current_expr) / op(next_ref.distributionInfo->blockSize(i))) );
                ii_list[i] = ReprisePtr<ExpressionBase>( &(op(*current_expr) % op(next_ref.distributionInfo->blockSize(i))) );
            }


        }

        ExpressionBase* result = 0;
        for (int i = 0; i< dim; i++)
        {
            ExpressionBase& a = next_ref.distributionInfo->linear_d_coef(i);
            ExpressionBase& b = next_ref.distributionInfo->linear_i_coef(i);

            if (result == 0)
                result = &(op(*di_list[i])*op(a) + op(ii_list[i]) * op(b));
            else
                result = &((*result) + op(*di_list[i]) * op(a) + op(*ii_list[i]) * op(b));
        }

        ExpressionBase* new_ref = &(*(new ReferenceExpression(*next_ref.distributionInfo->arrayInfo())) R_BK (op(result)));
        Editing::replaceExpression(*next_ref.source, ReprisePtr<ExpressionBase>(new_ref));

    }
}


}
}
}
