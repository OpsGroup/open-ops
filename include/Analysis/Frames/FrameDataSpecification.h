#ifndef FRAMEDATASPECIFICATION_H
#define FRAMEDATASPECIFICATION_H

#include <list>
#include <map>
#include <vector>

#include "Reprise/Reprise.h"
#include "Analysis/ComplexOccurrenceAnalysis/GrouppedOccurrences.h"
#include "Shared/ExpressionHelpers.h"

namespace OPS
{

namespace Analysis
{

class DataRange
{
public:
	virtual unsigned DataAmount() = 0;

	virtual std::ostream& operator<<( std::ostream& out ) = 0;
};

/**
*	Описание диапазонов данных через прямоугольники
*/
class RectangleDataRange: public DataRange
{

public:
	unsigned DataAmount()
	{
		return 0;
	}
};

/**
*	Описание диапазонов данных через ленты
*/
class BeltDataRange: public DataRange
{

public:
	unsigned DataAmount()
	{
		return 0;
	}
};

class DataSpecificationAnalysis
{
public:
	typedef std::vector< std::pair<Reprise::ReprisePtr<Reprise::ExpressionBase>, Reprise::ReprisePtr<Reprise::ExpressionBase> > > Range;
	typedef std::multimap<Reprise::VariableDeclaration*, Range> VariableToRangeMap;

	/**
	Функция, формирующая описание данных во фрагменте.
	occurences - все вхождения фрагмента fragment
	fragment - анализируемый фрагмент
	*/
	void specifyDataSubsets(OccurrencesByDeclarations& occurences,
		const Reprise::ProgramFragment& fragment,
		bool removeLocalVarsFlag = false,
		std::map<const Reprise::ForStatement*, Range> *lengths = 0);

	/**
	Вычисление объема данных в октетах.
	*/
	unsigned calculateDataAmount() const;

	/**
	Функция возвращает информацию о диапазонах данных.
	*/
	VariableToRangeMap getRanges() const
	{
		return m_dataRanges;
	}

	/**
	Функция возвращает информацию об объединенных диапазонах данных.
	*/
	VariableToRangeMap getUnitedRanges() const
	{
		return m_unitedDataRanges;
	}

	/// Объединение диапазонов
	static VariableToRangeMap uniteDataRanges(VariableToRangeMap &Ranges);
private:
	VariableToRangeMap detectLoopCountersAndBorders(const Reprise::ProgramFragment& fragment,
		const Reprise::ExpressionBase* Node,
		std::map<const Reprise::ForStatement*, Range> *lengths);

	Range specifyLoopDataSubset(VariableToRangeMap loopCounterBorders,
		const Reprise::BasicCallExpression& Node);

	/**

	*/
	Range specifySingleDataSubset(VariableToRangeMap loopCounterBorders,const Reprise::ExpressionBase* occurenceExpr);

	static bool checkIntersectTheRanges( const Range &a, const Range &b );
	static Range uniteRange(const Range& a, const Range& b);
	///	Удаляет информацию о локальных переменных.
	void removeLocalVariables(const Reprise::ProgramFragment& fragment);

	VariableToRangeMap m_dataRanges;
	VariableToRangeMap m_unitedDataRanges;
};

}

}

#endif // FRAMEDATASPECIFICATION_H
