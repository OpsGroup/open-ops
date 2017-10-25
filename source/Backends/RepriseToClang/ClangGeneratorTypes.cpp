/// ClangGeneratorTypes.cpp
///   Create clang ASTContext from target options.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 9.03.2013

#include "Backends/RepriseToClang/ClangGeneratorTypes.h"

#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Lex/Preprocessor.h"
#include "OPS_Core/disable_llvm_warnings_end.h"

using namespace OPS::Backends::Clang::Internal;

//////////////////////////////////////////////////////////////////////
// ASTContextParams

ASTContextParams::ASTContextParams()
{
	//
}

ASTContextParams::ASTContextParams(
	clang::TargetOptions& rTargetOptions)
{
	// 1. LangOptions

	m_PLangOptions = new clang::LangOptions;

	// 2. SourceManager

	llvm::IntrusiveRefCntPtr <clang::DiagnosticIDs> PDiagID(
		new clang::DiagnosticIDs());
	llvm::IntrusiveRefCntPtr <clang::DiagnosticOptions> PDiagOpts =
		new clang::DiagnosticOptions();
	clang::TextDiagnosticBuffer *pDiagsBuffer =
		new clang::TextDiagnosticBuffer;
	m_PDiagEngine =
		new clang::DiagnosticsEngine(PDiagID, &*PDiagOpts, pDiagsBuffer);
	m_PFileManager = new clang::FileManager(clang::FileSystemOptions());
	m_PSourceManager =
		new clang::SourceManager(*m_PDiagEngine, *m_PFileManager);

	// 3. TargetInfo

	m_PTargetInfo =
		clang::TargetInfo::CreateTargetInfo(*m_PDiagEngine, &rTargetOptions);

	// 4. IdentifierTable

	m_PIdTable.reset(new clang::IdentifierTable(*m_PLangOptions));

	// 5. SelectorTable

	m_PSelectorTable.reset(new clang::SelectorTable());

	// 6. Builtin::Contex

	m_PBuiltinContext.reset(new clang::Builtin::Context());
}

ASTContextParams::~ASTContextParams()
{
	//
}

//////////////////////////////////////////////////////////////////////
// ASTContextParams attributes

void ASTContextParams::initialize(
	clang::CompilerInstance& rCompilerInstance)
{
	// 1. LangOptions

	m_PLangOptions = &rCompilerInstance.getLangOpts();

	// 2. SourceManager

	m_PSourceManager = &rCompilerInstance.getSourceManager();

	// 3. TargetInfo

	m_PTargetInfo = &rCompilerInstance.getTarget();

	// 4. IdentifierTable

	//m_PIdTable.reset(
	//	new clang::IdentifierTable(
	//		rCompilerInstance.getPreprocessor().getIdentifierTable()));

	m_PIdTable.reset(new clang::IdentifierTable(*m_PLangOptions));

	// 5. SelectorTable

	m_PSelectorTable.reset(new clang::SelectorTable());

	// 6. Builtin::Contex

	m_PBuiltinContext.reset(
		new clang::Builtin::Context(
			rCompilerInstance.getPreprocessor().getBuiltinInfo()));
}

//////////////////////////////////////////////////////////////////////
// ASTContextParams operations

clang::ASTContext* ASTContextParams::createASTContext()
{
	clang::ASTContext* pASTContext = new clang::ASTContext(
		*m_PLangOptions,
		*m_PSourceManager,
		&*m_PTargetInfo,
		*m_PIdTable,
		*m_PSelectorTable,
		*m_PBuiltinContext,
		100000);

	return pASTContext;
}

// End of File
