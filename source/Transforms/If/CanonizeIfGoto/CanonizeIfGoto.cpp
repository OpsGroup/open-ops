#include <Transforms/If/CanonizeIfGoto/CanonizeIfGoto.h>
#include <Reprise/Reprise.h>
#include <Shared/ExpressionHelpers.h>

#include <iostream>

using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Reprise;

namespace OPS
{
namespace Transforms
{

bool canCanonizeIfGoto(IfStatement& ifStatement)
{
    BlockStatement& thenBlock = ifStatement.getThenBody();

    if (thenBlock.getChildCount() != 1 ||
            !thenBlock.getChild(0).is_a<GotoStatement>())
        return false;

    if (!ifStatement.getElseBody().isEmpty()) return false;

    BlockStatement& parent = ifStatement.getParentBlock();

    StatementBase* pointed = thenBlock.getChild(0).
            cast_to<GotoStatement>().getPointedStatement();

    if (&parent != &pointed->getParentBlock()) return false;

    BlockStatement::Iterator iter = parent.convertToIterator(&ifStatement);
    BlockStatement::Iterator iterEnd  = parent.convertToIterator(pointed);

    while (iter.isValid() && iter != iterEnd)
        iter++;

    return iter.isValid();
}

void canonizeIfGoto(IfStatement& ifStatement)
{
    OPS_ASSERT(canCanonizeIfGoto(ifStatement));

    BlockStatement& thenBlock = ifStatement.getThenBody();
    BlockStatement& parent = ifStatement.getParentBlock();

    BlockStatement::Iterator iter = parent.convertToIterator(&(ifStatement));
    //BlockStatement::Iterator iter2 = iter;
    BlockStatement::Iterator iterEnd  = parent.convertToIterator(thenBlock.getChild(0).
            cast_to<OPS::Reprise::GotoStatement>().getPointedStatement());

    thenBlock.erase(thenBlock.getFirst());
    iter++;

    while (!(iter == iterEnd))
    {
        ReprisePtr<OPS::Reprise::StatementBase> st(&(*iter));
        iter++;
        parent.erase(st.get());
        thenBlock.addLast(st.get());

    }

    BasicCallExpression* pCond = ifStatement.getCondition().cast_ptr<BasicCallExpression>();

    if (pCond != 0 &&
            pCond->getKind() == BasicCallExpression::BCK_LOGICAL_NOT)
    {
        ifStatement.setCondition(pCond->getArgument(0).clone());
    }
    else
    {
        ifStatement.setCondition(&(!op(ifStatement.getCondition())));
    }
}

}
}
