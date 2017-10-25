/// ClangStatements.h
///   Create clang statements from Reprise statements.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created:  9.08.2013

#ifndef CLANG_STATEMENTS_H__
#define CLANG_STATEMENTS_H__

#include "R2CDeclNodes.h"

namespace clang
{
	class ASTContext;
	class Stmt;
}

namespace OPS
{
	namespace Reprise
	{
		class StatementBase;
	}

	namespace Backends
	{
		namespace Clang
		{
			namespace Internal
			{
				clang::Stmt* getClangStmt(
					clang::ASTContext &rASTContext,
					OPS::Reprise::StatementBase& rStatementBase,
					R2CDeclNodes& rR2CDeclNodes);

			}    // namespace Internal
		}    // namespace Clang
	}    // namespace Backends
}    // namespace OPS

#endif    // CLANG_STATEMENTS_H__

// End of File
