#ifndef _SSA_SUBSTITUTION_FORWARD_H_INCLUDED_
#define _SSA_SUBSTITUTION_FORWARD_H_INCLUDED_

#include "Reprise/Reprise.h"
#include "Analysis/SSAForm/SSAForm.h"


namespace OPS
{
	namespace Transforms
	{
		namespace Scalar
		{


			using OPS::Reprise::ExpressionBase;
			using OPS::Reprise::BasicCallExpression;
			using OPS::Analysis::SSAForms::SSAForm;



			
			/***************************************************************** 
			на вход подаётся SSA-форма, генератор и выражение, в которое следует подставить правую часть этого генератора
			Если переменная, имеющая вхождение в правой части гернератора, к моменту подстановки успевает изменить значение,
			создается её копия рядом с генератором 
			******************************************************************/
			void makeSSASubstitutionForward(SSAForm& ssa, BasicCallExpression& generator, ExpressionBase& expressionTo);

		} // Scalar
	} // Transforms
} // OPS

#endif	// _SSA_SUBSTITUTION_FORWARD_H_INCLUDED_
