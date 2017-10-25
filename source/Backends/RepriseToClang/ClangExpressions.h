/// ClangExpressions.h
///   Create clang expressions from Reprise expressions.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 12.04.2013

#ifndef CLANG_EXPRESSIONS_H__
#define CLANG_EXPRESSIONS_H__

#include "R2CDeclNodes.h"

namespace clang
{
	class ASTContext;
	class Expr;
}

namespace OPS
{
	namespace Reprise
	{
		class ExpressionBase;
	}

	namespace Backends
	{
		namespace Clang
		{
			namespace Internal
			{
				clang::Expr* getClangExpr(
					clang::ASTContext &rASTContext,
					OPS::Reprise::ExpressionBase& rExpressionBase,
					R2CDeclNodes& rR2CDeclNodes,
					bool needLtoRconversion = false);

			}    // namespace Internal
		}    // namespace Clang
	}    // namespace Backends
}    // namespace OPS

#endif    // CLANG_EXPRESSIONS_H__

// End of File
