#ifndef REFERENCE_TABLE_H
#define REFERENCE_TABLE_H

#include <memory>
#include <deque>
#include "ArrayDistributionInfo.h"

#include "Reprise/Service/DeepWalker.h"

#include "shared_helpers.h"
#include "DistributedArrayReferenceInfo.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using OPS::Reprise::ExpressionBase;
using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::ReferenceExpression;
using OPS::Reprise::VariableDeclaration;
using OPS::Reprise::ForStatement;
using OPS::Reprise::Service::DeepWalker;
using OPS::Reprise::ReprisePtr;

class ReferenceTable
{
public:
    struct Column
    {
        OPS::Reprise::ForStatement* loopStmt;

        Column();
        Column(OPS::Reprise::ForStatement* loopStmt);

        bool operator==(const ReferenceTable::Column& i2) const;
        bool operator<(const ReferenceTable::Column& i2) const;
    };

    struct Row
    {
        DistributedArrayReferenceInfo ref;
        int dimensionIndex;

        Row();
        Row(DistributedArrayReferenceInfo source, int dimensionIndex);

        bool operator!=(const ReferenceTable::Row& i2) const;
        bool operator==(const ReferenceTable::Row& i2) const;
        bool operator<(const ReferenceTable::Row& i2)  const;
    };

    struct Index
    {
        Column col;
        Row row;

        Index() {}

        inline bool operator==(const ReferenceTable::Index& i2) const {
            return col == i2.col && row == i2.row;
        }

    };


    struct Value
    {
        enum Type
        {
            Literal,
            Reference
        };

        static Value makeInt(int val);
        static Value makeRef(ReferenceExpression* ref);

        Type getType();
        ExpressionBase& getValue();
        std::string toString();

    private:
        ReprisePtr<ExpressionBase> m_val;
        Type m_type;
    };

    void insert_table_item(Row row, Column col, Value data);
    std::set<Row> table_rows();
    std::set<Column> table_cols();
    bool contains_table_item(Row row, Column col);
    Value get_table_item(Row row, Column col);

    void add_not_optimizable_dim(Row row, OPS::Reprise::ReprisePtr<ExpressionBase> c);

    std::set<Row> not_optimizable_rows();
    bool contains_not_optimizable_item(Row row);
    ExpressionBase& not_optimizable_item(Row row);


 private:
    std::list<std::pair<Index, Value> > table;
    std::list<std::pair<Row, OPS::Reprise::ReprisePtr<ExpressionBase> > > not_optimizable_dims;
};

enum LoopKinds
{
    LoopByBlocks = 1,
    LoopByBlockElements = 2
};

bool row_is_good(ReferenceTable::Row& r, ReferenceTable& dim_table);
bool row_needs_optimization(ReferenceTable::Row& r, ReferenceTable& dim_table, bool& is_good);

}
}
}

#endif
