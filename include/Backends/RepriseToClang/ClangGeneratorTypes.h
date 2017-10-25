/// ClangGeneratorTypes.h
/// Created: 27.02.2013

#ifndef CLANGGENERATORTYPES_H__
#define CLANGGENERATORTYPES_H__

#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "clang/AST/ASTContext.h"

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/OwningPtr.h"
#include "OPS_Core/disable_llvm_warnings_end.h"

#include <vector>

namespace clang
{
	class CompilerInstance;
	class LangOptions;
	class DiagnosticsEngine;
	class FileManager;
	class IdentifierTable;
	class SelectorTable;
	class SourceManager;
	class TargetInfo;
	class TargetOptions;

	namespace Builtin
	{
		class Context;
	}
}

namespace OPS
{
	namespace Backends
	{
		namespace Clang
		{
			typedef llvm::IntrusiveRefCntPtr <clang::ASTContext>
				ASTContextPtr;

			typedef std::vector <ASTContextPtr>
				ASTContexts;

			namespace Internal
			{
				typedef llvm::IntrusiveRefCntPtr <clang::LangOptions>
					LangOptionsPtr;

				typedef llvm::IntrusiveRefCntPtr <clang::DiagnosticsEngine>
					DiagnosticsEnginePtr;

				typedef llvm::IntrusiveRefCntPtr <clang::FileManager>
					FileManagerPtr;

				typedef llvm::IntrusiveRefCntPtr <clang::SourceManager>
					SourceManagerPtr;

				typedef llvm::IntrusiveRefCntPtr <clang::TargetInfo>
					TargetInfoPtr;

				typedef llvm::OwningPtr <clang::IdentifierTable>
					IdentifierTablePtr;

				typedef llvm::OwningPtr <clang::SelectorTable>
					SelectorTablePtr;

				typedef llvm::OwningPtr <clang::Builtin::Context>
					BuiltinContextPtr;

				class ASTContextParams
				{
				public:
					//
					ASTContextParams();
					explicit ASTContextParams(
						clang::TargetOptions& rTargetOptions);
					~ASTContextParams();
					//
				// Attributes:
				public:
					//
					void initialize(
						clang::CompilerInstance& rCompilerInstance);
					//
				// Operations:
				public:
					//
					clang::ASTContext* createASTContext();
					//
				private:
					//
					LangOptionsPtr m_PLangOptions;
					DiagnosticsEnginePtr m_PDiagEngine;
					FileManagerPtr m_PFileManager;
					SourceManagerPtr m_PSourceManager;
					TargetInfoPtr m_PTargetInfo;
					IdentifierTablePtr m_PIdTable;
					SelectorTablePtr m_PSelectorTable;
					BuiltinContextPtr m_PBuiltinContext;
					//
				};    // class ASTContextParams
			}    // namespace Internal
		}    // namespace Clang
	}    // namespace Backends
}    // namespace OPS

#endif    // CLANGGENERATORTYPES_H__

// End of File
