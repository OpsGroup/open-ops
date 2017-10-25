#ifndef PRAGMA_INFO_H
#define PRAGMA_INFO_H

#include <memory>
#include <vector>

#include "OPS_Core/MemoryHelper.h"

#include "Reprise/Utils.h"
#include "Reprise/Declarations.h"
#include "Reprise/Types.h"
#include "Reprise/Expressions.h"


namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using OPS::Reprise::StatementBase;
using OPS::Reprise::VariableDeclaration;
using OPS::Reprise::ExpressionBase;
using OPS::Reprise::SubroutineDeclaration;
using OPS::Reprise::BasicType;
using OPS::Reprise::ReprisePtr;
using OPS::Reprise::ReferenceExpression;


class ArrayDistributionInfo
{
public:
    ArrayDistributionInfo() {}

    void setArrayInfo(VariableDeclaration* decl);
    VariableDeclaration* arrayInfo();

    BasicType& arrayType();

    int dimensionCount();
    void setDimensionCount(int dimCount);

    ReferenceExpression& dimensionSize(int index);
    VariableDeclaration& dimensionSizeReference(int index);
    void setDimensionSizeReference(VariableDeclaration* expr, int index);

    ReferenceExpression& blockSize(int index);
    VariableDeclaration& blockSizeReference(int index);
    void setBlockSizeReference(VariableDeclaration* var, int index);

    ReferenceExpression& distributedArrayDimensionSize(int index);
    VariableDeclaration& distributedArrayDimensionSizeReference(int index);
    ExpressionBase* calculateDistributedArraySize();


    void addAllocStmt(StatementBase* stmt);
    std::vector<OPS::Reprise::StatementBase*> allocStmtList();
    int allocStmtCount();
    void replaceAllocStmt(int index, StatementBase* newStmt);

    void addReleaseStmt(StatementBase* stmt);
    std::vector<OPS::Reprise::StatementBase*> releaseStmtList();
    int releaseStmtCount();
    void replaceReleaseStmt(int index, StatementBase* newStmt);


    ExpressionBase& linear_d_coef(int index);
    ExpressionBase& linear_i_coef(int index);
private:
    void initialize_coefs();
    void initializeArrayDimensionSizeVariables();

    OPS::Reprise::VariableDeclaration* m_decl;
    int m_dimCount;
    std::vector<VariableDeclaration*> m_dimSizeList, m_blockSizeList;
    std::vector<ReprisePtr<ReferenceExpression> > m_dimSizeListRefs, m_blockSizeListRefs;

    std::vector<ReprisePtr<ReferenceExpression> > m_distrArrayBlockDimSizeRefs;
    std::vector<VariableDeclaration*> m_distrArrayBlockDimSizeVars;

    std::vector<StatementBase*> m_allocStmtList, m_releaseStmtList;
    std::vector<ReprisePtr<ExpressionBase> > d_coefs_refs, i_coefs_refs;

};

typedef std::list<std::shared_ptr<ArrayDistributionInfo> > ArrayDistributionInfoList;

ArrayDistributionInfoList collectDataDistirubitionInfo(OPS::Reprise::SubroutineDeclaration* decl);
void checkDataDistributionInfoCorrectness(ArrayDistributionInfoList& infoList, OPS::Reprise::SubroutineDeclaration* decl);
std::list<OPS::Reprise::ReferenceExpression*> findDistributedArrayReferences(ArrayDistributionInfoList& infoList,
                                                                             OPS::Reprise::SubroutineDeclaration* decl);

}
}
}

#endif

