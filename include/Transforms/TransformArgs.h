#pragma once

#include "Reprise/Reprise.h"

namespace OPS
{
namespace TransformationsHub
{
	/// Конкретное значение аргумента
	class ArgumentValue
	{
	public:

		enum Type
		{
			RepriseAny,		//< Любой узел Reprise
			StmtAny,		//< Любой оператор
			StmtBlock,		//< Блок операторов
			StmtFor,		//< Цикл for
			StmtIf,			//< Условный оператор
			StmtExpr,		//< Оператор выражение
			StmtSwitch,		//< Оператор switch
			StmtWhile,		//< Цикл while

			/// Оператор присваивания, т.е. StmtExpr у которого
			/// корневое выражение имеет тип ExprAssign
			StmtAssign,

			ExprAny,		//< Любое выражение
			ExprVar,		//< Вхождение переменной
			ExprCall,		//< Вызов функции

			VarDeclaration,  //< Объявление переменной

			TransUnit, //< Единица трансляции

			LastRepriseType = TransUnit,

			Int,			//< Целое число
			Bool,			//< Флаг
			Double,			//< Вещественное число
			String,         //< Строка
			Undefined		//< Не определено
		};

		explicit ArgumentValue(ArgumentValue::Type _type = Undefined);

		Type getType() const { return m_type; }

		OPS::Reprise::RepriseBase*		getAsRepriseObject() const;
		OPS::Reprise::StatementBase*	getAsStatement() const;
		OPS::Reprise::BlockStatement*	getAsBlock() const;
		OPS::Reprise::ForStatement*		getAsFor() const;
		OPS::Reprise::IfStatement*		getAsIf() const;
		OPS::Reprise::ExpressionStatement*  getAsAssign() const;
		OPS::Reprise::ExpressionStatement*	getAsExpr() const;
		OPS::Reprise::WhileStatement*  getAsWhile() const;

		OPS::Reprise::ExpressionBase*		getAsExprNode() const;
		OPS::Reprise::ReferenceExpression*	getAsData() const;
		OPS::Reprise::SubroutineCallExpression*	getAsCall() const;

		OPS::Reprise::VariableDeclaration* getAsVariableDeclaration() const;

		OPS::Reprise::TranslationUnit* getAsTranslationUnit() const;

		int		getAsInt() const;
		bool	getAsBool() const;
		double	getAsDouble() const;
		
		std::string getAsString() const;

		void	setReprise(OPS::Reprise::RepriseBase* objVal);
		void	setInt(int intVal);
		void	setBool(bool boolVal);
		void	setDouble(double doubleVal);

		void    setString(std::string stringVal);

		bool	isValid() const;
		bool	isRepriseArgument() const;

	protected:
		/// Тип значения
		Type	m_type;

		/// Объединение всех возможных значений
		union
		{
			OPS::Reprise::RepriseBase* m_repriseBase;
			
			OPS::Reprise::StatementBase* m_anyStmt;
			OPS::Reprise::BlockStatement* m_blockStmt;
			OPS::Reprise::ForStatement* m_forStmt;
			OPS::Reprise::IfStatement*	m_ifStmt;
			
			OPS::Reprise::ExpressionStatement*	m_assignStmt;
			OPS::Reprise::ExpressionStatement*	m_exprStmt;
			OPS::Reprise::WhileStatement*		m_whileStmt;

			OPS::Reprise::ExpressionBase*		m_anyExpr;
			OPS::Reprise::ReferenceExpression*	m_referenceExpr;
			OPS::Reprise::SubroutineCallExpression*	m_callExpr;

			OPS::Reprise::VariableDeclaration*  m_variableDeclaration;

			OPS::Reprise::TranslationUnit*      m_translationUnit;

			int			m_intArg;
			bool		m_boolArg;
			double		m_doubleArg;
		};

		std::string m_stringArg;

		void reset();
	};

	typedef std::vector<ArgumentValue> ArgumentValues;

	class ArgumentInfo
	{
	public:
		/// Текстовое описание параметра
		std::string			description;
		/// Тип параметра
		ArgumentValue::Type	type;

		/// Обязательный параметр или нет
		bool			mandatory;

		/// Значение по умолчанию
		ArgumentValue	defaultValue;

		/// Список возможных значений параметра
		/**
			Используется только если тип параметра PT_INT.
			В этом случае, 
		**/	
		std::vector<std::string>	valueList;

		ArgumentInfo()
			:type(ArgumentValue::Int)
			,mandatory(true)
			,defaultValue(type)
		{
		}

		ArgumentInfo(ArgumentValue::Type _type, const std::string& _desc = "");

		bool operator == (const ArgumentInfo& other) const;

		static std::string getDefaultDescription(ArgumentValue::Type);

		bool isRepriseArgument() const;

		bool validate(const ArgumentValue& value) const;
		void updateDefaultValue(const ArgumentValue& value);
	};

	class ArgumentsInfo : public std::vector<ArgumentInfo>
	{
	public:
		ArgumentsInfo() {};
		explicit ArgumentsInfo(const ArgumentInfo& argsInfo);

		void addListArg(
			const std::string& description, 
			const std::string& choise1, 
			const std::string& choise2 = "", 
			const std::string& choise3 = "",
			const std::string& choise4 = ""
			);

		bool validate(const ArgumentValues& values) const;
		void updateDefaultValues(const ArgumentValues& values);
	};

	/// Осуществляет трансляцию параметров одной программы в параметры другой
	/// Возвращает true если найдены соответствия для всех аргументов
	bool translateArguments(const OPS::Reprise::ProgramUnit& original,
							OPS::Reprise::ProgramUnit& clone,
							ArgumentValues& vals);

}
}
