/// ClangTypes.h
///   Create clang types from Reprise types.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 12.04.2013

#ifndef CLANG_TYPES_H__
#define CLANG_TYPES_H__

#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "clang/AST/Type.h"
#include "OPS_Core/disable_llvm_warnings_end.h"

#include "R2CDeclNodes.h"

#include <map>

namespace clang
{
	enum StorageClass;
	class ASTContext;
}

namespace OPS
{
	namespace Reprise
	{
		class TypeBase;
		class VariableDeclarators;
	}

	namespace Backends
	{
		namespace Clang
		{
			namespace Internal
			{
				clang::StorageClass getClangStorageClass(
					const OPS::Reprise::VariableDeclarators& rcVariableDeclarators);

				clang::QualType getClangQualType(
					clang::ASTContext &rASTContext,
					OPS::Reprise::TypeBase& rType,
					R2CDeclNodes& rR2CDeclNodes);

			}    // namespace Internal
		}    // namespace Clang
	}    // namespace Backends
}    // namespace OPS

#endif    // CLANG_TYPES_H__

// End of File
