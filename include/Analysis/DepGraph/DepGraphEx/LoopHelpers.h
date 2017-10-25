#ifndef _LOOPHELPERS_H_
#define _LOOPHELPERS_H_

#include "Reprise/Reprise.h"
#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/DepGraph/DepGraphEx/DepGraphEx.h"
#include <vector>
#include <map>

//#include <string>

namespace DepGraph
{

void WriteLog(const std::string& message, OPS::Console::MessageLevel level = OPS::Console::LEVEL_DEBUG);

typedef OPS::Shared::CanonicalLinearExpression LinearExpr;

/// Создать условие типа "остаток от деления"
AutoExprNode makeMODCondition(const LinearExpr& expr, int divisor);
/// Создать условие типа "меньше либо равно"
AutoExprNode makeLECondition(const LinearExpr& expr, int right);
/// Создать условие типа "больше либо равно"
AutoExprNode makeGECondition(const LinearExpr& expr, int right);

/// Описание цикла
/// УСТАРЕВШИЙ КЛАСС - НЕ ИСПОЛЬЗОВАТЬ!
class LoopDescr
{
	/// Валидны ли границы цикла
	bool		m_bValidBounds;
	/// Верхняя граница цикла
	LinearExpr  m_cUpBound;
	/// Нижняя граница цикла
	LinearExpr  m_cDwBound;
	/// Шаг цикла
	LinearExpr  m_cStep;
public:
	LoopDescr();
	LoopDescr(const Id::LoopInfo& el);

	const LinearExpr& getUpBound() const
	{
		if( m_bValidBounds )
			return m_cUpBound;
		else
			throw OPS::Exception("Invalid loop bounds");
	}

	const LinearExpr& getDwBound() const
	{
		if( m_bValidBounds )
			return m_cDwBound;
		else
			throw OPS::Exception("Invalid loop bounds");
	}

	const LinearExpr& getStep() const
	{
		if( m_bValidBounds )
			return m_cStep;
		else
			throw OPS::Exception("Invalid loop bounds");
	}

	OPS::Reprise::VariableDeclaration*	counterIter;
	OPS::Reprise::ForStatement*	m_pFor;
};

}

#endif
