#ifndef _SUBSTITUTION_FORWARD_H_INCLUDED_
#define _SUBSTITUTION_FORWARD_H_INCLUDED_

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

using OPS::Reprise::ReprisePtr;

using OPS::Reprise::StatementBase;
using OPS::Reprise::ExpressionBase;

using OPS::Reprise::BlockStatement;


void makeSubstitutionForward(StatementBase& statementTo, ExpressionBase& expressionInstead, ReprisePtr<ExpressionBase> expressionWhat,
	bool changeLeftPartOfAssign = false);

void makeSubstitutionForward(ExpressionBase& expressionTo, ExpressionBase& expressionInstead, ReprisePtr<ExpressionBase> expressionWhat,
	bool changeLeftPartOfAssign = false);

} // Scalar
} // Transforms
} // OPS

#endif	// _SUBSTITUTION_FORWARD_H_INCLUDED_
