#ifndef OPS_CLANGPARSER_CLANGPARSER_H__
#define OPS_CLANGPARSER_CLANGPARSER_H__

#include <string>
#include <map>
#include <list>
#include <memory>

#include "Reprise/ParserResult.h"
#include "ClangParser/clangParserSettings.h"

// Предварительные объявления
namespace OPS
{
	namespace Reprise
	{
		class TranslationUnit;
		class RepriseContext;
	}
}

namespace clang
{
	class DiagnosticsEngine;
	class ASTConsumer;

	/// Разбор файла парсером Си-clang
	/// \param context контекст разбора
	/// \param fileToParse путь к разбираемому файлу
	/// \param translationUnit ссылка на TranslationUnit, в который будет разобрано дерево программы
	/// \param compilerMessages диагностические сообщения компилятора: ошибки, предупреждения, советы
	/// \param clangParserSettings настройки парсера для разбора программы
	/// \return true, если программа была разобрана без ошибок
	bool parseFile(
		OPS::Reprise::RepriseContext& context, const std::string& fileToParse, 
		OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
		std::list<OPS::Reprise::CompilerResultMessage>& compilerMessages,
		bool useMemoryInput,
		const ClangParserSettings& clangParserSettings = ClangParserSettings::defaultSettings());

	/// Разбор файла парсером Си-clang
	/// \param fileToParse путь к разбираемому файлу
	/// \param translationUnit ссылка на TranslationUnit, в который будет разобрано дерево программы
	/// \param compilerMessages диагностические сообщения компилятора: ошибки, предупреждения, советы
	/// \param clangParserSettings настройки парсера для разбора программы
	/// \return true, если программа была разобрана без ошибок
	bool parseFile(
		const std::string& fileToParse,
		OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
		std::list<OPS::Reprise::CompilerResultMessage>& compilerMessages, 
		bool useMemoryInput,
		const ClangParserSettings& clangParserSettings = ClangParserSettings::defaultSettings());

	/// Разбор файла парсером Си-clang (не используйте эту функцию, если можете обойтись её перегруженным аналогом (см. выше))
	bool parseFile(
		const std::string& fileToParse, 
		DiagnosticsEngine& diagnostic,
		ASTConsumer* consumer, 
		/*PragmaHandler* distributionHandler,*/
		const ClangParserSettings& clangParserSettings = ClangParserSettings::defaultSettings());

	bool parseMemory(
		const std::string& memoryBuffer, 
		DiagnosticsEngine& diagnostic,
		ASTConsumer* consumer, 
		/*PragmaHandler* distributionHandler,*/
		const ClangParserSettings& clangParserSettings = ClangParserSettings::defaultSettings());
}

#endif
