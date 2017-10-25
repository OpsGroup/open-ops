#include "Transforms/Loops/LoopHeaderRemoval/LoopHeaderRemoval.h"
#include "Analysis/DepGraph/DepGraph.h"
#include "Shared/ExpressionHelpers.h"



namespace OPS
{
namespace Transforms
{
namespace Loops
{

	using namespace OPS::Shared;
	using namespace OPS::Reprise;
	using namespace OPS::Reprise::Editing;
	using namespace OPS::Shared::ExpressionHelpers;
	using namespace DepGraph;
	

	bool subExpr(BlockStatement * body, ExpressionBase * pExpr, ReferenceExpression * pSource)
{
	
	GetTypeVisitor visitor;
	pExpr->accept(visitor);

	switch(visitor.m_typeOfNode)
	{
	case GetTypeVisitor::NK_BasicLiteralExpression:

		return 0;
		break;
	case GetTypeVisitor::NK_StrictLiteralExpression:

		return 0;
		break;


	case GetTypeVisitor::NK_BasicCallExpression:
		{
			BasicCallExpression * ptemp = dynamic_cast<BasicCallExpression*>(pExpr);
			if ( ptemp )
			{
				
				if(ptemp->getArgumentCount()==2)
					return( subExpr(body, &ptemp->getArgument(0), pSource)  || 
						subExpr(body, &ptemp->getArgument(1), pSource)  
					) ;
				if(ptemp->getArgumentCount()==1)
					return subExpr(body, &ptemp->getArgument(0), pSource);
			};
		};
		break;
	case GetTypeVisitor::NK_ReferenceExpression:
		{
			const ReferenceExpression *ptemp = dynamic_cast<const ReferenceExpression*>(pExpr) ;
			if (ptemp)
			{
				if(ptemp->isEqual(*pSource)){
				return 1;
				}

				for(BlockStatement::Iterator i = body->getFirst();i.isValid();++i)
				{
					if(i->is_a<ExpressionStatement>())
					{
						if(i->cast_to<ExpressionStatement>().get().is_a<BasicCallExpression>())
						{
							if(i->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getKind()==BasicCallExpression::BCK_ASSIGN)
							{

								if(i->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0).isEqual(*ptemp))
								{
									if(subExpr(body,&(i->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(1)),pSource))
									{
									 return 1;
									}
								}
							}
						}
					}


				}

				return 0;


			}
		};
		break;
	OPS_DEFAULT_CASE_LABEL
		
	};
	return 0;
	
}





	int getNumberOfIterations(ForStatement& forStatement){
		const BasicCallExpression& init= forStatement.getInitExpression().cast_to<BasicCallExpression>();
		const BasicCallExpression& fin = forStatement.getFinalExpression().cast_to<BasicCallExpression>();
		const BasicCallExpression& step = forStatement.getStepExpression().cast_to<BasicCallExpression>();
		
		StrictLiteralExpression initNumber = init.getArgument(1).cast_to<StrictLiteralExpression>();
		StrictLiteralExpression finNumber = fin.getArgument(1).cast_to<StrictLiteralExpression>();
		StrictLiteralExpression stepNumber = step.getArgument(1).cast_to<BasicCallExpression>().getArgument(1).cast_to<StrictLiteralExpression>();
		

	
		
		double a = initNumber.getInt32();
		double b = finNumber.getInt32();
		double c = stepNumber.getInt32();
		 int result = floor((b-a)/c);
		 if(result*c+a==b && fin.getKind()==BasicCallExpression::BCK_LESS){
		result-=1;
		}
		 result++;
		return result;
	}



	double getFinalExpression(ForStatement& forStatement){

		const BasicCallExpression& init= forStatement.getInitExpression().cast_to<BasicCallExpression>();
		const BasicCallExpression& fin = forStatement.getFinalExpression().cast_to<BasicCallExpression>();
		const BasicCallExpression& step = forStatement.getStepExpression().cast_to<BasicCallExpression>();
		
		StrictLiteralExpression initNumber = init.getArgument(1).cast_to<StrictLiteralExpression>();
		StrictLiteralExpression finNumber = fin.getArgument(1).cast_to<StrictLiteralExpression>();
		StrictLiteralExpression stepNumber = step.getArgument(1).cast_to<BasicCallExpression>().getArgument(1).cast_to<StrictLiteralExpression>();
		
		
		
		double a = initNumber.getInt32();
		double b = finNumber.getInt32();
		double c = stepNumber.getInt32();
		double final =  (c*floor((b-a)/c)+a);

		if(final==b && fin.getKind()==BasicCallExpression::BCK_LESS){
		final-=c;
		}

		return final;
	
	}




	bool canApplyLoopHeaderRemovalTo(ForStatement& forStatement)
	{


		BlockStatement* loopBody = &(forStatement.getBody());

		for(BlockStatement::Iterator i = loopBody->getFirst();i.isValid();++i)
		{

			if(i->is_a<GotoStatement>())
			{
			return 0;
			}
		}

		const ExpressionBase& init=forStatement.getInitExpression();
		const ExpressionBase& fin=forStatement.getFinalExpression();
		const ExpressionBase& step=forStatement.getStepExpression();
	
		
		
		if(!init.is_a<BasicCallExpression>() || !fin.is_a<BasicCallExpression>() || !step.is_a<BasicCallExpression>())
		{
			return 0;
		}

		const BasicCallExpression& initExp = init.cast_to<BasicCallExpression>();
		const BasicCallExpression& finExp = fin.cast_to<BasicCallExpression>();
		const BasicCallExpression& stepExp = step.cast_to<BasicCallExpression>();


		if(initExp.getKind()!=BasicCallExpression::BCK_ASSIGN)
		{

			return 0;
		}
		if(finExp.getKind()!=BasicCallExpression::BCK_LESS && finExp.getKind()!=BasicCallExpression::BCK_LESS_EQUAL)
		{

			return 0;
		}
		if(stepExp.getKind()!=BasicCallExpression::BCK_ASSIGN)
		{

			return 0;
		}
		if(!initExp.getArgument(0).is_a<ReferenceExpression>() ||  !initExp.getArgument(1).is_a<LiteralExpression>())
		{
			return 0;
		}
		if(!stepExp.getArgument(0).is_a<ReferenceExpression>() ||  !stepExp.getArgument(1).is_a<BasicCallExpression>())
		{
			

			return 0;

			
		}
		if(stepExp.getArgument(1).cast_to<BasicCallExpression>().getKind()!=BasicCallExpression::BCK_BINARY_PLUS || !stepExp.getArgument(1).cast_to<BasicCallExpression>().getArgument(0).is_a<ReferenceExpression>())
		{
			return 0;
		}
		

		if(!finExp.getArgument(0).is_a<ReferenceExpression>() ||  !finExp.getArgument(1).is_a<LiteralExpression>())
		{
			return 0;
		}

		if(!finExp.getArgument(0).isEqual(stepExp.getArgument(0)) || !initExp.getArgument(0).isEqual(stepExp.getArgument(0)) || !stepExp.getArgument(1).cast_to<BasicCallExpression>().getArgument(0).isEqual(stepExp.getArgument(0)))
		{
			return 0;
		}

			
		if(getNumberOfIterations(forStatement)==1){
		return 1;
		}
		if(getNumberOfIterations(forStatement)<=0){
		return 0;
		}







DepGraph::LamportGraph depGraph;
depGraph.Build(loopBody);
	
for(DepGraph::LampArrowIterator i=depGraph.Begin();i!=depGraph.End();++i)
{
	
	
	if((*i)->type==DepGraph::FLOWDEP)
	{
		if( !(*i)->IsLoopIndependentOnly())
		{
				
		return 0;
		}

		

	}
}

if(loopBody->isEmpty()){
return(1);
}


ReferenceExpression ind = initExp.getArgument(0).cast_to<ReferenceExpression>();

		
for(BlockStatement::Iterator i = loopBody->getFirst();i.isValid();++i)
{


	

	if(i->is_a<ExpressionStatement>())
	{
		if(i->cast_to<ExpressionStatement>().get().is_a<BasicCallExpression>()){
			if(subExpr(loopBody,&(i->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0)),&ind))
			{

				return 0;
			}
		}

	}
	
	
	if(i->is_a<WhileStatement>())
	{
		if(subExpr(loopBody,&(i->cast_to<WhileStatement>().getCondition()),&ind))
		{
	
				return 0;
		}
			
	}
	if(i->is_a<IfStatement>())
	{
		if(subExpr(loopBody,&(i->cast_to<IfStatement>().getCondition()),&ind))
		{
	
				return 0;
		}
			
	}


	if(i->is_a<ForStatement>())
	{
		if(subExpr(loopBody,&(i->cast_to<ForStatement>().getInitExpression()),&ind))
		{
	
				return 0;
		}

		if(subExpr(loopBody,&(i->cast_to<ForStatement>().getFinalExpression()),&ind))
		{
	
				return 0;
		}

		if(subExpr(loopBody,&(i->cast_to<ForStatement>().getStepExpression()),&ind))
		{
	
				return 0;
		}
			
	}

}
	
		

return 1;


}
	void makeLoopHeaderRemoval(ForStatement& forStatement)
	{


		
		BlockStatement* loopBody = &forStatement.getBody();
		const BasicCallExpression& init= forStatement.getInitExpression().cast_to<BasicCallExpression>();
		
		StrictLiteralExpression* fn = new StrictLiteralExpression();
		fn->setInt32(getFinalExpression(forStatement));

		
		loopBody->addFirst(new ExpressionStatement(&(op(init.getArgument(0)) R_AS op(fn))));
		
		replaceStatement(forStatement, ReprisePtr<StatementBase>(loopBody));
		
		
	}

} //Loops
} //Transforms
} //OPS
