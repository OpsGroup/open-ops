#ifndef OPS_CLANGPARSER_CLANGPARSER_SETTINGS_H__
#define OPS_CLANGPARSER_CLANGPARSER_SETTINGS_H__

#include <string>
#include <map>
#include <list>

namespace clang
{
	typedef std::map<std::string,std::string> StringToStringMap;
	typedef std::list<std::string> StringList;

	/// Настройки для Си-парсера clang
	struct ClangParserSettings
	{
		enum CompatibilityMode
		{
			CM_NONE,
			CM_MICROSOFT /*®*/,
			CM_GCC
		};

		/// Поля

        /// Идентификатор целевой платформы
        std::string m_targetTriple;
		/// Список дополнительных макроопределений, инициализируемых непосредственно перед разбором программы
		/// (вида #define NAME VALUE, например "#define PI 3.14)
		StringToStringMap m_userDefines;
		/// Список дополнительных макро-деопределений, инициализируемых непосредственно перед разбором программы
		/// (вида #undef NAME, например "#undef DEBUG)
		StringList m_userUndefines;
		/// Список дополнительных каталогов для поиска заголовочных (*.h) файлов
		StringList m_userIncludePaths;
		/// Добавлять ли автоматически стандартные пути для поиска заголовочных файлов
		bool m_useStandardIncludes;
		/// Добавлять ли автоматически пути для поиска встроенных заголовочных файлов
		bool m_useBuiltinIncludes;
		/// Считать ли bool, true и false ключевыми словами
		bool m_boolKeywords;
		/// Использовать режим совместимости
		CompatibilityMode m_compatibilityMode;
		/// Указатель на map, в который, если он не равен 0, будут записаны макроопределения констант разбираемой программы.
		/// Например, если в программе встречается определение "#define PI 3.14", то в m_parsedMacrosMap после разбора будет "PI" => "3.14".
		/// В частности в m_parsedMacrosMap автоматически попадут макросы из m_userDefines.
		/// Если указатель равен 0, макросы записаны не будут, а разбор программы немного ускорится.
		StringToStringMap* m_parsedMacrosMap;

		/// Конструктор
		ClangParserSettings(StringToStringMap* parsedMacrosMap = 0);

		/// Методы

		/// Добавляет \p additionalIncludePath в список каталогов, в которых может находиться заголовочный файл
		/// \param additionalIncludePath путь к каталогу с заголовочными файлами. ВНИМАНИЕ! Путь НЕ должен оканчиваться на \ или /
		void addIncludePath(const std::string& additionalIncludePath);

        /// Ищет и добавляет стандартные папки инклюдов в список каталогов, в которых может находиться заголовочный файл
        void initWithStdSettingsForCurrentPlatform();

		/// Добавляет в разбираемую программу макроопределение "#define name value"
		void defineMacro(const std::string& name, const std::string& value = "");

		/// Добавляет в разбираемую программу макро-деопределение "#undef name"
		void undefineMacro(const std::string& name);

		/// Статические методы

		/// Настройки по-умолчанию
		static const ClangParserSettings& defaultSettings();

		/// Эмуляция MSVC
		static ClangParserSettings msvcSettings();

		/// Эмуляция gcc
		static ClangParserSettings gccSettings();
	};
}
#endif

