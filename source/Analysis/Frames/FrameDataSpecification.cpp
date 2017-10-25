#include <set>
#include <iostream>

#include "Analysis/Frames/FrameDataSpecification.h"
#include "Backends/OutToC/OutToC.h"
#include "Frontend/Frontend.h"
#include "FrontTransforms/ExpressionSimplifier.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/LinearExpressions.h"
#include "Shared/LoopShared.h"
#include "Shared/StatementsShared.h"

using namespace std;

using namespace OPS::ExpressionSimplifier;
using namespace OPS::Frontend;
using namespace OPS::Reprise;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Shared;

namespace OPS
{
namespace Analysis
{

/* ostream& RectangleDataRange::operator<<( ostream& out )
{
	if ( this->size() == 0)
		out << "(scalar)";
	else
	{
		out << "(vector)";
		for (int i = 0; i < this->size(); ++i)
		{
			out << "[" << r[i].first->dumpState() << ":" << r[i].second->dumpState() << "]";
		}
	}
}*/

// -------------------------------
// class DataSpecificationAnalysis
// -------------------------------

typedef DataSpecificationAnalysis::Range Range;
typedef DataSpecificationAnalysis::VariableToRangeMap VariableToRangeMap;


VariableToRangeMap DataSpecificationAnalysis::detectLoopCountersAndBorders(const ProgramFragment& fragment,
																		   const ExpressionBase* Node,
																		   map<const Reprise::ForStatement*, Range> *lengths)
{
	VariableToRangeMap res;
	list<const ForStatement*> loops = getEmbracedLoopsNest(*Node, &fragment.getStatementsBlock());
	list<const ForStatement*>::const_iterator it = loops.begin();

	for(; it != loops.end(); ++it)
	{
		Range resR(1);
		VariableDeclaration* vd = 0;
		const ForStatement* forStmt = *it;

		if (const BasicCallExpression* stepExpr = forStmt->getStepExpression().cast_ptr<BasicCallExpression>())
		{
			switch (stepExpr->getKind())
			{
			case BasicCallExpression::BCK_ASSIGN:
				{
					if (stepExpr->getArgument(0).is_a<ReferenceExpression>())
					{
						vd = const_cast<VariableDeclaration*>( &(stepExpr->getArgument(0).cast_to<ReferenceExpression>().getReference()) );
					}
					else
					{
						OPS::RuntimeError("Unexamined StepExpression: it's not a ReferenceExpression");
					}
					break;
				}
			default:
				OPS::RuntimeError("Unexamined StepExpression");
			}
		}

		// Если нам дали уже готовый диапазон для этого цикла, то просто подставляем его.
		if ( lengths && lengths->find(forStmt) != lengths->end() )
		{
			std::map<const Reprise::ForStatement*, Range>::iterator readyRange = lengths->find(forStmt);
			resR = readyRange->second;
			res.insert( make_pair(vd, resR) );
			continue;
		}

		if (const BasicCallExpression* initExp = forStmt->getInitExpression().cast_ptr<BasicCallExpression>())
		{
			switch( initExp->getKind())
			{
			case BasicCallExpression::BCK_ASSIGN:
				{
					resR[0].first.reset(initExp->getArgument(1).clone());
					break;
				}
			default:
				OPS::RuntimeError("Unexamined InitExpression");
			}
		}
		if (const BasicCallExpression* finalExp = forStmt->getFinalExpression().cast_ptr<BasicCallExpression>())
		{
			switch( finalExp->getKind())
			{
			case BasicCallExpression::BCK_LESS:
				{
					IntegerHelper ih(BasicType::BT_INT32);
					resR[0].second.reset(&(op(finalExp->getArgument(1)) - ih(1)));
					break;
				}
			case BasicCallExpression::BCK_GREATER:
				{
					IntegerHelper ih(BasicType::BT_INT32);
					resR[0].second = resR[0].first;
					resR[0].first.reset(&(op(finalExp->getArgument(1)) + (ih(1))));
					break;
				}
			default:
				throw OPS::RuntimeError("Unexamined FinalExpression");
			}
		}
		res.insert( make_pair(vd, resR) );
	}
	return res;
}


Range DataSpecificationAnalysis::specifyLoopDataSubset(VariableToRangeMap loopCounterBorders,
														const Reprise::BasicCallExpression& Node)
{
	IntegerHelper ih(BasicType::BT_INT32);

	int arrayDimensionsCount = Node.getArgumentCount() -1;
	Range resultDataRange(arrayDimensionsCount);

	for ( int dim=0; dim < arrayDimensionsCount; ++dim )
	{
		unique_ptr<ParametricLinearExpression> currentDimensionExpression(ParametricLinearExpression::
			createByIndexes(const_cast<ExpressionBase*>( &Node.getArgument(dim+1) ) ));
		if (!currentDimensionExpression.get())
			throw OPS::RuntimeError("Can't create ParametricLinearExpression");

		BasicCallExpression* indL = 0;
		BasicCallExpression* indR = 0;

		for ( VariableToRangeMap::iterator counterRange = loopCounterBorders.begin(); counterRange != loopCounterBorders.end(); ++counterRange )
		{
			ReprisePtr<ExpressionBase> currentDimensionIndexCoef = currentDimensionExpression->getCoefficient(counterRange->first);

			if ( currentDimensionIndexCoef->cast_to<StrictLiteralExpression>().getInt32() > 0 )
			{
				indL = &( op(*currentDimensionIndexCoef) * op(counterRange->second[0].first) + opnc(indL) );
				indR = &( op(*currentDimensionIndexCoef) * op(counterRange->second[0].second) + opnc(indR) );
			}
			else if ( currentDimensionIndexCoef->cast_to<StrictLiteralExpression>().getInt32() < 0 )
			{
				indL = &( op(*currentDimensionIndexCoef) * op(counterRange->second[0].second) + opnc(indL) );
				indR = &( op(*currentDimensionIndexCoef) * op(counterRange->second[0].first) + opnc(indR) );
			}
		}

		ReprisePtr<BasicCallExpression> resExpL(&(opnc(indL) + op(currentDimensionExpression->getFreeCoefficient())));
		ReprisePtr<BasicCallExpression> resExpR(&(opnc(indR) + op(currentDimensionExpression->getFreeCoefficient())));

		resultDataRange[dim].first = ParametricLinearExpression::simplify(resExpL.get());
		resultDataRange[dim].second = ParametricLinearExpression::simplify(resExpR.get());
	}

	return resultDataRange;
}

Range DataSpecificationAnalysis::specifySingleDataSubset(VariableToRangeMap loopCounterBorders,const Reprise::ExpressionBase* occurenceExpr)
{
	Range resultDataRange;

	if (occurenceExpr->is_a<BasicCallExpression>())
	{
		const BasicCallExpression& Node = occurenceExpr->cast_to<BasicCallExpression>();

		if (Node.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
		{
			if ( !loopCounterBorders.empty())
			{
				/*VariableToRangeMap::iterator itr = loopCounterBorders.end();
				--itr;
				Range loopBorder = itr->second;*/
				resultDataRange = specifyLoopDataSubset(loopCounterBorders, Node);
			}
			else
			{
				for ( int i=1; i < Node.getArgumentCount(); ++i )
				{
					ExpressionBase* resRange = const_cast<ExpressionBase*>( &Node.getArgument(i) );
					ReprisePtr<ExpressionBase> pDrange = ParametricLinearExpression::simplify(resRange);
					resultDataRange.push_back( make_pair( pDrange, pDrange) );
				}
			}
		}
		else
			throw OPS::RuntimeError("Occurence isn't array or scalar");
	}
	else
		if (occurenceExpr->is_a<ReferenceExpression>())
		{
			/*IntegerHelper ih(BasicType::BT_INT32);
			resultDataRange.push_back( make_pair( ReprisePtr<ExpressionBase>(&(ih(0))),
												  ReprisePtr<ExpressionBase>(&(ih(0))) ) );*/
			//resultDataRange.push_back( make_pair( &(ih(0)), &(ih(0)) ) );
		}
		else
			throw OPS::RuntimeError("Occurence isn't array or scalar");
	return resultDataRange;
}

void DataSpecificationAnalysis::specifyDataSubsets( OccurrencesByDeclarations& occurences,
													const Reprise::ProgramFragment& fragment,
													bool removeLocalVarsFlag,
													std::map<const Reprise::ForStatement*, Range> *lengths)
{
	// Собираем информацию о диапазонах данных
	OccurrencesByDeclarations::iterator itOc;
	for ( itOc = occurences.begin(); itOc != occurences.end(); ++itOc)
	{
		for(TopLevelOccurrenceList::const_iterator itLE = (itOc->second).begin(); itLE != (itOc->second).end(); ++itLE)
		{
			VariableToRangeMap loopBordersAndCounter = detectLoopCountersAndBorders(fragment,*itLE, lengths);
			m_dataRanges.insert(make_pair(const_cast<VariableDeclaration*>(itOc->first), specifySingleDataSubset(loopBordersAndCounter,*itLE)));
		}
	}
	// Удаление локальных переменных
	if (removeLocalVarsFlag)
		removeLocalVariables(fragment);

	// Пытаемся объединить диапазоны
	m_unitedDataRanges = uniteDataRanges(m_dataRanges);
}

void DataSpecificationAnalysis::removeLocalVariables(const ProgramFragment& fragment)
{
	VariableToRangeMap::iterator it = m_dataRanges.begin();
	for(;it != m_dataRanges.end();)
		if (it->first->hasDefinedBlock() &&
			Shared::contain(&fragment.getStatementsBlock(), &it->first->getDefinedBlock()))
		{
			VariableToRangeMap::iterator tmp = it;
			++it;
			m_dataRanges.erase(tmp);
		}
		else
			++it;
}

unsigned DataSpecificationAnalysis::calculateDataAmount() const
{
	IntegerHelper ih(BasicType::BT_INT32);
	unsigned result = 0;
	VariableToRangeMap::const_iterator it = m_unitedDataRanges.begin();
	for(; it != m_unitedDataRanges.end(); ++it)
	{
		unsigned sizeOfSingleUnit(0), amount(1);
		const TypeBase& type = it->first->getType();

		// Scalar
		if (type.is_a<BasicTypeBase>())
			sizeOfSingleUnit = type.cast_to<BasicTypeBase>().getSizeOf();

		// Array
		if (type.is_a<ArrayType>())
		{
			const TypeBase* elementType = &(type.cast_to<ArrayType>().getBaseType());
			while (elementType->is_a<ArrayType>())
				elementType = &(elementType->cast_to<ArrayType>().getBaseType());

			if (elementType->is_a<BasicTypeBase>())
				sizeOfSingleUnit = elementType->cast_to<BasicTypeBase>().getSizeOf();
			else
				throw RuntimeError("Cann't calculate amount of data for variable " + it->first->dumpState() + "!");

			ReprisePtr<ExpressionBase> numberOfElements( &ih(1) );


			// Считаем объем диапазона
			for ( size_t dim=0; dim < it->second.size(); ++dim )
			{
				// # Не очень хорошо, что тут необходимо вызывать конструктор вот так. Хочется иметь реализацию присваивания тут.
				numberOfElements = ParametricLinearExpression::simplify(
					&( op(numberOfElements)*(op(it->second[dim].second) - op(it->second[dim].first) + ih(1) ) ) );
			}

			if (numberOfElements->is_a<StrictLiteralExpression>())
				amount = numberOfElements->cast_to<StrictLiteralExpression>().getInt32();
			else
				throw RuntimeError("Cann't calculate amount of data for variable " + it->first->dumpState() + "!");
		}

		// Wrong type
		if (!sizeOfSingleUnit)
			throw ArgumentError("Analized (by DataSpecificationAnalysis) fragment contains ocurences of unsuitable type: " + type.dumpState() + "!");

		result += sizeOfSingleUnit * amount;
	}
	return result;
}

bool DataSpecificationAnalysis::checkIntersectTheRanges( const Range &a, const Range &b )
{
	// Скаляр сам с собою всегда пересекается.
	if ( a.size() == 0  )
		return true;

	// Проверим, что границы проекций это StrictLiteralExpression
	for ( size_t dim=0; dim < a.size(); ++dim )
		if ( !( a[dim].first->is_a<StrictLiteralExpression>() &&
				a[dim].second->is_a<StrictLiteralExpression>() &&
				b[dim].first->is_a<StrictLiteralExpression>() &&
				b[dim].second->is_a<StrictLiteralExpression>() ) )
			return false;

	// Проверяем, что два диапазона пересекаются
	// Если пересекаются проекции, то пересекаются и диапазоны,
	// ибо мы рассматриваем n-мерные параллелепипеды.
	// А еще мы предполагаем, что размерности диапазонов одной и той же матрицы равны
	for ( size_t dim=0; dim < a.size(); ++dim )
	{
		// Проверка пересечения проекций
		int al = a[dim].first->cast_to<StrictLiteralExpression>().getInt32();
		int ar = a[dim].second->cast_to<StrictLiteralExpression>().getInt32();

		int bl = b[dim].first->cast_to<StrictLiteralExpression>().getInt32();
		int br = b[dim].second->cast_to<StrictLiteralExpression>().getInt32();

		// Обозначив ( ) - диапазон a, [] - диапазон b. Опишем условие, как: ( [ ) ] or ( [ ] ) or [ ( ] ) or [ ( ) ]
		if ( al <= bl && ar+1 >= bl || bl <= al && br+1 >= al )
			return true;
	}

	return false;
}

Range DataSpecificationAnalysis::uniteRange(const Range& a, const Range& b)
{
	Range res( min(a.size(), b.size()) );
	for ( size_t dim=0; dim < min(a.size(), b.size()); ++dim )
	{
		// # В идеале, всё должно было быть коротко и красиво...
		//(*it)[dim].first.reset( min( *(*it)[dim].first, *(*cit)[dim].first ) );
		//(*it)[dim].second.reset( max( *(*it)[dim].second, *(*cit)[dim].second ) );

		// Но окуни, тобишь - косяк. Поэтому пока не разберемся и/или не починим, будет трость.
		int al = a[dim].first->cast_to<StrictLiteralExpression>().getInt32();
		int ar = a[dim].second->cast_to<StrictLiteralExpression>().getInt32();

		int bl = b[dim].first->cast_to<StrictLiteralExpression>().getInt32();
		int br = b[dim].second->cast_to<StrictLiteralExpression>().getInt32();

		res[dim].first = ReprisePtr<ExpressionBase>( al < bl ? StrictLiteralExpression::createInt32(al) : StrictLiteralExpression::createInt32(bl) );
		res[dim].second = ReprisePtr<ExpressionBase>( ar < br ? StrictLiteralExpression::createInt32(br) : StrictLiteralExpression::createInt32(ar) );
	}
	return res;
}

/*
*	# TODO: Нам необходимо хоть какое-нибудь объединение.
*	#		А, вообще говоря, нужно уметь обрамлять диапазон используемых данных минимальным параллелепипедом.
*/
DataSpecificationAnalysis::VariableToRangeMap DataSpecificationAnalysis::uniteDataRanges(VariableToRangeMap& Ranges)
{
	VariableToRangeMap res(Ranges);

	// # ToDo!: Сделать кошерное объединение.
	for (auto begin = res.begin(); begin != res.end(); begin = res.upper_bound(begin->first))
	{
		auto target = begin;
		auto end = res.upper_bound(begin->first);

		bool wasUnite = false;

		while ( target != end )
		{
			wasUnite = false;

			auto cit = target;
			if ( ++cit == end ) // Если объединять не с кем.
				break;

			while( cit != end )
			{
				// Объединяем диапазоны объединением проекций, если они пересекаются
				if ( checkIntersectTheRanges( target->second, cit->second ) )
				{
					target->second = uniteRange( target->second, cit->second);

					res.erase(cit++);
					wasUnite = true;
				}
				else
					cit++;
			}

			if ( !wasUnite )
				++target;
			else
			{
				target = begin;
				end = res.upper_bound(target->first);
			}

		}
	}
	return res;
}

} // Analysis
} // OPS
