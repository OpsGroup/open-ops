#include "Reprise/Reprise.h"
#include "Transforms/Loops/Canonizing/CanonizeFor.h"
#include "Transforms/Loops/Canonizing/ConvertLessEqualToLess.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/Checks.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Analysis/Montego/OccurrenceContainer.h"

#include <iostream>

using namespace OPS::Reprise;
using namespace OPS::Transforms;
using namespace OPS::Shared;
using namespace OPS::Montego;

namespace OPS{namespace Montego{
//определена в Analysis/Montego/AliasAnalysis/MemoryCell.h
bool getInteger(RepriseBase* expr, long long int& res);
}}

namespace OPS
{
namespace Transforms
{
using namespace OPS::Shared::ExpressionHelpers;

bool forIsCanonized(OPS::Reprise::ForStatement& fstmt, AliasImplementation& ai)
{
    if (!Editing::forHeaderIsCanonized(fstmt)) return false;
    if (ai.isVariableChanged(Editing::getBasicForCounter(fstmt).getReference(), &fstmt.getBody())) return false;
    return true;
}

bool canonizeFor(OPS::Reprise::ForStatement& fstmt)
{
    if (!Editing::forIsBasic(fstmt))
    {
        if (canConvertLessEqualToLess(fstmt)) convertLessEqualToLess(fstmt);
        else return false;
    }
    //for speedup
    //if (Editing::forHeaderIsCanonized(fstmt)) return true;

    ReprisePtr<ExpressionBase> h = ReprisePtr<ExpressionBase>(Editing::getBasicForStep(fstmt).clone());
    ReprisePtr<ExpressionBase> i0 = ReprisePtr<ExpressionBase>(Editing::getBasicForInitExpression(fstmt).clone());
    ReprisePtr<ExpressionBase> i = ReprisePtr<ExpressionBase>(Editing::getBasicForCounter(fstmt).clone());
    ReprisePtr<ExpressionBase> n = ReprisePtr<ExpressionBase>(Editing::getBasicForFinalExpression(fstmt).clone());

    OccurrenceContainer ocont(*fstmt.findProgramUnit());
    //проводим анализ псевдонимов
    AliasImplementation ai(*fstmt.findProgramUnit(), ocont);
    int err = ai.runAliasAnalysis();
    if (err != 0) return false;
    if (forIsCanonized(fstmt,ai)) return true;

    if (ai.isVariableChanged(Editing::getBasicForCounter(fstmt).getReference(), &fstmt.getBody())) return false;

    if (!ai.isFortranAliases(Editing::getBasicForCounter(fstmt).getReference())) return false;
	long long int hInt;
    bool hIsInt = getInteger(&*h, hInt);
    ReprisePtr<ExpressionBase> subst;
    if (hIsInt && hInt == 1)
        subst = ReprisePtr<ExpressionBase>(&( op(i0) + op(i) ));
    else
        subst = ReprisePtr<ExpressionBase>(&( op(i0) + (op(i)*op(h)) ));
    Scalar::makeSubstitutionForward(fstmt.getBody(), *i, subst, true);
    IntegerHelper c(i->getResultType()->cast_to<BasicType>());
    Editing::replaceExpression(Editing::getBasicForInitExpression(fstmt), ReprisePtr<ExpressionBase>(&c(0)));
    Editing::replaceExpression(Editing::getBasicForStep(fstmt), ReprisePtr<ExpressionBase>(&c(1)));
    ReprisePtr<ExpressionBase> newFin;
    if (hIsInt && hInt == 1)
        newFin = ReprisePtr<ExpressionBase>(&( op(n)-op(i0) ));
    else
        newFin = ReprisePtr<ExpressionBase>(&( (op(n)-op(i0)+op(h)-c(1))/op(h) ));
    Editing::replaceExpression(Editing::getBasicForFinalExpression(fstmt), newFin);

    //TODO: сделать DataFlowAnalysis и вставить в канонизацию цикла добавление присваивания счетчику
    //правильного финального значения, если нужно


    OPS_ASSERT(Editing::forHeaderIsCanonized(fstmt));
    return true;
}


}

}
