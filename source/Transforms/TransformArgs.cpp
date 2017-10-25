#include "Transforms/TransformArgs.h"
#include "Shared/ReferenceMapper.h"
#include "OPS_Core/Localization.h"
#include <cmath>

static inline void OPSAssert(bool val, const std::string& message)
{
	if (!val) throw OPS::AssertionFailed(message);
}

namespace OPS {
namespace TransformationsHub {

	using namespace OPS::Reprise;

	bool ArgumentValue::isRepriseArgument() const
	{
		return m_type <= LastRepriseType;
	}

	ArgumentValue::ArgumentValue(Type _type)
		:m_type(_type)
	{
		reset();
	}

	RepriseBase*	ArgumentValue::getAsRepriseObject() const
	{
		OPSAssert(isRepriseArgument(), "Parameter is not IR object");
		return m_repriseBase;
	}

	StatementBase*	ArgumentValue::getAsStatement() const
	{
		OPSAssert(m_type == StmtAny, "Parameter is not statement");
		return m_anyStmt;
	}

	BlockStatement* ArgumentValue::getAsBlock() const
	{
		OPSAssert(m_type == StmtBlock, "Parameter is not block");
		return m_blockStmt;
	}
	ForStatement* ArgumentValue::getAsFor() const
	{
		OPSAssert(m_type == StmtFor, "Parameter is not for loop");
		return m_forStmt;
	}
	IfStatement*		ArgumentValue::getAsIf() const
	{
		OPSAssert(m_type == StmtIf, "Parameter is not if operator");
		return m_ifStmt;
	}
	WhileStatement* ArgumentValue::getAsWhile() const
	{
		OPSAssert(m_type == StmtWhile, "Parameter is not while loop");
		return m_whileStmt;
	}

	ExpressionBase*	ArgumentValue::getAsExprNode() const
	{
		OPSAssert(m_type == ExprAny, "Parameter is not expression");
		return m_anyExpr;
	}
	ReferenceExpression*	ArgumentValue::getAsData() const
	{
		OPSAssert(m_type == ExprVar, "Parameter is not vairable");
		return m_referenceExpr;
	}
	ExpressionStatement* ArgumentValue::getAsAssign() const
	{
		OPSAssert(m_type == StmtAssign, "Parameter is not assignment statement");
		return m_assignStmt;
	}
	ExpressionStatement* ArgumentValue::getAsExpr() const 
	{
		OPSAssert(m_type == StmtExpr, "Parameter is not expression statement");
		return m_exprStmt;
	}
	SubroutineCallExpression*	ArgumentValue::getAsCall() const
	{
		OPSAssert(m_type == ExprCall, "Parameter is not function call expression");
		return m_callExpr;
	}
	VariableDeclaration* ArgumentValue::getAsVariableDeclaration() const
	{
		OPSAssert(m_type == VarDeclaration, "Parameter is not variable declaration");
		return m_variableDeclaration;
	}
	TranslationUnit* ArgumentValue::getAsTranslationUnit() const
	{
		OPSAssert(m_type == TransUnit, "Parameter is not variable declarationtranslation unit");
		return m_translationUnit;
	}

	int ArgumentValue::getAsInt() const
	{
		OPSAssert(m_type == Int, "Parameter is not integer");
		return m_intArg;
	}
	bool ArgumentValue::getAsBool() const
	{
		OPSAssert(m_type == Bool, "Parameter is not boolean");
		return m_boolArg;
	}
	double ArgumentValue::getAsDouble() const
	{
		OPSAssert(m_type == Double, "Parameter is not double");
		return m_doubleArg;
	}

	std::string ArgumentValue::getAsString() const
	{
		OPSAssert(m_type == String, "Parameter is not string");
		return m_stringArg;
	}

	void ArgumentValue::reset()
	{
		OPSAssert(m_type != Undefined, "Parameter type is undefined");

		if (m_type == Int)
			setInt(0);
		else if (m_type == Bool)
			setBool(false);
		else if (m_type == Double)
			setDouble(0.0);
		else if(m_type == String)
			setString("");
		else
			setReprise(0);
	}

	void ArgumentValue::setReprise(RepriseBase* objVal)
	{
		OPSAssert(isRepriseArgument(), "Parameter type is not IR object");
		if (objVal == 0) {
			m_repriseBase = 0;
			return;
		}

		switch(m_type)
		{
		case RepriseAny:
			m_repriseBase = objVal; break;
		case StmtAny:
			{
				m_anyStmt = dynamic_cast<StatementBase*>(objVal);
				if (!m_anyStmt)
				{
					ExpressionBase* exprNode = dynamic_cast<ExpressionBase*>(objVal);
					if (exprNode &&
						exprNode->getParent())
					{
						// Тут приведение именно к StmtExpr, т.к. это оператор без
						// ключевого слова, поэтому пользователь не может его никак
						// выделить кроме как тыкнув по выражению. 
						// У остальных операторов есть ключевые слова, поэтому они 
						// выделяются явно.
						m_anyStmt = dynamic_cast<ExpressionStatement*>(exprNode->getParent()); break;
					}
				}
				break;
			}
		case StmtBlock:	
			{
				m_blockStmt = dynamic_cast<BlockStatement*>(objVal);

				if (m_blockStmt == 0) {
					ForStatement* forStmt = dynamic_cast<ForStatement*>(objVal);
					if (forStmt) {
						m_blockStmt = &forStmt->getBody();
					}
				}
			}
			 break;

		case StmtFor:   m_forStmt = dynamic_cast<ForStatement*>(objVal); break;
		case StmtIf:	m_ifStmt = dynamic_cast<IfStatement*>(objVal); break;
		case StmtAssign:
			{
				m_assignStmt = dynamic_cast<ExpressionStatement*>(objVal);
				if (!m_assignStmt) {
					ExpressionBase* pExpr = dynamic_cast<ExpressionBase*>(objVal);
					if (pExpr && pExpr->getParent())
						m_assignStmt = dynamic_cast<ExpressionStatement*>( pExpr->getParent() );
				}

				OPS_ASSERT(!"Нужна проверка на операцию присваивания");
			}
			break;
		case StmtExpr:
			{
				m_exprStmt = dynamic_cast<ExpressionStatement*>(objVal);
				if (!m_exprStmt)
				{
					ExpressionBase* exprNode = dynamic_cast<ExpressionBase*>(objVal);
					if (exprNode && exprNode->getParent())
						m_exprStmt = dynamic_cast<ExpressionStatement*>(exprNode->getParent());
				}
			}
			break;
		case StmtWhile:  m_whileStmt = dynamic_cast<WhileStatement*>(objVal); break;

		case ExprAny:	m_anyExpr = dynamic_cast<ExpressionBase*>(objVal); break;
		case ExprVar:
			{
				m_referenceExpr = dynamic_cast<ReferenceExpression*>(objVal);
			}
			break;
		case ExprCall:  m_callExpr = dynamic_cast<SubroutineCallExpression*>(objVal); break;
		case VarDeclaration:
			m_variableDeclaration = dynamic_cast<VariableDeclaration*>(objVal); break;
		case TransUnit:
			m_translationUnit = dynamic_cast<TranslationUnit*>(objVal); break;
		OPS_DEFAULT_CASE_LABEL;
		}
	}

	void ArgumentValue::setBool(bool boolVal)
	{
		if (m_type != Bool)
			throw StateError("");
		m_boolArg = boolVal;
	}

	void ArgumentValue::setInt(int intVal)
	{
		if (m_type != Int)
			throw StateError("");
		m_intArg = intVal;
	}

	void ArgumentValue::setDouble(double doubleVal)
	{
		if (m_type != Double)
			throw StateError("");
		m_doubleArg = doubleVal;
	}

	void ArgumentValue::setString( std::string stringVal )
	{
		if (m_type != String)
			throw StateError("");
		m_stringArg = stringVal;
	}

	bool ArgumentValue::isValid() const
	{
		if (m_type == Undefined)
			return false;

		if (m_type == Bool || m_type == Int || m_type == String)
			return true;

		if (m_type == Double)
		{
			return m_doubleArg == m_doubleArg;	// NaN test
		}

		if (m_repriseBase == 0)
			return false;

		switch(m_type)
		{
		case RepriseAny: return m_repriseBase != 0;
		case StmtAny:	return m_anyStmt != 0;
		case StmtBlock: return m_blockStmt != 0;
		case StmtFor:	return m_forStmt != 0;
		case StmtIf:	return m_ifStmt != 0;
		case StmtAssign:return m_assignStmt != 0;
		case StmtExpr:  return m_exprStmt != 0;
		case StmtWhile:return m_whileStmt != 0;
		case ExprVar:	return m_referenceExpr != 0;
		case ExprAny:	return m_anyExpr != 0;
		case ExprCall:	return m_callExpr != 0;
		case VarDeclaration: return m_variableDeclaration != 0;
		case TransUnit: return m_translationUnit != 0;
		OPS_DEFAULT_CASE_LABEL;
		}
		return false;
	}

	ArgumentInfo::ArgumentInfo(ArgumentValue::Type _type, const std::string& _desc)
		:description(_desc)
		,type(_type)
		,defaultValue(_type)
	{
		if (description.empty())
			description = getDefaultDescription(type);
	}

	bool ArgumentInfo::operator ==(const ArgumentInfo& other) const
	{
		return description == other.description &&
				type == other.type;
	}

	std::string ArgumentInfo::getDefaultDescription(ArgumentValue::Type type)
	{
		switch(type)
		{
		case ArgumentValue::RepriseAny: return _TL("Any node","Любой узел");
		case ArgumentValue::StmtAny:	return _TL("Operator","Оператор");
		case ArgumentValue::StmtBlock:	return _TL("Block","Блок");
		case ArgumentValue::StmtFor:	return _TL("For loop","Цикл for");
		case ArgumentValue::StmtIf:		return _TL("If operator","Оператор if");
		case ArgumentValue::StmtAssign:	return _TL("Assignment operator","Оператор присваивания");
		case ArgumentValue::StmtExpr:	return _TL("Expression operator","Оператор-выражение");
		case ArgumentValue::StmtSwitch:	return _TL("Switch operator","Оператор switch");
		case ArgumentValue::StmtWhile:	return _TL("While loop","Цикл while");
		case ArgumentValue::ExprAny:	return _TL("Expression","Выражение");
		case ArgumentValue::ExprCall:	return _TL("Function call","Вызов функции");
		case ArgumentValue::ExprVar:	return _TL("Variable occurrence", "Вхождение переменной");
		case ArgumentValue::Int:		return _TL("Integer parameter", "Целочисленный параметр");
		case ArgumentValue::Bool:		return _TL("Boolean parameter", "Булев параметр");
		case ArgumentValue::Double:		return _TL("Real parameter","Вещественный параметр");
		case ArgumentValue::String:     return _TL("String parameter","Строковый параметр");
		OPS_DEFAULT_CASE_LABEL;
		}
		return "";
	}

	bool ArgumentInfo::validate(const ArgumentValue& value) const
	{
		if (type != value.getType())
			return false;

		if (!value.isValid() && mandatory)
			return false;

		if (type == ArgumentValue::Int && !valueList.empty()) {
			if (value.getAsInt() >= int(valueList.size()))
				return false;
		}
		return true;
	}

	void ArgumentInfo::updateDefaultValue(const ArgumentValue& value)
	{
		// update only nonIR parameters
		if (validate(value) && !isRepriseArgument())
			defaultValue = value;
	}

	bool ArgumentInfo::isRepriseArgument() const
	{
		return type <= ArgumentValue::LastRepriseType;
	}

	ArgumentsInfo::ArgumentsInfo(const ArgumentInfo& paramInfo)
	{
		push_back(paramInfo);
	}

	void ArgumentsInfo::addListArg(
		const std::string& description, 
		const std::string& choise1, 
		const std::string& choise2, 
		const std::string& choise3,
		const std::string& choise4
		)
	{
		ArgumentInfo	paramInfo(ArgumentValue::Int, description);
		paramInfo.valueList.push_back(choise1);
		if (!choise2.empty())
			paramInfo.valueList.push_back(choise2);
		if (!choise3.empty())
			paramInfo.valueList.push_back(choise3);
		if (!choise4.empty())
			paramInfo.valueList.push_back(choise4);

		this->push_back(paramInfo);
	}

	bool ArgumentsInfo::validate(const ArgumentValues& values) const
	{
		if (this->size() != values.size())
			return false;

		for(size_t i = 0; i < this->size(); ++i) {
			if (!at(i).validate(values[i]))
				return false;
		}
		return true;
	}

	void ArgumentsInfo::updateDefaultValues(const ArgumentValues& values)
	{
		OPSAssert(this->size() == values.size(), "Ivalid parameter list size");

		for(size_t i = 0; i < this->size(); ++i) {
			at(i).updateDefaultValue(values[i]);
		}
	}

	/// Осуществляет трансляцию параметров одной программы в параметры другой
	bool translateArguments(const OPS::Reprise::ProgramUnit& original, OPS::Reprise::ProgramUnit& clone, ArgumentValues& vals)
	{
		// словарь для отображения параметров
		OPS::Shared::RepriseReferenceMap refMap;

		// заполняем словарь параметрами которые нужно найти
		for(size_t i = 0; i < vals.size(); ++i )
		{
			if (vals[i].isRepriseArgument())
			{
				refMap[vals[i].getAsRepriseObject()] = 0;
			}
		}

		OPS::Shared::mapReferences(original, clone, refMap);

		bool result = true;
		// меняем оригинальные значения параметров на новые
		for(size_t i = 0; i < vals.size(); ++i )
		{
			if (vals[i].isRepriseArgument())
			{
				vals[i].setReprise( refMap[vals[i].getAsRepriseObject()] );
				result &= (vals[i].getAsRepriseObject() != 0);
			}
		}

		return result;
	}

}
}
