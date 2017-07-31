#define USE_CLANG 1
// #define USE_F2003 OPS_HAVE_ANTLR
#define USE_F2003 0

#include "Frontend/Frontend.h"
#if USE_CLANG
#include "ClangParser/clangParser.h"
#include "Reprise/Reprise.h"
using namespace clang;
using namespace std;
#endif
#if USE_F2003
#include "F2003Parser/FParser.h"
#endif
#include "FrontTransforms/CantoToReprise.h"
#include "FrontTransforms/Resolver.h"


#include <fstream>


//	Namespaces using
using namespace OPS::Reprise;

//	Enter namespace
namespace OPS
{
namespace Frontend
{

Frontend::Frontend(void) : m_program(new ProgramUnit()), m_clangSettings(ClangParserSettings::defaultSettings())
{
}

Frontend::~Frontend(void)
{
	delete m_program;
}

Options& Frontend::options()
{
	return m_options;
}

clang::ClangParserSettings& Frontend::clangSettings()
{
	return m_clangSettings;
}

const CompileResult& Frontend::compileSingleFile(const std::string& fileName, SourceLanguage language)
{
	delete m_program;
	m_program = new ProgramUnit();
	m_souceList.clear();
	m_sourceBufferList.clear();
	m_results.clear();
	m_bufferResults.clear();
	addSourceFile(fileName, language);
	compile();
	return getResultByName(fileName);
}

void Frontend::addSourceFile(
	const std::string& fileName, SourceLanguage language)
{
	m_souceList.push_back(make_pair(fileName, language));
	m_results.insert(std::make_pair(fileName, CompileResult()));
}

void Frontend::addSourceBuffer(
	const std::string& memoryBuffer, SourceLanguage language)
{
	m_sourceBufferList.push_back(make_pair(memoryBuffer, language));
	m_bufferResults.push_back(CompileResult());
}

void Frontend::clearSources()
{
	m_souceList.clear();
	m_sourceBufferList.clear();
	m_results.clear();
	m_bufferResults.clear();
	delete m_program;
	m_program = new ProgramUnit;
}

bool Frontend::compile(void)
{
	bool success = true;

	for (TFilesList::const_iterator source = m_souceList.begin(); source != m_souceList.end(); ++source)
	{
		SourceLanguage lang = source->second == AutoLanguage 
			? guessLanguage(source->first)
			: source->second;

		switch(lang)
		{
#if USE_CLANG
		case CLanguage:
			success &= compileC(source->first, m_results[source->first], false);
			break;
#endif
#if USE_F2003
		case FortranLanguage:
			success &= compileFortran(source->first);
			break;
#endif
		OPS_DEFAULT_CASE_LABEL
		}
	}

	TBufferCompileResults::iterator result = m_bufferResults.begin();
	for (
		TFilesList::const_iterator source = m_sourceBufferList.begin();
		source != m_sourceBufferList.end(); ++ source, ++ result)
	{
		SourceLanguage lang = source->second == AutoLanguage 
			? CLanguage
			: source->second;

		switch(lang)
		{
#if USE_CLANG
		case CLanguage:
			success &= compileC(source->first, *result, true);
			break;
#endif
		OPS_DEFAULT_CASE_LABEL
		}
	}

	if (m_options.resolve)
	{
			resolve();
	}

	return success;
}

static const char* const FortranExtensions[] = {"f", "for", "f90", "f95", "f2k"};

SourceLanguage Frontend::guessLanguage(const std::string& fileName)
{
	size_t pos = fileName.rfind('.');
	std::string ext = pos != std::string::npos
		? Strings::tolower(fileName.substr(pos + 1))
		: "";

	for(size_t i = 0; i < sizeof(FortranExtensions)/sizeof(FortranExtensions[0]); ++i)
	{
		if (ext == FortranExtensions[i])
			return FortranLanguage;
	}

	// default - C language
	return CLanguage;
}

std::list<std::string> Frontend::getFilters()
{
	std::list<std::string> filters;

#if USE_CLANG
	filters.push_back("*.c");
#endif

#if USE_F2003
	for(size_t i = 0; i < sizeof(FortranExtensions)/sizeof(FortranExtensions[0]); ++i)
	{
		filters.push_back(std::string("*.") + FortranExtensions[i]);
	}
#endif
	return filters;
}


#if USE_CLANG
bool Frontend::compileC(
	const std::string& fileName, CompileResult& result, bool useMemoryInput)
{
	try
	{
		ReprisePtr<OPS::Reprise::TranslationUnit> unit;
		if (!useMemoryInput)
			result.m_fileName = fileName;

		clang::parseFile(
			fileName, unit, result.m_messages,
			useMemoryInput, m_clangSettings);

		if (unit.get() != 0)
		{
			applyCantoToReprise(*unit);
#ifdef _DEBUG
			OPS::Reprise::Editing::checkParentChildRelations(*unit);
#endif
			m_program->addTranslationUnit(unit);
			return true;
		}
		else
		{
//			m_results[fileName].errorCount = 1;
//			m_results[fileName].errorText = "Unknown reason.";
			return false;
		}
	}
	catch(std::exception& e)
	{
		m_results[fileName].m_messages.push_back(
			CompilerResultMessage(CompilerResultMessage::CRK_ERROR, e.what(), "", fileName, 
			CompilerResultMessage::TLocation(0,0), ""));
		return false;
	}
}
#endif

#if USE_F2003
bool Frontend::compileFortran(const std::string& fileName)
{
	try
	{
		m_results[fileName].m_fileName = fileName;
		Fortran2003Parser::FortranParser parser(Fortran2003Parser::FortranParserSettings::createDefault());
		std::unique_ptr<TranslationUnit> unit(parser.parseFile(fileName));

		const OPS::Reprise::CompileResult& result = parser.getCompileResult();
		m_results[fileName].m_messages = result.m_messages;

		if (unit.get() != 0)
		{
			ReprisePtr<TranslationUnit> reprise_unit(unit.release());
			applyCantoToReprise(*reprise_unit);
			if (m_options.convertHirFArrays)
			{
				C2R::convertHirFArrays(*m_program);
			}
#ifdef _DEBUG
			OPS::Reprise::Editing::checkParentChildRelations(*reprise_unit);
#endif
			m_program->addTranslationUnit(reprise_unit);
//			C2R::convertHirFArrays(*m_program);
//			C2R::convertStrictToBasic(*m_program);
			return true;
		}
		return false;
	}
	catch(std::exception& e)
	{
		m_results[fileName].m_messages.push_back(CompilerResultMessage(CompilerResultMessage::CRK_FATAL, e.what(), 
			"", fileName, std::make_pair(1, 1), ""));
		return false;
	}
}
#endif

void Frontend::applyCantoToReprise(TranslationUnit& unit)
{
	if (m_options.convertTypes)
	{
		C2R::convertTypes(unit);
	}
	if (m_options.convertVariablesInit)
	{
		C2R::convertVariablesInit(unit);
	}
	if (m_options.convertCommonExpressions || m_options.convertGeneralExpressions || m_options.convertOtherExpressions)
	{
		C2R::convertExpressions(unit, m_options.convertCommonExpressions, m_options.convertGeneralExpressions, m_options.convertOtherExpressions);
	}
	if (m_options.convertLiterals)
	{
		C2R::convertLiterals(unit);
	}
	if (m_options.convertBreakContinue)
	{
		C2R::convertBreakContinue(unit);
	}
}

void Frontend::resolve()
{
    OPS::Transforms::Resolver resolver;
    resolver.setProgram(*m_program);
    resolver.resolve();

    int errorCount = resolver.getErrorCount();

    for(int i = 0; i < errorCount; ++i)
    {
        TCompileResults::reference result = *m_results.begin();
        result.second.m_messages.push_back(
                    CompilerResultMessage(CompilerResultMessage::CRK_ERROR,
                                          resolver.getError(i),
                                          "", result.first, std::make_pair(1, 1), ""));
    }
}

int Frontend::getResultCount(void) const
{
	return static_cast <int> (
		m_souceList.size() + m_sourceBufferList.size());
}

const CompileResult& Frontend::getResult(const unsigned index) const
{
	if (int(index) >= getResultCount())
		throw FrontendError(
			Strings::format(
				"Unexpected getting compile result for index (%u).", index));

	if (index < m_souceList.size())
		return getResultByName(m_souceList[index].first);

	return m_bufferResults[index - m_souceList.size()];
}

const CompileResult& Frontend::getResultByName(const std::string& fileName) const 
{
	TCompileResults::const_iterator result = m_results.find(fileName);
	if (result != m_results.end())
		return result->second;
	else
		throw FrontendError(Strings::format("Unexpected getting compile result for file '%s'.", fileName.c_str()));
}

ProgramUnit& Frontend::getProgramUnit(void)
{
	return *m_program;
}

ReprisePtr<ProgramUnit> Frontend::detachProgramUnit(void)
{
	ReprisePtr<ProgramUnit> program(m_program);
	m_program = 0;
	return program;
}


//	Exit namespace
}
}
