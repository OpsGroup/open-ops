#include <string>
#include <algorithm>
#include <iostream>


#include "Shared/ExpressionHelpers.h"

#include "OPS_Core/StlHelpers.h"
#include "OPS_Core/OPS_Core.h"

#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Declarations.h"
#include "Reprise/Types.h"

#include "include/ArrayDistributionInfo.h"
#include "include/shared_helpers.h"
#include "Shared/DataShared.h"


namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace std;
using namespace OPS::Reprise;
using namespace OPS::Transforms;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Reprise::Editing;


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


#define PragmasFinderVisitStmt(StmtType) \
    void visit(StmtType& node) \
    {		\
    if (node.hasNote("block_array_allocate")) \
    m_allocNodeList.push_back(&node); \
    if (node.hasNote("block_array_release")) \
    m_releaseNodeList.push_back(&node); \
    Service::DeepWalker::visit(node); \
}


    PragmasFinderVisitStmt(BlockStatement)
    PragmasFinderVisitStmt(ForStatement)
    PragmasFinderVisitStmt(WhileStatement)
    PragmasFinderVisitStmt(IfStatement)
    PragmasFinderVisitStmt(GotoStatement)
    PragmasFinderVisitStmt(ReturnStatement)
    PragmasFinderVisitStmt(Canto::HirBreakStatement)
    PragmasFinderVisitStmt(Canto::HirContinueStatement)
    PragmasFinderVisitStmt(ExpressionStatement)
    PragmasFinderVisitStmt(EmptyStatement)
};

void ArrayDistributionInfo::setArrayInfo(OPS::Reprise::VariableDeclaration* decl)
{
    m_decl = decl;
}
OPS::Reprise::VariableDeclaration* ArrayDistributionInfo::arrayInfo()
{
    return m_decl;
}

BasicType& ArrayDistributionInfo::arrayType()
{
    PtrType& t = m_decl->getType().cast_to<PtrType>();
    return t.getPointedType().cast_to<BasicType>();
}

int ArrayDistributionInfo::dimensionCount()
{
    return m_dimCount;
}
void ArrayDistributionInfo::setDimensionCount(int dimCount)
{
    m_dimCount = dimCount;
    m_blockSizeList.resize(dimCount);
    m_dimSizeList.resize(dimCount);

    m_dimSizeListRefs.resize(dimCount);
    m_blockSizeListRefs.resize(dimCount);

    m_distrArrayBlockDimSizeRefs.resize(dimensionCount());
    m_distrArrayBlockDimSizeVars.resize(dimensionCount());
}

ReferenceExpression& ArrayDistributionInfo::dimensionSize(int index)
{
    OPS_ASSERT(index >= 0 && index < m_dimSizeList.size());
    return *m_dimSizeListRefs[index];
}
VariableDeclaration& ArrayDistributionInfo::dimensionSizeReference(int index)
{
    OPS_ASSERT(index >= 0 && index < m_dimSizeList.size());
    return *m_dimSizeList[index];
}
void ArrayDistributionInfo::setDimensionSizeReference(VariableDeclaration* var, int index)
{
    OPS_ASSERT(index >= 0 && index < m_dimSizeList.size());
    m_dimSizeList[index] = var;
    m_dimSizeListRefs[index] =  ReprisePtr<ReferenceExpression>(new ReferenceExpression(*m_dimSizeList[index]));
}

ReferenceExpression& ArrayDistributionInfo::blockSize(int index)
{
    OPS_ASSERT(index >= 0 && index < m_dimSizeList.size());
    return *m_blockSizeListRefs[index];
}
VariableDeclaration& ArrayDistributionInfo::blockSizeReference(int index)
{
    OPS_ASSERT(index >= 0 && index < m_dimSizeList.size());
    return *m_blockSizeList[index];
}
void ArrayDistributionInfo::setBlockSizeReference(VariableDeclaration* var, int index)
{
    OPS_ASSERT(index >= 0 && index < m_blockSizeList.size());
    m_blockSizeList[index] = var;
    m_blockSizeListRefs[index] =  ReprisePtr<ReferenceExpression>(new ReferenceExpression(*m_blockSizeList[index]));
}

void ArrayDistributionInfo::initializeArrayDimensionSizeVariables()
{
    for (int i = 0; i < dimensionCount(); i++)
    {
        const std::string name = OPS::Strings::format("_%s_NN_%d", m_decl->getName().c_str(), i);
        if (m_decl->hasDefinedBlock() == true)
            m_distrArrayBlockDimSizeVars[i] = &(OPS::Reprise::Editing::createNewVariable(*BasicType::int32Type(), m_decl->getDefinedBlock(), name));
        else
        {
            OPS::Reprise::TranslationUnit* unit = OPS::Shared::getTranslationUnit(m_decl);
            m_distrArrayBlockDimSizeVars[i] = &(OPS::Reprise::Editing::createNewGlobalVariable(*BasicType::int32Type(), *unit, name));
        }
        m_distrArrayBlockDimSizeRefs[i] = ReprisePtr<ReferenceExpression>(new ReferenceExpression(*m_distrArrayBlockDimSizeVars[i]));

        ReprisePtr<ExpressionBase> expr = ReprisePtr<ExpressionBase>(&(op(*m_distrArrayBlockDimSizeRefs[i]) R_AS ((op(dimensionSize(i)) + op(blockSize(i))) / op(blockSize(i)) * op(blockSize(i))) ));
        for (auto it = m_allocStmtList.begin(); it != m_allocStmtList.end(); ++it)
        {
            StatementBase* next_alloc_stmt = *it;
            BlockStatement& block = next_alloc_stmt->getParentBlock();

            BlockStatement::Iterator bi = block.convertToIterator(next_alloc_stmt);
            block.addBefore(bi, new ExpressionStatement(expr->clone()));
        }
    }
}

ReferenceExpression& ArrayDistributionInfo::distributedArrayDimensionSize(int index)
{
    if (m_distrArrayBlockDimSizeVars[index] == 0)
        initializeArrayDimensionSizeVariables();

    return *m_distrArrayBlockDimSizeRefs[index];
}
VariableDeclaration& ArrayDistributionInfo::distributedArrayDimensionSizeReference(int index)
{
    if (m_distrArrayBlockDimSizeVars[index] == 0)
        initializeArrayDimensionSizeVariables();

    return *m_distrArrayBlockDimSizeVars[index];
}
ExpressionBase* ArrayDistributionInfo::calculateDistributedArraySize()
{
    ExpressionBase* result = 0;
    for (int i=0; i<dimensionCount(); i++)
    {
        if (result == 0)
            result = &distributedArrayDimensionSize(i);
        else
            result =  &(op(result)*op(&distributedArrayDimensionSize(i)));
    }

    return result;
}

void ArrayDistributionInfo::addAllocStmt(StatementBase* stmt)
{
    m_allocStmtList.push_back(stmt);
}
int ArrayDistributionInfo::allocStmtCount()
{
    return m_allocStmtList.size();
}
void ArrayDistributionInfo::replaceAllocStmt(int index, StatementBase* newStmt)
{
    OPS::Reprise::Editing::replaceStatement(*m_allocStmtList[index], ReprisePtr<StatementBase>(newStmt));
    m_allocStmtList[index] = newStmt;
}

void ArrayDistributionInfo::addReleaseStmt(StatementBase* stmt)
{
    m_releaseStmtList.push_back(stmt);
}
int ArrayDistributionInfo::releaseStmtCount()
{
    return m_releaseStmtList.size();
}
void ArrayDistributionInfo::replaceReleaseStmt(int index, StatementBase* newStmt)
{
    OPS::Reprise::Editing::replaceStatement(*m_releaseStmtList[index], ReprisePtr<StatementBase>(newStmt));
    m_releaseStmtList[index] = newStmt;
}

std::vector<OPS::Reprise::StatementBase*> ArrayDistributionInfo::allocStmtList()
{
    return m_allocStmtList;
}
std::vector<OPS::Reprise::StatementBase*> ArrayDistributionInfo::releaseStmtList()
{
    return m_releaseStmtList;
}

void ArrayDistributionInfo::initialize_coefs()
{
    d_coefs_refs.resize(dimensionCount());
    i_coefs_refs.resize(dimensionCount());
    for (int i = 0; i < dimensionCount(); i++)
    {
        {
            const std::string name = OPS::Strings::format("_%s_d_coef_%d", m_decl->getName().c_str(), i);
            VariableDeclaration* dc = 0;
            if (m_decl->hasDefinedBlock() == true)
                dc = &(OPS::Reprise::Editing::createNewVariable(*BasicType::int32Type(), m_decl->getDefinedBlock(), name));
            else
            {
                OPS::Reprise::TranslationUnit* unit = OPS::Shared::getTranslationUnit(m_decl);
                dc = &(OPS::Reprise::Editing::createNewGlobalVariable(*BasicType::int32Type(), *unit, name));
            }

            d_coefs_refs[i] = ReprisePtr<ReferenceExpression>(new ReferenceExpression(*dc));
            ExpressionBase* dc_value = 0;
            for (int j = 0; j < dimensionCount(); j++)
            {
                ExpressionBase* mul = &(j <= i ? op(blockSize(j)) : op(distributedArrayDimensionSize(j)));
                dc_value = dc_value == 0 ? mul : &((*dc_value) * (*mul));
            }

            ReprisePtr<ExpressionBase> expr = ReprisePtr<ExpressionBase>(&(op(*d_coefs_refs[i]) R_AS (*dc_value)));
            for (auto it = m_allocStmtList.begin(); it != m_allocStmtList.end(); ++it)
            {
                StatementBase* next_alloc_stmt = *it;
                BlockStatement& block = next_alloc_stmt->getParentBlock();

                BlockStatement::Iterator bi = block.convertToIterator(next_alloc_stmt);
                block.addBefore(bi, new ExpressionStatement(expr->clone()));
            }
        }


        {
            const std::string name = OPS::Strings::format("_%s_i_coef_%d", m_decl->getName().c_str(), i);
            VariableDeclaration* ic = 0;
            if (m_decl->hasDefinedBlock() == true)
                ic = &(OPS::Reprise::Editing::createNewVariable(*BasicType::int32Type(), m_decl->getDefinedBlock(), name));
            else
            {
                OPS::Reprise::TranslationUnit* unit = OPS::Shared::getTranslationUnit(m_decl);
                ic = &(OPS::Reprise::Editing::createNewGlobalVariable(*BasicType::int32Type(), *unit, name));
            }

            i_coefs_refs[i] = ReprisePtr<ReferenceExpression>(new ReferenceExpression(*ic));
            ExpressionBase* ic_value = 0;
            for (int j = i+1; j < dimensionCount(); j++)
            {
                ExpressionBase* mul = &op(blockSize(j));
                ic_value = ic_value == 0 ? mul : &((*ic_value) * (*mul));
            }
            if (ic_value == 0) ic_value = StrictLiteralExpression::createInt32(1);

            ReprisePtr<ExpressionBase> expr = ReprisePtr<ExpressionBase>(&(op(*i_coefs_refs[i]) R_AS (*ic_value)));
            for (auto it = m_allocStmtList.begin(); it != m_allocStmtList.end(); ++it)
            {
                StatementBase* next_alloc_stmt = *it;
                BlockStatement& block = next_alloc_stmt->getParentBlock();

                BlockStatement::Iterator bi = block.convertToIterator(next_alloc_stmt);
                block.addBefore(bi, new ExpressionStatement(expr->clone()));
            }
        }
    }

}

ExpressionBase& ArrayDistributionInfo::linear_d_coef(int index)
{
    if (d_coefs_refs.size() == 0)
        initialize_coefs();

    return *d_coefs_refs[index];
}
ExpressionBase& ArrayDistributionInfo::linear_i_coef(int index)
{
    if (i_coefs_refs.size() == 0)
        initialize_coefs();

    return *i_coefs_refs[index];
}

ArrayDistributionInfoList collectDataDistirubitionInfo(OPS::Reprise::SubroutineDeclaration* decl)
{
    ArrayDistributionInfoList result;
    if (decl->hasImplementation() == false)
        return result;

    PragmasFinder visitor;
    decl->accept(visitor);
    for (std::list<VariableDeclaration*>::iterator it = visitor.m_declNodeList.begin(); it != visitor.m_declNodeList.end(); ++it)
    {
        std::shared_ptr<ArrayDistributionInfo> info(new ArrayDistributionInfo);
        string value = (*it)->getNote("block_array_declare").getString();
        list<string> args = OPS::Strings::split(value, ",");

        bool stopped;
        list<string>::iterator argIt;

        //  get array name
        argIt = args.begin();
        if (argIt == args.end())
            continue;
        VariableDeclaration* var = findVariableByName(decl->getBodyBlock(), *argIt);
        if (var == 0)
            continue;
        info->setArrayInfo(var);

        //  get dim count
        ++argIt;
        if (argIt == args.end())
            continue;
        int dimCount;
        if (OPS::Strings::fetch(*argIt, dimCount) == false || dimCount < 0 || dimCount > 10)
            continue;
        info->setDimensionCount(dimCount);

        //  get dim size list
        stopped = false;
        for (int i = 0; i < 2*dimCount; i++)
        {
            ++argIt;
            if (argIt == args.end())
            {
                stopped = true;
                break;
            }


            VariableDeclaration* paramDecl = 0;
            int value;
            if (OPS::Strings::fetch(*argIt, value) == true && value > 0)
            {
                const std::string name = OPS::Strings::format("_%s_arg_%d", var->getName().c_str(), i);
                ReprisePtr<BasicType> type = ReprisePtr<BasicType>(BasicType::int32Type());
                type->setConst(true);
                paramDecl = &(OPS::Reprise::Editing::createNewVariable(*type->clone(), decl->getBodyBlock(), name));
                paramDecl->setInitExpression(*StrictLiteralExpression::createInt32(value));
            }
            else
                paramDecl = findVariableByName(decl->getBodyBlock(), *argIt);

            if (paramDecl == 0)
            {
                stopped = true;
                break;
            }

            if (i < dimCount)
                info->setDimensionSizeReference(paramDecl, i);
            else
                info->setBlockSizeReference(paramDecl, i-dimCount);
        }
        if (stopped == true)
            continue;

        result.push_back(info);
    }

    for (std::list<StatementBase*>::iterator it = visitor.m_allocNodeList.begin(); it != visitor.m_allocNodeList.end(); ++it)
    {
        string value = (*it)->getNote("block_array_allocate").getString();
        list<string> args = OPS::Strings::split(value, ",");

        list<string>::iterator argIt;

        //  get array name
        argIt = args.begin();
        if (argIt == args.end())
            continue;
        VariableDeclaration* var = findVariableByName(decl->getBodyBlock(), *argIt);
        if (var == 0)
            continue;

        for (auto infoIt = result.begin(); infoIt != result.end(); ++infoIt)
        {
            auto info = *infoIt;
            if (info->arrayInfo() == var)
            {
                info->addAllocStmt(*it);
                break;
            }
        }
    }

    for (std::list<StatementBase*>::iterator it = visitor.m_releaseNodeList.begin(); it != visitor.m_releaseNodeList.end(); ++it)
    {
        string value = (*it)->getNote("block_array_release").getString();
        list<string> args = OPS::Strings::split(value, ",");

        list<string>::iterator argIt;

        //  get array name
        argIt = args.begin();
        if (argIt == args.end())
            continue;
        VariableDeclaration* var = findVariableByName(decl->getBodyBlock(), *argIt);
        if (var == 0)
            continue;

        for (auto infoIt = result.begin(); infoIt != result.end(); ++infoIt)
        {
            auto info = *infoIt;
            if (info->arrayInfo() == var)
            {
                info->addReleaseStmt(*it);
                break;
            }
        }
    }

    return result;
}


class ArrayReferenceFinder : public Service::DeepWalker
{
public:
    list<ReferenceExpression*> referenceList;
    shared_ptr<ArrayDistributionInfo> info;
    void visit(ReferenceExpression& node)
    {
        if (&node.getReference() == info->arrayInfo())
            referenceList.push_back(&node);
        Service::DeepWalker::visit(node);
    }

#define ArrayReferenceFinderVisitStmt(StmtType) \
    void visit(StmtType& node)\
    { \
    std::vector<OPS::Reprise::StatementBase*> list1 = info->allocStmtList();\
    std::vector<OPS::Reprise::StatementBase*> list2 = info->releaseStmtList();\
    if (std::find(list1.begin(), list1.end(), &node) == list1.end()\
    && std::find(list2.begin(), list2.end(), &node) == list2.end())\
    Service::DeepWalker::visit(node); \
}


    ArrayReferenceFinderVisitStmt(BlockStatement)
    ArrayReferenceFinderVisitStmt(ForStatement)
    ArrayReferenceFinderVisitStmt(WhileStatement)
    ArrayReferenceFinderVisitStmt(IfStatement)
    ArrayReferenceFinderVisitStmt(GotoStatement)
    ArrayReferenceFinderVisitStmt(ReturnStatement)
    ArrayReferenceFinderVisitStmt(Canto::HirBreakStatement)
    ArrayReferenceFinderVisitStmt(Canto::HirContinueStatement)
    ArrayReferenceFinderVisitStmt(ExpressionStatement)
    ArrayReferenceFinderVisitStmt(EmptyStatement)
};

void checkDataDistributionInfoCorrectness(ArrayDistributionInfoList& infoList, OPS::Reprise::SubroutineDeclaration* decl)
{
    for (auto infoIt = infoList.begin(); infoIt != infoList.end();)
    {
        auto info = *infoIt;
        bool correct = true;


        ArrayReferenceFinder visitor;
        visitor.info = *infoIt;
        decl->accept(visitor);

        for (auto it = visitor.referenceList.begin(); it != visitor.referenceList.end(); ++it)
        {
            ReferenceExpression* expr = *it;
            if (expr->getParent()->is_a<BasicCallExpression>() == false)
            {
                std::cout << expr->getParent()->dumpState() << endl;
                correct = false;
                break;
            }
            BasicCallExpression* bck = expr->getParent()->cast_ptr<BasicCallExpression>();
            BasicCallExpression::BuiltinCallKind kind = bck->getKind();
            if (bck->getKind() != BasicCallExpression::BCK_ARRAY_ACCESS)
            {
                correct = false;
                break;
            }
            int opCount = bck->getArgumentCount() - 1;
            if (opCount != 1)
            {
                correct = false;
                break;
            }
        }

        if (correct == false)
            infoIt = infoList.erase(infoIt);
        else
            ++infoIt;
    }
}

list<ReferenceExpression*> findDistributedArrayReferences(ArrayDistributionInfoList& infoList, OPS::Reprise::SubroutineDeclaration* decl)
{
    list<ReferenceExpression*> result;
    for (auto infoIt = infoList.begin(); infoIt != infoList.end();)
    {
        ArrayReferenceFinder visitor;
        visitor.info = *infoIt;
        decl->accept(visitor);
        result.insert(result.end(), visitor.referenceList.begin(), visitor.referenceList.end());
    }

    return result;
}


}
}
}

