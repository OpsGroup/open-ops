#ifndef OPS_FRAGMENT_TO_SUBROUTINE_H_INCLUDED_
#define OPS_FRAGMENT_TO_SUBROUTINE_H_INCLUDED_


#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Reprise/Reprise.h"
namespace OPS {
	namespace Transforms{

		namespace Subroutines {

            OPS::Reprise::SubroutineDeclaration* fragmentToSubroutine(Reprise::BlockStatement::Iterator first, Reprise::BlockStatement::Iterator last);

			class FragmentToSubroutine : public OPS::TransformationsHub::TransformBase
			{
			public:
				class FragmentToSubroutineException: public OPS::Exception
				{
				public:
					FragmentToSubroutineException(std::string message): OPS::Exception(message) {};
				};

				FragmentToSubroutine();

                virtual bool isApplicable(Reprise::ProgramUnit *program, const OPS::TransformationsHub::ArgumentValues &args, std::string *message);
                virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);
			};
	
		}
	
	}
}

#endif
