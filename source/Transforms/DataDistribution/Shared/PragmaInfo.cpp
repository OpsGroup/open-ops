#include "Reprise/Declarations.h"

#include "OPS_Core/OPS_Core.h"
#include "Reprise/Service/DeepWalker.h"
#include "include/PragmaInfo.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace std;
using namespace OPS::Reprise;
using namespace OPS::Transforms;


//
//      PragmaInfo
//
PragmaInfo::PragmaInfo()
{
}
VariableDeclaration* PragmaInfo::arrayInfo()
{
    return m_decl;
}
void PragmaInfo::setArrayInfo(VariableDeclaration* decl)
{
    m_decl = decl;
}


//
//      DeclarePragmaInfo
//
DeclarePragmaInfo::DeclarePragmaInfo()
{
}
void DeclarePragmaInfo::setArrayDimensionCount(int dimCount)
{
    m_dimCount = dimCount;
    m_blockSizeList.resize(dimCount);
    m_dimSizeList.resize(dimCount);
}

const ExpressionBase& DeclarePragmaInfo::dimensionSize(int index)
{
    OPS_ASSERT(isValid());
    OPS_ASSERT(index>=0 && index < m_dimCount);
    return *m_dimSizeList[index];
}
void DeclarePragmaInfo::setDimensionSize(OPS::Reprise::ReprisePtr<ExpressionBase> expr, int index)
{
    m_dimSizeList[index] = expr;
}

const ExpressionBase& DeclarePragmaInfo::blockSize(int index)
{
    OPS_ASSERT(isValid());
    OPS_ASSERT(index>=0 && index < m_dimCount);
    return *m_blockSizeList[index];
}
void DeclarePragmaInfo::setBlockSize(OPS::Reprise::ReprisePtr<ExpressionBase> expr, int index)
{
    m_blockSizeList[index] = expr;
}

bool DeclarePragmaInfo::isValid()
{
    return m_dimCount > 0;
}

//
//      AllocatePragmaInfo
//
AllocatePragmaInfo::AllocatePragmaInfo()
{
}

//
//      ReleasePragmaInfo
//
ReleasePragmaInfo::ReleasePragmaInfo()
{
}


class PragmasFinder : public Service::DeepWalker
{
public:
    using Service::DeepWalker::visit;
    list<VariableDeclaration*> m_declNodeList;
    list<StatementBase*> m_allocNodeList, m_releaseNodeList;

    void visit(OPS::Reprise::VariableDeclaration& node)
    {
        if (node.hasNote("block_array_declare"))
            m_declNodeList.push_back(&node);
        Service::DeepWalker::visit(node);
    }


#define VISIT_STMT(StmtType) \
    void visit(StmtType& node) \
    {		\
        if (node.hasNote("block_array_allocate")) \
            m_allocNodeList.push_back(&node); \
    if (node.hasNote("block_array_release")) \
        m_releaseNodeList.push_back(&node); \
        Service::DeepWalker::visit(node); \
    }


    VISIT_STMT(BlockStatement)
    VISIT_STMT(ForStatement)
    VISIT_STMT(WhileStatement)
    VISIT_STMT(IfStatement)
    VISIT_STMT(GotoStatement)
    VISIT_STMT(ReturnStatement)
    VISIT_STMT(Canto::HirBreakStatement)
    VISIT_STMT(Canto::HirContinueStatement)
    VISIT_STMT(ExpressionStatement)
    VISIT_STMT(EmptyStatement)
};

std::list<std::tr1::shared_ptr<PragmaInfo> > findAllPragmas(OPS::Reprise::SubroutineDeclaration* func)
{
    std::list<std::tr1::shared_ptr<PragmaInfo> > result;
/*
    PragmasFinder visitor;
    func->accept(visitor);
    for (std::list<StatementBase*>::iterator it = visitor.m_nodeList.begin(); it != visitor.m_nodeList.end(); ++it)
    {
        parsePragma(result, *it);
    }
*/
    return result;
}

}
}
}

