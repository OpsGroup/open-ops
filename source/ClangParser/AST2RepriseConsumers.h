//===--- AST2RepriseConsumers.h - clang::ASTConsumer implementations -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// AST Consumers.
//
//===----------------------------------------------------------------------===//

#ifndef DRIVER_AST2RepriseConsumers_H
#define DRIVER_AST2RepriseConsumers_H

#include <string>
#include <memory>
#include <map>

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4146 4244 4345 4800)
#endif
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/Diagnostic.h"
#include "llvm/Support/raw_ostream.h"
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#include "Reprise/Reprise.h"
#include "ClangParser/PragmaHandler.h"

namespace llvm {
    class raw_ostream;
    class Module;
    class LLVMContext;
    namespace sys { class Path; }
}
namespace clang 
{
    class ASTConsumer;
    class Diagnostic;
    class FileManager;
    class Preprocessor;
    class PreprocessorFactory;
    class CompileOptions;
    class LangOptions;
    class Diagnostic;
	class Token;
	class SourceLocation;
}

namespace OPS
{
    namespace Reprise
    {

		class AST2RepriseConverter;

		class AST2RepriseConverter : public clang::ASTConsumer
		{
			OPS::Reprise::RepriseContext& m_repriseContext;
			clang::ASTContext *m_astContext;

            OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& m_translationUnit;
            clang::DiagnosticsEngine &m_diagnostic;
			llvm::raw_ostream *m_outputStream;

			AST2RepriseConverter(const AST2RepriseConverter&);
			AST2RepriseConverter& operator=(const AST2RepriseConverter&);

            ClangParser::Pragmas m_pragmas;
			
		public:
			AST2RepriseConverter(
				OPS::Reprise::RepriseContext& repriseContext,
                ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
                clang::DiagnosticsEngine &diagnostic,
				llvm::raw_ostream* outputStream);

            ClangParser::Pragmas& getPragmas() { return m_pragmas; }

            virtual void HandleTranslationUnit(clang::ASTContext &astContext);
        };


		class RepriseContext;
        class TranslationUnit;
        // AST pretty-printer: prints out the AST in a format that is close to the
        // original C code.  The output is intended to be in a format such that
        // clang could re-parse the output back into the same AST, but the
        // implementation is still incomplete.
        std::unique_ptr<clang::ASTConsumer> CreateAST2RepriseDumper(
			OPS::Reprise::RepriseContext& context,
            clang::DiagnosticsEngine& diagnostic,
            OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& translation_unit,
            llvm::raw_ostream* outputStream = 0);
    }
}
#endif
