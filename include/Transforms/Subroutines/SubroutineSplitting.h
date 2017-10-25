#ifndef OPS_SUBROUTINE_SPLITTING_H_INCLUDED__
#define OPS_SUBROUTINE_SPLITTING_H_INCLUDED__

#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Reprise/Reprise.h"
namespace OPS {
	namespace Transforms{

		namespace Subroutines {
	
			bool splitSubroutine(OPS::Reprise::SubroutineDeclaration* subroutine, OPS::Reprise::BlockStatement::Iterator place, OPS::Reprise::ProgramUnit* unit); 
	
			class SubroutineSplitting : public OPS::TransformationsHub::TransformBase
			{
			public:
				class SubroutineSplittingException: public OPS::Exception
				{
				public:
					SubroutineSplittingException(std::string message): OPS::Exception(message) {};
				};

				SubroutineSplitting();

                virtual bool isApplicable(Reprise::ProgramUnit *program, const OPS::TransformationsHub::ArgumentValues &args, std::string *message);
                virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);
			};

		}
	
	}
}

#endif
