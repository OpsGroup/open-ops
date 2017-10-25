#ifndef PRAGMA_INFO_H
#define PRAGMA_INFO_H

#include <vector>

#include "Reprise/Utils.h"
#include "OPS_Core/MemoryHelper.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using OPS::Reprise::VariableDeclaration;
using OPS::Reprise::ExpressionBase;
using OPS::Reprise::SubroutineDeclaration;


class ExpressionBase;

class PragmaInfo
{
public:
    enum Type
    {
        Declare,
        Allocate,
        Release
    };

    PragmaInfo();
    virtual Type type() = 0;

    VariableDeclaration* arrayInfo();
    void setArrayInfo(VariableDeclaration* decl);

private:
    VariableDeclaration* m_decl;
};

class DeclarePragmaInfo : public PragmaInfo
{
public:
    DeclarePragmaInfo();
    virtual Type type() { return Allocate; }

    void setArrayDimensionCount(int m_dimCount);

    const ExpressionBase& dimensionSize(int index);
    void setDimensionSize(OPS::Reprise::ReprisePtr<ExpressionBase> expr, int index);

    const ExpressionBase& blockSize(int index);
    void setBlockSize(OPS::Reprise::ReprisePtr<ExpressionBase>, int index);

    bool isValid();

private:
    int m_dimCount;
    std::vector<OPS::Reprise::ReprisePtr<ExpressionBase> > m_dimSizeList;
    std::vector<OPS::Reprise::ReprisePtr<ExpressionBase> > m_blockSizeList;
};

class AllocatePragmaInfo : public PragmaInfo
{
public:
    AllocatePragmaInfo();
    virtual Type type() { return Allocate; }
};

class ReleasePragmaInfo : public PragmaInfo
{
public:
    ReleasePragmaInfo();
    virtual Type type() { return Release; }
};


std::list<std::tr1::shared_ptr<PragmaInfo> > findAllPragmas(OPS::Reprise::SubroutineDeclaration* func);

}
}
}

#endif

