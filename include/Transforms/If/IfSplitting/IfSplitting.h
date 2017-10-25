#ifndef OPS_TRANSFORMATIOINS_IF_IFSPLITTING_H_
#define OPS_TRANSFORMATIOINS_IF_IFSPLITTING_H_

#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Reprise/Reprise.h"

/*
	Transformation IfSplitting. Simplifyes the condition of input conditional operator

	Input parameter: conditional operator which need to be splitted
	
	Sceme of working:
	
		Case '&&':
		
			Input:
			if(Expr1 && Expr2)
				ThenBody;
			else
				ElseBody;

			Output:
			if(Expr1)
			{
				if(Expr2)
					ThenBody;
				else
					ElseBody;
			}
			else
				ElseBody;

	Cases '||' and '!' described in documentation
*/

namespace OPS
{
namespace Transforms
{
	class IfSplitting : public OPS::TransformationsHub::TransformBase
	{
	public:
		class IfSplittingException: public OPS::Exception
		{
		public:
			IfSplittingException(std::string message): OPS::Exception(message) {};
		};

		IfSplitting();

        virtual bool isApplicable(Reprise::ProgramUnit *program, const OPS::TransformationsHub::ArgumentValues &args, std::string *message);
        virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);
	};
}
}
#endif						// OPS_TRANSFORMATIOINS_IF_IFSPLITTING_H_
