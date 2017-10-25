#ifndef OPS_INLINING_H_INCLUDED__
#define OPS_INLINING_H_INCLUDED__
#include <list>

#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Reprise/Reprise.h"

namespace OPS {
	namespace Transforms{

		namespace Subroutines {
	
			bool fullInlining(OPS::Reprise::TranslationUnit* unit);
			bool fullInlining(OPS::Reprise::StatementBase* stmt);

			bool inlineSubstitution(OPS::Reprise::SubroutineCallExpression* pCall, std::list<Reprise::VariableDeclaration*>* pAssignmentsList = NULL);

			class Inlining : public OPS::TransformationsHub::TransformBase
			{
			public:
				OPS_DEFINE_EXCEPTION_CLASS(InliningException, OPS::Exception)

				Inlining();

                virtual bool isApplicable(Reprise::ProgramUnit *program, const OPS::TransformationsHub::ArgumentValues &args, std::string *message);
                virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);
			};

			class FullInlining : public OPS::TransformationsHub::TransformBase
			{
			public:
				OPS_DEFINE_EXCEPTION_CLASS(FullInliningException, Inlining::InliningException)

				FullInlining();

                virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);
			};
	
		}
	
	}
}

#endif
