#ifndef _LOOP_TEST_H_
#define _LOOP_TEST_H_

#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Reprise/Reprise.h"

#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
	namespace Transforms
	{
		class LoopCycleOffset : public OPS::TransformationsHub::TransformBase
		{
		public:
			class LoopCycleOffsetException : public OPS::Exception
			{
			public:
				LoopCycleOffsetException(std::string message): OPS::Exception(message) {};
			}; // LoopTestException
			
			class Replacer : public OPS::Reprise::Service::DeepWalker
			{
			public:
				Replacer(OPS::Reprise::RepriseBase& old_expr, OPS::Reprise::RepriseBase& new_expr);
				void visit(OPS::Reprise::ReferenceExpression& reference_expr);

			private:
				OPS::Reprise::RepriseBase& m_old_expr;
				OPS::Reprise::RepriseBase& m_new_expr;
			};

			LoopCycleOffset();

			virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);

		}; // LoopTest

	} // Transforms

} // OPS

#endif  //_LOOP_TEST_H_
