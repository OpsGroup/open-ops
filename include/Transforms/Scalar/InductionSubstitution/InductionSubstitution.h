#ifndef _INDUCTION_SUBSTITION_H_INCLUDED_
#define _INDUCTION_SUBSTITION_H_INCLUDED_

#include "Analysis/InductionVariables/InductionVariables.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

	using OPS::Analysis::LoopInductionAnalysis;
	using OPS::Reprise::ReprisePtr;
	using OPS::Reprise::ExpressionBase;

	//данные для восстановления после замены переменных
	typedef std::map<OPS::Reprise::ExpressionBase*, OPS::Reprise::ReprisePtr<OPS::Reprise::ExpressionBase> > ReplaceMap;

	//если есть эталонная переменная, подставить её во все вхождения индуктивных переменных
	//возвращает данные для восстановления
ReplaceMap substituteAllInductionVariables(LoopInductionAnalysis& inductions);

	//отменить подстановку эталонной переменной
	//если подставленные узлы ВП уже кто-то удалил, возвращает false
void undoInductionSubstitution(ReplaceMap& undoData);



} // Scalar
} // Transforms
} // OPS

#endif	// _INDUCTION_SUBSTITION_H_INCLUDED_
