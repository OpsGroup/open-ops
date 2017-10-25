#include <cassert>

#include "Reprise/Types.h"
#include "Reprise/Expressions.h"
#include "OPS_Core/OPS_Core.h"
#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include "Shared/ExpressionHelpers.h"

#include "include/shared_helpers.h"

using namespace OPS::Reprise;
using namespace std;
using namespace OPS::Shared::ExpressionHelpers;

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{


int getValueOfConstIntegerVariable(OPS::Reprise::VariableDeclaration* decl)
{
	assert(decl->getType().isConst());
	
	ExpressionBase* initExpr = &decl->getInitExpression();
	assert(initExpr->is_a<StrictLiteralExpression>());

	int result = initExpr->cast_to<StrictLiteralExpression>().getInt32();
	return result;
}

bool isBlockArrayReference(ReferenceExpression& ref, list<shared_ptr<ArrayDistributionInfo> >& infoList,
                           shared_ptr<ArrayDistributionInfo>& info)
{
    VariableDeclaration* decl = &ref.getReference();
    for (auto it = infoList.begin(); it != infoList.end(); ++it)
    {
        if ((*it)->arrayInfo() == decl)
        {
            info = *it;
            return true;
        }
    }
    return false;
}

OPS::Reprise::VariableDeclaration* getLoopCounter(OPS::Reprise::ForStatement* stmt)
{
    BasicCallExpression* init = stmt->getInitExpression().cast_ptr<BasicCallExpression>();
    if (init != 0)
    {
        if (init->getKind() == BasicCallExpression::BCK_ASSIGN)
        {
            ReferenceExpression* loopRef = init->getArgument(0).cast_ptr<ReferenceExpression>();
            if (loopRef != 0)
                return &loopRef->getReference();
        }
    }
    return 0;
}

bool check_loop(const ForStatement& forStmt)
{
    // for (i = ...; ...; ...)
    const ExpressionBase& initExpr = forStmt.getInitExpression();
    if (!initExpr.is_a<BasicCallExpression>())
    {
        return false;
    }
    const BasicCallExpression& initAssignExpr = initExpr.cast_to<BasicCallExpression>();
    if (initAssignExpr.getKind() != BasicCallExpression::BCK_ASSIGN || !initAssignExpr.getArgument(0).is_a<ReferenceExpression>())
    {
        return false;
    }
    const ReferenceExpression& initCounter = initAssignExpr.getArgument(0).cast_to<ReferenceExpression>();

    // for (...; i < ...; ...)
    const ExpressionBase& finalExpr = forStmt.getFinalExpression();
    if (!finalExpr.is_a<BasicCallExpression>())
    {
        return false;
    }
    const BasicCallExpression& finalLessExpr = finalExpr.cast_to<BasicCallExpression>();
    if (finalLessExpr.getKind() != BasicCallExpression::BCK_LESS)
    {
        return false;
    }
    const ReferenceExpression& finalCounter = finalLessExpr.getArgument(0).cast_to<ReferenceExpression>();

    // for (...; ...; i = i + ...)
    const ExpressionBase& stepExpr = forStmt.getStepExpression();
    if (!stepExpr.is_a<BasicCallExpression>())
    {
        return false;
    }
    const BasicCallExpression& stepAssignExpr = stepExpr.cast_to<BasicCallExpression>();
    if (stepAssignExpr.getKind() != BasicCallExpression::BCK_ASSIGN || !stepAssignExpr.getArgument(0).is_a<ReferenceExpression>() || !stepAssignExpr.getArgument(1).is_a<BasicCallExpression>())
    {
        return false;
    }
    const ReferenceExpression& stepLeftCounter = stepAssignExpr.getArgument(0).cast_to<ReferenceExpression>();
    const BasicCallExpression& stepPlusExpr = stepAssignExpr.getArgument(1).cast_to<BasicCallExpression>();
    if (stepPlusExpr.getKind() != BasicCallExpression::BCK_BINARY_PLUS || !stepPlusExpr.getArgument(0).is_a<ReferenceExpression>())
    {
        return false;
    }
    const ReferenceExpression& stepRightCounter = stepPlusExpr.getArgument(0).cast_to<ReferenceExpression>();

    // compare all counters
    if (!initCounter.isEqual(finalCounter) || !stepLeftCounter.isEqual(stepRightCounter) || !initCounter.isEqual(stepLeftCounter))
    {
        return false;
    }

    return true;
}

class LoopFinder : public OPS::Reprise::Service::DeepWalker
{
public:
    list<ForStatement*> loops;
    void visit(ForStatement& node)
    {
        VariableDeclaration* counter = getLoopCounter(&node);
        if (counter != 0)
            loops.push_back(&node);
        Service::DeepWalker::visit(node);
    }
};

std::vector<ForStatement*> findAllLoops(OPS::Reprise::SubroutineDeclaration* func)
{
    LoopFinder visitor;
    func->accept(visitor);
    return std::vector<ForStatement*>(visitor.loops.begin(), visitor.loops.end());
}



VariableDeclaration* findVariableByName(BlockStatement& innerBlock, string name)
{
    VariableDeclaration* variableDecl = innerBlock.getDeclarations().findVariable(name);
    if (variableDecl != 0)
        return variableDecl;
    if (innerBlock.hasParentBlock() == true)
        return findVariableByName(innerBlock.getParentBlock(), name);

    return innerBlock.findTranslationUnit()->getGlobals().findVariable(name);
}

bool expression_values_are_equal(ExpressionBase& e1, ExpressionBase& e2)
{
    if (e1.is_a<ReferenceExpression>() && e2.is_a<ReferenceExpression>())
    {
        ReferenceExpression& r1 = e1.cast_to<ReferenceExpression>();
        ReferenceExpression& r2 = e2.cast_to<ReferenceExpression>();

        if (r1.getReference().getNCID() == r2.getReference().getNCID())
            return true;
    }


    bool v1_found = false;
    int v1 = -1;
    if (e1.is_a<StrictLiteralExpression>())
    {
        StrictLiteralExpression& l1 = e1.cast_to<StrictLiteralExpression>();
        v1 = l1.getInt32();
        v1_found = true;
    }
    if (e1.is_a<ReferenceExpression>())
    {
        ReferenceExpression& r1 = e1.cast_to<ReferenceExpression>();
        VariableDeclaration& decl1 = r1.getReference();
        if (decl1.getType().isConst()
                && decl1.hasNonEmptyInitExpression() == true
                && decl1.getInitExpression().is_a<StrictLiteralExpression>() == true)
        {
            v1 = decl1.getInitExpression().cast_to<StrictLiteralExpression>().getInt32();
            v1_found = true;
        }
    }

    bool v2_found = false;
    int v2 = -1;
    if (e2.is_a<StrictLiteralExpression>())
    {
        StrictLiteralExpression& l2 = e1.cast_to<StrictLiteralExpression>();
        v2 = l2.getInt32();
        v2_found =  true;
    }
    if (e2.is_a<ReferenceExpression>())
    {
        ReferenceExpression& r2 = e2.cast_to<ReferenceExpression>();
        VariableDeclaration& decl2 = r2.getReference();
        if (decl2.getType().isConst()
                && decl2.hasNonEmptyInitExpression() == true
                && decl2.getInitExpression().is_a<StrictLiteralExpression>() == true)
        {
            v2 = decl2.getInitExpression().cast_to<StrictLiteralExpression>().getInt32();
            v2_found =  true;
        }
    }

    return v2_found && v1_found && v1 == v2;
}


class DevideQuotientHelper : public Service::DeepWalker
{
    ReferenceExpression* d;
    bool m_buildQuotientExpr;

    void populate(ExpressionBase* factor)
    {
        if (is_devided == false && expression_values_are_equal(*factor, *d))
        {
            is_devided = true;
            return;
        }

        if (m_buildQuotientExpr == true)
        {
            if (m_otherFactors == 0)
                m_otherFactors = factor->clone();
            else
                m_otherFactors = &((*m_otherFactors) * op(*factor));
        }
    }
    ExpressionBase* m_otherFactors;
public:
    bool is_devided;
    ExpressionBase* otherFactors()
    {
        if (m_otherFactors == 0)
            m_otherFactors = StrictLiteralExpression::createInt32(1);
        return m_otherFactors;
    }

    DevideQuotientHelper(ReferenceExpression* d, bool buildQuotientExpr)
        : d(d), is_devided(false), m_buildQuotientExpr(buildQuotientExpr), m_otherFactors(0)
    {
    }

    void visit(BasicCallExpression& node)
    {
        if (node.getKind() == BasicCallExpression::BCK_MULTIPLY)
            Service::DeepWalker::visit(node);
        else
            populate(&node);
    }

#define DevideQuotientHelperVisitMethod(ExprType) \
    void visit(ExprType& node) { populate(&node); }

    DevideQuotientHelperVisitMethod(BasicLiteralExpression)
    DevideQuotientHelperVisitMethod(CompoundLiteralExpression)
    DevideQuotientHelperVisitMethod(SubroutineReferenceExpression)
    DevideQuotientHelperVisitMethod(StructAccessExpression)
    DevideQuotientHelperVisitMethod(EnumAccessExpression)
    DevideQuotientHelperVisitMethod(SubroutineCallExpression)
    DevideQuotientHelperVisitMethod(EmptyExpression)
    DevideQuotientHelperVisitMethod(ReferenceExpression)
    DevideQuotientHelperVisitMethod(StrictLiteralExpression)
};

bool expression_value_is(ExpressionBase& e, int value)
{
    StrictLiteralExpression* s = e.cast_ptr<StrictLiteralExpression>();
    if (s != 0)
        return s->getInt32() == value;
    return false;
}

bool expression_is_devided_on(ExpressionBase& e1, ReferenceExpression& d)
{
    DevideQuotientHelper h(&d, false);
    e1.accept(h);
    return h.is_devided == true;
}

//  если e1 = a*d, то возвращается a
ExpressionBase* get_quotient_expression(ExpressionBase& e1, ReferenceExpression& d)
{
    OPS_ASSERT(expression_is_devided_on(e1, d));

    DevideQuotientHelper h(&d, true);
    e1.accept(h);
    return h.otherFactors();
}

}
}
}
