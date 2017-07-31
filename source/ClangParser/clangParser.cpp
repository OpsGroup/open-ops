/********************************************************
clang 2.7
********************************************************/

#include "OPS_Core/disable_llvm_warnings_begin.h"
#ifdef __GNUC__
#  include <cstddef>
#endif
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/CommandLineSourceLoc.h"
#include "clang/Frontend/Utils.h"
#include "clang/CodeGen/ModuleBuilder.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/ExternalASTSource.h"
#include "clang/Parse/Parser.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Pragma.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/Version.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Config/config.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/ErrorHandling.h"
#include "OPS_Core/disable_llvm_warnings_end.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#if HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

// OPS includes

// Local includes
#include "ClangParser/clangParserSettings.h"
#include "AST2RepriseConsumers.h"

using namespace clang;
using namespace std;

namespace 
{
	//===----------------------------------------------------------------------===//
	// Code generation options
	//===----------------------------------------------------------------------===//

	static llvm::cl::opt<bool> GenerateDebugInfo("g", llvm::cl::ReallyHidden, llvm::cl::desc("Generate source level debug information"));
	static llvm::cl::opt<std::string> TargetCPU("mcpu", llvm::cl::ReallyHidden, llvm::cl::desc("m_target a specific cpu type (-mcpu=help for details)"));
	static llvm::cl::list<std::string> TargetFeatures("target-feature", llvm::cl::ReallyHidden, llvm::cl::desc("m_target specific attributes"));
	static llvm::cl::opt<bool> DisableRedZone("disable-red-zone", llvm::cl::ReallyHidden, llvm::cl::desc("Do not emit code that uses the red zone."), llvm::cl::init(false));
	static llvm::cl::opt<bool> NoImplicitFloat("no-implicit-float", llvm::cl::ReallyHidden, llvm::cl::desc("Don't generate implicit floating point instructions (x86-only)"), llvm::cl::init(false));
}

namespace OPS
{
namespace ClangParser
{

class PragmaHandlerRegistry
{
    typedef clang::PragmaHandler*(*Ctor)(Pragmas&);
    std::list<Ctor> m_ctors;
public:
    static PragmaHandlerRegistry& instance()
    {
        static PragmaHandlerRegistry g_instance;
        return g_instance;
    }

    void registerPragmaHandler(Ctor ctor)
    {
        m_ctors.push_back(ctor);
    }

    void createPragmaHandlers(clang::Preprocessor& pp, Pragmas& pragmas) const
    {
        for(std::list<Ctor>::const_iterator it = m_ctors.begin();
            it != m_ctors.end(); ++it)
        {
            pp.AddPragmaHandler((*it)(pragmas));
        }
    }
};

void registerPragmaHandler(PragmaHandler *(*factory)(Pragmas &))
{
    PragmaHandlerRegistry::instance().registerPragmaHandler(factory);
}

}
}

namespace clang
{
	class ASTConsumeAction : public ASTFrontendAction 
	{
		ASTConsumer *m_astConsumer;
	protected:
		virtual ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef InFile)
		{
			OPS_UNUSED(CI)
			OPS_UNUSED(InFile)
			return m_astConsumer;
		}
	public:
		ASTConsumeAction(ASTConsumer* consumer): m_astConsumer(consumer)
		{
		}
	};

	class ClangParser
	{
		const ClangParserSettings& m_parserSettings;
		const std::string& m_fileToParsePath;
		const bool m_useMemoryInput;

        DiagnosticsEngine& m_diagnostic;
		ASTConsumer* m_consumer;

		typedef std::map<std::string,std::string> MacrosMap;

		ClangParser(ClangParser&);
		ClangParser& operator=(const ClangParser&);

		void initializeLanguageStandard(LangOptions &options, TargetInfo *target, const llvm::StringMap<bool> &features) 
		{
			OPS_UNUSED(features)
			OPS_UNUSED(target)
            options.setStackProtector(LangOptions::SSPOff);

            options.C99 = 1; // C99 Support
            options.C11 = 0; // C11
			options.MicrosoftExt = m_parserSettings.m_compatibilityMode == ClangParserSettings::CM_MICROSOFT; // Microsoft extensions.
			options.MicrosoftMode = m_parserSettings.m_compatibilityMode == ClangParserSettings::CM_MICROSOFT; // Microsoft compat. mode
            options.Borland = 0;  // Borland extensions.
            options.CPlusPlus = 0; // C++ Support
            options.CPlusPlus11 = 0; // C++0x Support
            options.CPlusPlus1y = 0; // C++1y
            options.ObjC1 = 0; // Objective-C 1 support enabled.
            options.ObjC2 = 0; // Objective-C 2 support enabled.
            options.ObjCDefaultSynthProperties = 0; // Objective-C auto-synthesized properties
            options.EncodeExtendedBlockSig = 0; // Encoding extended block type signature
            options.ObjCInferRelatedResultType = 0; // Objective-C related result type inference
            options.Trigraphs = 1; // Trigraphs in source files.
            options.LineComment = 1; // BCPL-style '//' comments.
			options.Bool = m_parserSettings.m_boolKeywords; // 'bool', 'true', 'false' keywords.
            options.WChar = 0; // wchar_t keyword
			options.DollarIdents = 1; // '$' allowed in identifiers.
			options.AsmPreprocessor = 0; // Preprocessor in asm mode.
			options.GNUMode = 0; // True in gnu99 mode false in c99 mode (etc)
			options.GNUKeywords = 0;  // True if GNU-only keywords are allowed
			options.ImplicitInt = 1; // C89 implicit 'int'.
			options.Digraphs = 1; // C94, C99 and C++
			options.HexFloats = 1; // C99 Hexadecimal float constants.
			options.CXXOperatorNames = 0; // Treat C++ operator names as keywords.
			options.AppleKext = 0;  // Allow apple kext features.
			options.PascalStrings = 0; // Allow Pascal strings
			options.WritableStrings = 0; // Allow writable strings
			options.ConstStrings = 0;  // Add const qualifier to strings (-Wwrite-strings)
			options.LaxVectorConversions = 0;
			options.AltiVec = 0; // Support AltiVec-style vector initializers.
			options.Exceptions = 0; // Support exception handling.
            options.ObjCExceptions = 0; // Objective-C exceptions
            options.CXXExceptions = 0; // C++ exceptions
            options.SjLjExceptions = 0; // setjmp-longjump exception handling
            options.TraditionalCPP = 0; // traditional CPP emulation
            options.RTTI = 0; // run-time type information

			options.MSBitfields = 0; // MS-compatible structure layout
			options.Freestanding = 0; // Freestanding implementation
			options.NoBuiltin = 0; // Do not use builtin functions (-fno-builtin)

			options.ThreadsafeStatics = 0; // Whether static initializers are protected by locks.
            options.POSIXThreads = 0; // POSIX thread support
			options.Blocks = 0; // block extension to C
			options.EmitAllDecls = 1; // Emit all declarations, even if they are unused.
			options.MathErrno = 0; // Math functions must respect errno (modulo the platform support).

			options.HeinousExtensions = 0; // Extensions that we really don't like and may be ripped out at any time.
            options.Modules = 0; // modules extension to C
			options.Optimize = 0; // Whether __OPTIMIZE__ should be defined.
			options.OptimizeSize = 0; // Whether __OPTIMIZE_SIZE__ should be defined.
			options.Static = 0; // Should __STATIC__ be defined (as opposed to __DYNAMIC__).
            options.PackStruct = 0; // default struct packing maximum alignment
			options.PICLevel = 0; // The value for __PIC__, if non-zero.
			options.GNUInline = 0; // Should GNU inline semantics be used (instead of C99 semantics).
            options.NoInlineDefine = 0; // Should __NO_INLINE__ be defined.
			options.ObjCGCBitmapPrint = 0; // Enable printing of gc's bitmap layout for __weak/__strong ivars.
			options.AccessControl = 0; // Whether C++ access control should be enabled.
			options.CharIsSigned = 0; // Whether char is a signed or options.type
			options.OpenCL = 0; // OpenCL C99 language extensions.
		}

		/// createTargetTriple - Process the various options that affect the target
		/// triple and build a final aggregate triple that we are compiling for.
		static std::string createTargetTriple() 
		{
            return llvm::sys::getDefaultTargetTriple();
		}

		//===----------------------------------------------------------------------===//
		// SourceManager initialization.
		//===----------------------------------------------------------------------===//

		bool initializeSourceManager(Preprocessor &preprocessor, const std::string &inFile) 
		{
			// Figure out where to get and map in the main file.
			SourceManager &sourceMgr = preprocessor.getSourceManager();
			FileManager &fileMgr = preprocessor.getFileManager();
			assert(inFile != "-");
			const FileEntry *file = fileMgr.getFile(inFile);
			if (file) 
			{
				sourceMgr.createMainFileID(file);
			}
			if (sourceMgr.getMainFileID().isInvalid()) 
			{
				preprocessor.getDiagnostics().Report(FullSourceLoc(), diag::err_fe_error_reading) << inFile.c_str();
				return true;
			}
			return false;
		}

		bool initializeSourceManagerWithBuffer(Preprocessor &preprocessor, const std::string &source) 
		{
			// Figure out where to get and map in the main file.
			SourceManager &sourceMgr = preprocessor.getSourceManager();
			const llvm::MemoryBuffer* memoryBuffer =
				llvm::MemoryBuffer::getMemBuffer(source);
			sourceMgr.createMainFileIDForMemBuffer(memoryBuffer);

			return false;
		}

		static void initializeIncludePaths(
			CompilerInstance& ci,
			const char *argv0, 
			const ClangParserSettings& clangParserSettings, 
			const LangOptions &langOptions) 
		{
			clang::HeaderSearchOptions& headerSearchOptions = ci.getHeaderSearchOpts();
            headerSearchOptions.UseStandardSystemIncludes = clangParserSettings.m_useStandardIncludes;
			headerSearchOptions.UseBuiltinIncludes = clangParserSettings.m_useBuiltinIncludes;

			if (headerSearchOptions.UseBuiltinIncludes && headerSearchOptions.ResourceDir.empty())
			{
				headerSearchOptions.ResourceDir = CompilerInvocation::GetResourcesPath(argv0, 0);
			}

			// Add additional include paths
			for(StringList::const_iterator it = clangParserSettings.m_userIncludePaths.begin(); it != clangParserSettings.m_userIncludePaths.end(); ++it)
			{
				const std::string& path = *it;
				headerSearchOptions.AddPath(path, clang::frontend::Angled, false, false);
			}
			// Apply added paths to PP.
			ApplyHeaderSearchOptions(ci.getPreprocessor().getHeaderSearchInfo(), 
				headerSearchOptions, 
				langOptions, 
				ci.getTarget().getTriple());
		}

		void initializeMacroDefs(PreprocessorOptions& opts)
		{
			if (m_parserSettings.m_compatibilityMode != ClangParserSettings::CM_GCC)
			{
				opts.addMacroUndef("__GNUC__");
				opts.addMacroUndef("__GNUC_MINOR__");
				opts.addMacroUndef("__GNUC_PATCHLEVEL__");
				opts.addMacroUndef("__GXX_ABI_VERSION");
			}

			// Add user defines...
			for(StringToStringMap::const_iterator it = m_parserSettings.m_userDefines.begin(); 
				it != m_parserSettings.m_userDefines.end(); ++it)
			{
				opts.addMacroDef(it->first + "=" + it->second);
			}
			// ...and user undefines
			for(StringList::const_iterator it = m_parserSettings.m_userUndefines.begin(); 
				it != m_parserSettings.m_userUndefines.end(); ++it)
			{
				opts.addMacroUndef(*it);
			}
		}

		//===----------------------------------------------------------------------===//
		// Basic Parser driver
		//===----------------------------------------------------------------------===//
/*
		void parseFile(Preprocessor &preprocessor, MinimalAction *minAction) 
		{
			Parser parse(preprocessor, *minAction);
			preprocessor.EnterMainSourceFile();

			// Parsing the specified input file.
			parse.ParseTranslationUnit();
			delete minAction;
		}
*/
		/// ComputeTargetFeatures - Recompute the target feature list to only
		/// be the list of things that are enabled, based on the target cpu
		/// and feature list.
		void computeFeatureMap(TargetInfo *target, llvm::StringMap<bool> &features) 
		{
			assert(features.empty() && "invalid map"); 

			// Initialize the feature map based on the target.
            target->getDefaultFeatures(features);

			// Apply the user specified deltas.
			for (llvm::cl::list<std::string>::iterator it = TargetFeatures.begin(),  ie = TargetFeatures.end(); it != ie; ++it) 
			{
				const char *name = it->c_str();

				// FIXME: Don't handle errors like this.
				if (name[0] != '-' && name[0] != '+') 
				{
					const std::string msg = std::string("error: clang-cc: invalid target feature string: ") + *it;
					throw std::runtime_error(msg.c_str());
				}
				if (!target->setFeatureEnabled(features, name + 1, (name[0] == '+'))) 
				{
					const std::string msg = std::string("error: clang-cc: invalid target feature name: ") + *it;
					throw std::runtime_error(msg.c_str());
				}
			}
		}


		/// processInputFile - Process a single input file with the specified state.
		void processInputFile(
			Preprocessor &preprocessor, LangOptions& langOpts,
			const std::string &inFile, ASTConsumer* consumer,
			bool useMemoryInput)
		{
			llvm::OwningPtr<llvm::raw_ostream> outputStream;
			bool clearSourceMgr = false;
			llvm::OwningPtr<ASTContext> contextOwner;
			if (consumer)
			{
				contextOwner.reset(new ASTContext(langOpts,
					preprocessor.getSourceManager(),
                    &preprocessor.getTargetInfo(),
					preprocessor.getIdentifierTable(),
					preprocessor.getSelectorTable(),
					preprocessor.getBuiltinInfo(), 0));
			}

			llvm::OwningPtr<ExternalASTSource> source;

			if (useMemoryInput)
			{
				if (initializeSourceManagerWithBuffer(preprocessor, inFile))
					return;
			}
			else
			{
				if (initializeSourceManager(preprocessor, inFile))
					return;
			}

			// If we have an ASTConsumer, run the parser with it.
			if (consumer)
                ParseAST(preprocessor, consumer, *contextOwner.get(), false, TU_Complete);

			// Disable the consumer prior to the context, the consumer may perform actions
			// in its destructor which require the context.
			// Consumer.reset();

			contextOwner.reset(); // Delete ASTContext

			// For a multi-file compilation, some things are ok with nuking the source
			// manager tables, other require stable fileid/macroid's across multiple
			// files.
			if (clearSourceMgr)
				preprocessor.getSourceManager().clearIDTables();

			// Always delete the output stream because we don't want to leak file
			// handles.  Also, we don't want to try to erase an open file.
			outputStream.reset();

			// Store macro constants into the map
			if (!preprocessor.getDiagnostics().hasErrorOccurred())
			{
				if (m_parserSettings.m_parsedMacrosMap != 0)
				{
					{
						for(clang::Preprocessor::macro_iterator it = preprocessor.macro_begin();it != preprocessor.macro_end(); ++it)
						{
							MacroInfo* macroInfo = it->second->getMacroInfo();
							if (macroInfo && 
								!macroInfo->isFunctionLike() && 
								!macroInfo->isGNUVarargs() && 
								!macroInfo->isC99Varargs() && 
								!macroInfo->isBuiltinMacro()
								)
							{
								string macroLiteral;
								for(MacroInfo::tokens_iterator tokenIt = macroInfo->tokens_begin(); tokenIt != macroInfo->tokens_end();/* инкремент не нужен!*/)
								{
									if (tokenIt->isLiteral())
									{
										const char* literalStart = tokenIt->getLiteralData();
										const char* literalEnd = tokenIt->getLiteralData() + tokenIt->getLength();
										macroLiteral += string(literalStart, literalEnd);
									}
									break;
								}
								if (!macroLiteral.empty())
								{
									m_parserSettings.m_parsedMacrosMap->insert(MacrosMap::value_type(it->first->getName(), macroLiteral));
								}
							}
						}
					}
				}
			}
			else /* preprocessor.getDiagnostics().getNumErrors() != 0 */ 
			{
			}
		}

		static void llvmErrorHandler(
			void *userData,
			const std::string &message,
			bool gen_crash_diag) 
		{
			DiagnosticsEngine &diagnostic = *static_cast<DiagnosticsEngine*>(userData);
			diagnostic.Report(FullSourceLoc(), diag::err_fe_error_backend) << message;
			// We cannot recover from llvm errors.
			throw std::runtime_error("Cannot recover from llvm errors");
		}

	public:

		ClangParser(const std::string& fileToParsepath, 
            DiagnosticsEngine& diagnostic,
			ASTConsumer* consumer, 
			const ClangParserSettings& parserSettings,
			bool useMemoryInput): 
				m_parserSettings(parserSettings),
				m_fileToParsePath(fileToParsepath),
				m_useMemoryInput(useMemoryInput),
				m_diagnostic(diagnostic), 
				m_consumer(consumer)
		  {
		  }

		  bool parseFile()
		  {
			  clang::CompilerInstance Clang;
			  Clang.setDiagnostics(&m_diagnostic);
			  // Set an error handler, so that any LLVM backend diagnostics go through our error handler.
              if (!llvm::llvm_is_multithreaded())
                llvm::install_fatal_error_handler(llvmErrorHandler, static_cast<void*>(&Clang.getDiagnostics()));
			  // Compute the feature set, unfortunately this effects the language!
			  llvm::StringMap<bool> features;
			  // computeFeatureMap(target.get(), features);
			  if (!m_fileToParsePath.empty())
			  {
                  clang::TargetOptions& options = Clang.getTargetOpts();
                  if (m_parserSettings.m_targetTriple.empty())
                      options.Triple = createTargetTriple();
                  else
                      options.Triple = m_parserSettings.m_targetTriple; // "x86_64-unknown-linux-gnuclang"
				  TargetInfo *targetInfo = TargetInfo::CreateTargetInfo(m_diagnostic, &options);

				  Clang.setTarget(targetInfo);
				  Clang.createFileManager();
				  Clang.createSourceManager(Clang.getFileManager());

				  LangOptions& langInfo = Clang.getLangOpts();
				  initializeLanguageStandard(langInfo, &Clang.getTarget(), features);

				  initializeMacroDefs(Clang.getPreprocessorOpts());
				  // NB! 
				  // Препроцессор необходимо создавать только после инициализации свойств языка - LangOptions.
				  Clang.createPreprocessor();


				  initializeIncludePaths(Clang, m_fileToParsePath.c_str(), m_parserSettings, langInfo);
				  Preprocessor& PP = Clang.getPreprocessor();
				  PP.getBuiltinInfo().InitializeBuiltins(PP.getIdentifierTable(), langInfo);

                  OPS::Reprise::AST2RepriseConverter* pConverter = static_cast<OPS::Reprise::AST2RepriseConverter*>(m_consumer);
                  OPS::ClangParser::PragmaHandlerRegistry::instance().createPragmaHandlers(Clang.getPreprocessor(), pConverter->getPragmas());

					processInputFile(
						Clang.getPreprocessor(), langInfo, m_fileToParsePath,
						m_consumer, m_useMemoryInput);
			  }
			  else
			  {
				  // FIXME:
				  throw std::runtime_error("No input file specified");
			  }
              if (!llvm::llvm_is_multithreaded())
                llvm::remove_fatal_error_handler();
			  //Clang.takeDiagnosticClient();
			  //Clang.takeDiagnostics();
			  return (m_diagnostic.hasErrorOccurred() == false);
		  }
	};

	bool parseFile(
		const std::string& fileToParse,
		DiagnosticsEngine& diagnostic,
		ASTConsumer* consumer, 
		const ClangParserSettings& clangParserSettings /*= ClangParserSettings::defaultSettings()*/)
	{
		return ClangParser(fileToParse, diagnostic, consumer, clangParserSettings, false).parseFile();
	}

	bool parseMemory(
		const std::string& memoryBuffer, 
		DiagnosticsEngine& diagnostic,
		ASTConsumer* consumer, 
		const ClangParserSettings& clangParserSettings /*= ClangParserSettings::defaultSettings()*/)
	{
		return ClangParser(memoryBuffer, diagnostic, consumer, clangParserSettings, true).parseFile();
	}
}
