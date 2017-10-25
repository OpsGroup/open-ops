#ifndef OPS_TRANSFORMATIOINS_IF_IFEXTRACTION_H_
#define OPS_TRANSFORMATIOINS_IF_IFEXTRACTION_H_

#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Reprise/Reprise.h"

/*
	Transformation IfExtraction. Extract and moves out conditional statement from for statement
	
	Input parameters: 
		1) conditional statement that will be moved out
		2) loop statement that will be partitioned by 1)			
	
	Restrictions: 
		1. 2) must contain 1)
		2. 2) must be canonized
		3. Condition of 1) must be independent of 2). It means:
			3.1 Condition of 1) independent from loop counter of 2)
			3.2 Condition of 1) doesn't contain variables declared in 2) and it's childs
			3.3 2) doesn't contains goto, break, continue statements
			3.4 2) doesn't contains subroutine calls
			3.5 Condition of 1) has no side effect

	Sceme of working:
		
		Input:
			for(i = 0; i < N; i = i + 1)
			{
				Part1;
				if(Condition)
					ThenBlock;
				else
					ElseBlock;
				Part2;
			}

		Output:
			if(Condition)
				for(i = 0; i < N; i = i + 1)
				{
					Part1;
					ThenBlock;
					Part2;
				}
			else
				for(i = 0; i < N; i = i + 1)
				{
					Part1;
					ElseBlock;
					Part2;
				}

		Other cases described in documentation
*/

namespace OPS
{
namespace Transforms
{
	class IfExtraction : public OPS::TransformationsHub::TransformBase
	{
	public:
		class IfExtractionException: public OPS::Exception
		{
		public:
			IfExtractionException(std::string message): OPS::Exception(message) {};
		};

	public:
		IfExtraction();

        bool isApplicable(Reprise::ProgramUnit *program, const OPS::TransformationsHub::ArgumentValues &args, std::string *message);
        virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* pProgram, const OPS::TransformationsHub::ArgumentValues& params);

		void transformNotDependentIfStatement();
	private:
		bool checkForStatement();
		bool checkIfInForStatement();
		bool checkIfStatement();

	private:
		OPS::Reprise::IfStatement* m_pIfStatement;
		OPS::Reprise::ForStatement* m_pForStatement;
		OPS::Reprise::VariableDeclaration* m_pLoopCounter;
	};
}
}

#endif // OPS_TRANSFORMATIOINS_IF_IFEXTRACTION_H_
