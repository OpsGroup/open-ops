#ifndef INLININGWITHSUBSTITUTION_H
#define INLININGWITHSUBSTITUTION_H

#include "Transforms/Subroutines/Inlining.h"

namespace OPS
{
namespace Transforms
{
namespace Subroutines
{

	void makeInliningWithSubstitution(OPS::Reprise::SubroutineCallExpression* pCall);

	class InliningWithSubtitution : public OPS::TransformationsHub::TransformBase
	{
	public:
		InliningWithSubtitution();
        virtual void makeTransformImpl(OPS::Reprise::ProgramUnit *program, const OPS::TransformationsHub::ArgumentValues &params);
	};

}
}
}

#endif // INLININGWITHSUBSTITUTION_H
