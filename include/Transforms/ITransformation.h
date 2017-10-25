#ifndef I_TRANSFORMATION_H
#define I_TRANSFORMATION_H

#include <string>

namespace OPS
{
	namespace Transforms
	{
		class ITransformation
		{
		public:
			virtual bool analyseApplicability() = 0;

			virtual std::string getErrorMessage() = 0;

			virtual void makeTransformation() = 0;

            virtual ~ITransformation() {};
		};
	}
}

#endif  // I_TRANSFORMATION_H
