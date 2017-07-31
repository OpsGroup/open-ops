#ifndef OPS_FRONTEND_INCLUDED_H__
#define OPS_FRONTEND_INCLUDED_H__

//	OPS includes
#include "Reprise/Reprise.h"
#include "Reprise/ParserResult.h"
#include "ClangParser/clangParserSettings.h"

//	std includes
#include <string>
#include <memory>
#include <vector>

//	Enter namespaces
namespace OPS
{
namespace Frontend
{

OPS_DEFINE_EXCEPTION_CLASS(FrontendError, OPS::Exception)



enum SourceLanguage
{
	/// Language will be guessed by file extension
	AutoLanguage,
	/// C language
	CLanguage,
	/// FORTRAN language
	FortranLanguage,
};

struct Options
{
	inline Options(void) : 	convertTypes(true), convertCommonExpressions(true), convertGeneralExpressions(true),
		convertOtherExpressions(true), convertLiterals(true), convertVariablesInit(true), convertBreakContinue(true),
        convertHirFArrays(true), resolve(true)
	{
	}
	// Canto to Reprise options
	bool convertTypes;
	bool convertCommonExpressions;
	bool convertGeneralExpressions;
	bool convertOtherExpressions;
	bool convertLiterals;
	bool convertVariablesInit;
	bool convertBreakContinue;

	bool convertHirFArrays;

    bool resolve;
};

class Frontend : OPS::NonCopyableMix
{
public:
	Frontend(void);
	~Frontend(void);

	Options& options();
	clang::ClangParserSettings& clangSettings();

	const OPS::Reprise::CompileResult& compileSingleFile(const std::string& fileName, SourceLanguage language = AutoLanguage);

	void addSourceFile(
		const std::string& fileName, SourceLanguage language = AutoLanguage);
	void addSourceBuffer(
		const std::string& memoryBuffer, SourceLanguage language = AutoLanguage);
	void clearSources();

	bool compile(void);
	int getResultCount(void) const;
	const OPS::Reprise::CompileResult& getResult(unsigned index) const;
	const OPS::Reprise::CompileResult& getResultByName(const std::string& fileName) const;

	OPS::Reprise::ProgramUnit& getProgramUnit(void);
	OPS::Reprise::ReprisePtr<OPS::Reprise::ProgramUnit> detachProgramUnit(void);

	static SourceLanguage guessLanguage(const std::string& fileName);
	static std::list<std::string> getFilters();

private:
	typedef std::vector <std::pair<std::string, SourceLanguage> > TFilesList;
	typedef std::map <std::string, OPS::Reprise::CompileResult> TCompileResults;
	typedef std::vector <OPS::Reprise::CompileResult> TBufferCompileResults;

	bool compileC(
		const std::string& fileName, OPS::Reprise::CompileResult& result,
		bool useMemoryInput);
	bool compileFortran(const std::string& fileName);
	void applyCantoToReprise(OPS::Reprise::TranslationUnit& unit);
    void resolve();

	TFilesList m_souceList;
	TFilesList m_sourceBufferList;
	TCompileResults m_results;
	TBufferCompileResults m_bufferResults;
	OPS::Reprise::ProgramUnit* m_program;
	Options m_options;
	clang::ClangParserSettings m_clangSettings;
};


//	Exit namespaces
}
}

#endif
