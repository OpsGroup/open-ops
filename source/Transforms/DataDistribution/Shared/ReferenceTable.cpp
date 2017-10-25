#include "Reprise/Reprise.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>

#include "include/ReferenceTable.h"
#include "include/shared_helpers.h"

using namespace OPS::Reprise::Editing;
using namespace OPS::Reprise;
using namespace std;


namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

ReferenceTable::Column::Column()
{

}
ReferenceTable::Column::Column(OPS::Reprise::ForStatement* loopStmt)
{
    this->loopStmt = loopStmt;
}

bool ReferenceTable::Column::operator==(const ReferenceTable::Column& i2) const
{
    if (loopStmt == 0 || i2.loopStmt == 0)
        return loopStmt == i2.loopStmt;
    return loopStmt->getNCID() == i2.loopStmt->getNCID();
}
bool ReferenceTable::Column::operator<(const ReferenceTable::Column& i2) const
{
    if (loopStmt == 0 || i2.loopStmt == 0)
    {
        if (loopStmt == i2.loopStmt)
            return false;
        if (loopStmt == 0)
            return true;
        return false;
    }
    return loopStmt->getNCID() < i2.loopStmt->getNCID();
}

ReferenceTable::Row::Row()
{
}
ReferenceTable::Row::Row(DistributedArrayReferenceInfo source, int dimensionIndex)
{
    this->ref = source;
    this->dimensionIndex = dimensionIndex;
}

bool ReferenceTable::Row::operator!=(const ReferenceTable::Row& i2) const {
    return !(*this == i2);
}
bool ReferenceTable::Row::operator==(const ReferenceTable::Row& i2) const {
    return dimensionIndex == i2.dimensionIndex
            && ref.source->getNCID() == i2.ref.source->getNCID();
}
bool ReferenceTable::Row::operator<(const ReferenceTable::Row& i2)  const {
    if (dimensionIndex < i2.dimensionIndex)
        return true;
    return ref.source->getNCID() < i2.ref.source->getNCID();
}

ReferenceTable::Value ReferenceTable::Value::makeInt(int val)
{
    ReferenceTable::Value v;
    v.m_type = ReferenceTable::Value::Literal;
    v.m_val = ReprisePtr<ExpressionBase>(StrictLiteralExpression::createInt32(val));
    return v;
}
ReferenceTable::Value ReferenceTable::Value::makeRef(ReferenceExpression* val)
{
    ReferenceTable::Value v;
    v.m_type = ReferenceTable::Value::Reference;
    v.m_val = ReprisePtr<ExpressionBase>(val);
    return v;
}

ReferenceTable::Value::Type ReferenceTable::Value::getType()
{
   return m_type;
}
ExpressionBase& ReferenceTable::Value::getValue()
{
    return *m_val;
}

std::string ReferenceTable::Value::toString()
{
    if (m_type == ReferenceTable::Value::Literal)
        return std::to_string(m_val->cast_to<StrictLiteralExpression>().getInt32());
    return m_val->cast_to<ReferenceExpression>().getReference().getName();
}

void ReferenceTable::insert_table_item(ReferenceTable::Row row, ReferenceTable::Column col, ReferenceTable::Value value)
{
    ReferenceTable::Index ind;
    if (col.loopStmt != 0)
        OPS_ASSERT(col.loopStmt->is_a<ForStatement>() == true);
    ind.col = col;
    ind.row = row;

    std::pair<ReferenceTable::Index, ReferenceTable::Value> p = std::pair<ReferenceTable::Index, ReferenceTable::Value>(ind, value);
    table.push_back(p);
}

std::set<ReferenceTable::Row> ReferenceTable::table_rows()
{
    std::set<ReferenceTable::Row> result;
    for (auto it = table.begin(); it != table.end(); ++it)
        result.insert(it->first.row);

    return result;
}
std::set<ReferenceTable::Column> ReferenceTable::table_cols()
{
    std::set<ReferenceTable::Column> result;
    for (auto it = table.begin(); it != table.end(); ++it)
        result.insert(it->first.col);

    return result;
}
bool ReferenceTable::contains_table_item(ReferenceTable::Row row, ReferenceTable::Column col)
{
    Index ind;
    ind.row = row;
    ind.col = col;

    for (auto it = table.begin(); it != table.end(); ++it)
        if (it->first == ind)
            return true;

    return false;
}
ReferenceTable::Value ReferenceTable::get_table_item(ReferenceTable::Row row, ReferenceTable::Column col)
{
    OPS_ASSERT(contains_table_item(row, col) == true);

    Index ind;
    ind.row = row;
    ind.col = col;

    for (auto it = table.begin(); it != table.end(); ++it)
        if (it->first == ind)
            return it->second;

    throw std::runtime_error("get_table_item");
}

void ReferenceTable::add_not_optimizable_dim(Row row, OPS::Reprise::ReprisePtr<ExpressionBase> c)
{
    auto p = std::make_pair(row, c);
    not_optimizable_dims.push_back(p);
}

std::set<ReferenceTable::Row> ReferenceTable::not_optimizable_rows()
{
    std::set<ReferenceTable::Row> result;
    for (auto it = not_optimizable_dims.begin(); it != not_optimizable_dims.end(); ++it)
        result.insert(it->first);

    return result;
}
bool ReferenceTable::contains_not_optimizable_item(ReferenceTable::Row row)
{
    for (auto it = not_optimizable_dims.begin(); it != not_optimizable_dims.end(); ++it)
        if (it->first == row)
            return true;

    return false;
}
ExpressionBase& ReferenceTable::not_optimizable_item(ReferenceTable::Row row)
{
    OPS_ASSERT(contains_not_optimizable_item(row) == true);

    for (auto it = not_optimizable_dims.begin(); it != not_optimizable_dims.end(); ++it)
        if (it->first == row)
            return *it->second;

    throw std::runtime_error("get_table_item");
}

// *************************************************************************************

//      Можно ли данную строку преобразовать (или он уже преобразован) в хорошую
bool row_is_good(ReferenceTable::Row& r, ReferenceTable& dim_table)
{
    bool is_good = true;
    row_needs_optimization(r, dim_table, is_good);
    return is_good;
}

//      Требуется ли для данной строки оптимизация
bool row_needs_optimization(ReferenceTable::Row& r, ReferenceTable& dim_table, bool& is_good)
{
    cout << "row_needs_optimization" << endl;
    cout << r.ref.source->dumpState() << endl;

    ReferenceExpression& d = r.ref.distributionInfo->blockSize(r.dimensionIndex);
    cout << "d = " << d.dumpState() << endl;
    if (expression_value_is(d, 1) == true)
    {
        is_good = true;
        return false;
    }

    int bad_columns_count = 0;
    int range_columns_count = 0;

    auto cols = dim_table.table_cols();
    for (auto itc = cols.begin(); itc != cols.end(); ++itc)
    {
        ReferenceTable::Column c = *itc;
        if (dim_table.contains_table_item(r, c) == false)
            continue;

        // не допускаются выражения вида "e + 1"
        if (c.loopStmt == 0)
        {
            is_good = false;
            return false;
        }

        cout << "next loop : " << c.loopStmt->dumpState() << endl;

        auto v = dim_table.get_table_item(r, c);

        cout << "v : " << v.getValue().dumpState() << endl;
        if (expression_is_devided_on(v.getValue(), d) == true)
            continue;

        if (c.loopStmt->hasNote("Kind") == true && c.loopStmt->getNote("Kind").getInt() == LoopByBlockElements
                && expression_values_are_equal(c.loopStmt->getNote("BlockSize").getReprise().cast_to<ExpressionBase>(), d))
            ++range_columns_count;
        else
        {
            if (! (v.getType() == ReferenceTable::Value::Literal && v.getValue().cast_to<StrictLiteralExpression>().getInt32() == 1 && check_loop(*c.loopStmt) == true) )
            {
                is_good = false;
                return false;
            }

            ++bad_columns_count;
        }
        if (bad_columns_count + range_columns_count >= 2)
        {
            is_good = false;
            return false;
        }
    }

    is_good = bad_columns_count == 0 && range_columns_count <= 1;
    return true;
}




// *************************************************************************************


// вхождение размещаемого массива
struct ArrayOccurence
{
public:

    BasicCallExpression* exprPtr;

    //	рассматривается выражение в размерности с номером dim
    //	обращение к массиву	A[..] .. [i + freeCoef]..[..].
    OPS::Reprise::ReprisePtr<BasicCallExpression> expr;
    OPS::Reprise::VariableDeclaration* arrayDecl();

    //	counterDecl = i
    VariableDeclaration* counterDecl;
    OPS::Reprise::ReprisePtr<ExpressionBase> freeCoef;

    //		freeCoef = aCoeft*d + bCoef
    //		0 <= bCoef < d
    OPS::Reprise::ReprisePtr<ExpressionBase> aCoef;
    OPS::Reprise::ReprisePtr<ExpressionBase> bCoef;


    //	number of dimension
    int dim;

};

struct ForStmtTree
{
public:
    struct Node
    {
    public:
        //	Может быть равным null в случае, когда это корень дерева
        ForStatement* forStmt;

        //	Массив непосредственных потомков
        std::list<int> childs;

        //	Непосредственный родитель
        int parent;

        std::list<ArrayOccurence> occuranceTable;

    private:
        VariableDeclaration* m_commonBlockSizeVariable;
    };

    std::list<Node> nodes;
};

/*
void ReferenceTableCreator::visit(BasicCallExpression& node)
{
    DeepWalker::visit(node);
}

void ReferenceTableCreator::visit(ForStatement& node)
{

    ForStmtTree::Node nextNode;
    nextNode.forStmt = &node;
    nextNode.parent = m_forStmtStack.back();

    m_tree->nodes.push_back(nextNode);

    m_forStmtStack.push_back(m_tree->nodes.size() - 1);
    DeepWalker::visit(node);
    m_forStmtStack.pop_back();
}
*/

}
}
}
