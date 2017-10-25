#ifndef OPS_TRANSFORMATIOINS_IF_IFDISTRIBUTION_H_
#define OPS_TRANSFORMATIOINS_IF_IFDISTRIBUTION_H_

#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Transforms/ITransformation.h"

#include "Reprise/Reprise.h"

/*
	Transformation IfDistribution. Divides conditional operator into 2 conditioianl operators
	
	Input parameters: 1) conditional operator which need to be distributed
	                  2) any statement which placed directly in THEN body of 1st parameter - border of distribution
					  3) boolean flag - always create a new variable
	
	Restrictions: 1) source conditional operator must have no ELSE body
	              2) source conditional operator must have no variable declarations in THEN body

	Sceme of working:
		
		Case with creating new variable:
			
			Input:
			if(ConditionExpr)
			{
				Stmt_1;
				...
				Stmt_i;
				Stmt_i+1;               // 2nd parameter
				...
				Stmt_N;
			}

			Output:
			{
				Type newVar = ConditionExpr;
				if(newVar)
				{
					Stmt_1;
					...
					Stmt_i;
				}
				if(newVar)
				{
					Stmt_i+1;
					...
					Stmt_N;
				}
			}

	Other cases described in documentation
*/
namespace OPS
{
namespace Transforms
{
	class IfDistribution : public OPS::TransformationsHub::TransformBase
	{
	public:
		class IfDistributionException: public OPS::Exception
		{
		public:
			IfDistributionException(std::string message): OPS::Exception(message) {};
		};

		IfDistribution();

        virtual bool isApplicable(Reprise::ProgramUnit *program, const OPS::TransformationsHub::ArgumentValues &args, std::string *message);
        virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);
	};

	class IfDistributionBasic: public ITransformation
	{
	public:
		enum IfDistributionBasicError
		{
			IDBE_NO = 0,

			IDBE_UNKNOWN = 1,
			IDBE_NULL_ARGUMENT,
			IDBE_BORDER_ISNT_IN_IF,
			IDBE_UNAVAILABLE_STMTS
		};

	public:
		IfDistributionBasic(OPS::Reprise::IfStatement* pIfStatement, OPS::Reprise::StatementBase* pBorderStatement, bool alwaysCreateNewVar = false);

	// Override section
		virtual void initializeCommonData();

		virtual bool analyseApplicability();

		virtual std::string getErrorMessage();

		virtual void makeTransformation();

	private:
		bool hasSideEffect();

	private:
		OPS::Reprise::IfStatement*   m_pIfStatement;
		OPS::Reprise::StatementBase* m_pBorderStatement;
		bool                         m_alwaysCreateNewVar;

		IfDistributionBasicError     m_errorCode;
	};
}
}
#endif // OPS_TRANSFORMATIOINS_IF_IFDISTRIBUTION_H_
