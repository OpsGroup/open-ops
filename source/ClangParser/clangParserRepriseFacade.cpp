/// Подключения clang
#include "OPS_Core/disable_llvm_warnings_begin.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "OPS_Core/disable_llvm_warnings_end.h"

/// Глобальные подключения
#include "Reprise/Reprise.h"
#include "ClangParser/clangParser.h"
#include "OPS_Core/Kernel.h"

/// Локальные подключения
#include "AST2RepriseConsumers.h"
#include "RepriseDiagnosticClient.h"


namespace clang
{
	static bool parseFileHelper(
		OPS::Reprise::RepriseContext* context, const std::string& fileToParse, 
		OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
		std::list<OPS::Reprise::CompilerResultMessage>& compilerMessages, 
		const ClangParserSettings& clangParserSettings, bool useMemoryInput);

	bool parseFile(
		OPS::Reprise::RepriseContext& context, const std::string& fileToParse, 
		OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
		std::list<OPS::Reprise::CompilerResultMessage>& compilerMessages, 
		bool useMemoryInput,
		const ClangParserSettings& clangParserSettings)
	{
		return parseFileHelper(
			&context,
			fileToParse, translationUnit, compilerMessages,
			clangParserSettings, useMemoryInput);
	}

	bool parseFile(
		const std::string& fileToParse,
		OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
		std::list<OPS::Reprise::CompilerResultMessage>& compilerMessages, 
		bool useMemoryInput,
		const ClangParserSettings& clangParserSettings)
	{
		return parseFileHelper(
			&OPS::Reprise::RepriseContext::defaultContext(),
			fileToParse, translationUnit, compilerMessages,
			clangParserSettings, useMemoryInput);
	}


	bool parseFileHelper(
		OPS::Reprise::RepriseContext* context, const std::string& fileToParse,
		OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
		std::list<OPS::Reprise::CompilerResultMessage>& compilerMessages, 
		const ClangParserSettings& clangParserSettings, bool useMemoryInput)
	{
		OPS_ASSERT(context != 0)
		llvm::IntrusiveRefCntPtr<DiagnosticIDs> Diags(new DiagnosticIDs);
        llvm::IntrusiveRefCntPtr<DiagnosticsEngine> diagnostic(new DiagnosticsEngine(Diags, new DiagnosticOptions, OPS::Reprise::createRepriseDiagnosticConsumer(compilerMessages).release()));
		std::unique_ptr<clang::ASTConsumer> astConsumer =
			OPS::Reprise::CreateAST2RepriseDumper(*context, *diagnostic, translationUnit);
		//dword t1 = OPS::getTickCount();
		const bool successfullyParsed =
			useMemoryInput
			? clang::parseMemory(
					fileToParse, *diagnostic, astConsumer.get(), clangParserSettings)
			: clang::parseFile(
					fileToParse, *diagnostic, astConsumer.get(), clangParserSettings);
        //OPS::log_console(OPS::Strings::format("clang::parseFile(): %f\n", (OPS::getTickCount() - t1) / 1000.0));
		// Если программы была разобрана без ошибок, то должно создаться дерево Reprise
		OPS_ASSERT(!successfullyParsed || translationUnit.get())
		return successfullyParsed;
	}
}
