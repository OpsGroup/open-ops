#include "Transforms/Scalar/CommonSubexpressionElimination/CommonSubexpressionElimination.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

int makeCommonSubexpressionElimination(BlockStatement& blockStatement)
{
	AEBList AEBs;
	AEB currentAEB;
	int position	= 0;
	int result		= 0;

	for(BlockStatement::Iterator it = blockStatement.getFirst(); it != blockStatement.getLast(); ++it, ++position)
	{
		if(isAssignBinary(*it, &currentAEB, position))
		{	
			if(checkAEBList(&AEBs, &currentAEB, blockStatement))
			{
				position++;
				result++;
			}
		}
		checkVarsChanging(&AEBs, *it);
	}

	return result;
}

bool isAssignBinary(StatementBase& stmt, AEB* result, int position)
{
	ExpressionStatement* expressionStatement = dynamic_cast<ExpressionStatement*>(&stmt);
	if (expressionStatement == NULL)
	{
		return false;
	}
	ExpressionBase& expression = expressionStatement->get();
	// ... = ...
	BasicCallExpression* assignExpression = dynamic_cast<BasicCallExpression*>(&expression);
	if (assignExpression == NULL || assignExpression->getKind() != BasicCallExpression::BCK_ASSIGN)
	{
		return false;
	}
	// variable = ...
	ReferenceExpression* referenceExpression = dynamic_cast<ReferenceExpression*>(&assignExpression->getArgument(0));
	if (referenceExpression == NULL)
	{
		return false;
	}
	// variable = expression
	ExpressionBase* rightExpression = dynamic_cast<ExpressionBase*>(&assignExpression->getArgument(1));
	if (rightExpression == NULL)
	{
		return false;
	}
	// variable = operand1 op operand2
	BasicCallExpression* rightBinaryExpression = dynamic_cast<BasicCallExpression*>(rightExpression);
	if (rightBinaryExpression == NULL || 
		(rightBinaryExpression->getKind() != BasicCallExpression::BCK_BINARY_MINUS
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_BINARY_PLUS
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_DIVISION
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_MULTIPLY
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_INTEGER_DIVISION
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_INTEGER_MOD
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_EQUAL
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_NOT_EQUAL
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_LOGICAL_AND
		&& rightBinaryExpression->getKind() != BasicCallExpression::BCK_LOGICAL_OR))
	{
		return false;
	}
	result->expr		= rightBinaryExpression->clone();
	result->wholeExpr	= assignExpression;
	result->vartype		= referenceExpression->getResultType();
	result->tmpVar		= NULL;
	result->position	= position;
	return true;
}

bool checkAEBList(AEBList* AEBs, AEB* expr, BlockStatement& block)
{
	bool found = false;
	for(AEBList::iterator it = AEBs->begin(); it != AEBs->end(); ++it)
	{	
		if(it->expr->isEqual(*expr->expr))
		{
			found = true;
			if(it->tmpVar == NULL)
			{
				VariableDeclaration& var = OPS::Reprise::Editing::createNewVariable(*it->vartype, block);
				ReferenceExpression* varRef = new ReferenceExpression(var);
				it->tmpVar = varRef;
				BasicCallExpression* newAssign = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, varRef->clone(), it->wholeExpr->getArgument(1).clone());
				ExpressionStatement* newExpr = new ExpressionStatement(newAssign);

				blockAddBeforePosition(block, it->position, newExpr);
				renumberAEBs(AEBs, &it);
				it->wholeExpr->setArgument(1, varRef);
				expr->wholeExpr->setArgument(1, varRef);
				return true;
			}
			else
			{
				expr->wholeExpr->setArgument(1, it->tmpVar);
			}
		}
	}
	if(!found)
	{
		AEBs->push_back(*expr);
	}
	return false;
}

void checkVarsChanging(AEBList* AEBs, StatementBase& stmt)
{
	ExpressionStatement* expressionStatement = dynamic_cast<ExpressionStatement*>(&stmt);
	if (expressionStatement == NULL)
	{
		return;
	}
	ExpressionBase& expression = expressionStatement->get();
	// ... = ...
	BasicCallExpression* assignExpression = dynamic_cast<BasicCallExpression*>(&expression);
	if (assignExpression == NULL || assignExpression->getKind() != BasicCallExpression::BCK_ASSIGN)
	{
		return;
	}
	// variable = ...
	ReferenceExpression* referenceExpression = dynamic_cast<ReferenceExpression*>(&assignExpression->getArgument(0));
	if (referenceExpression == NULL)
	{
		return;
	}

	std::list<AEBList::iterator> AEBToDelete;
	for(AEBList::iterator it = AEBs->begin(); it != AEBs->end(); ++it)
	{	
		if(isVarInExpression(referenceExpression, it->expr))
		{
			AEBToDelete.push_back(it);
		}
	}
	for(std::list<AEBList::iterator>::iterator it = AEBToDelete.begin(); it != AEBToDelete.end(); ++it)
	{	
		AEBs->erase(*it);
	}
}

bool isVarInExpression(ReferenceExpression* var, ExpressionBase* expr)
{
	bool result = false;

	BasicCallExpression* bcExpr = dynamic_cast<BasicCallExpression*>(expr);
	ReferenceExpression* refExpr = dynamic_cast<ReferenceExpression*>(expr);
	if(bcExpr != NULL)
	{
		for(int i = 0; i < bcExpr->getArgumentCount(); i++)
		{
			result |= isVarInExpression(var, &bcExpr->getArgument(i));
		}
	}
	else if(refExpr != NULL)
	{
		result = refExpr->isEqual(*var);
	}

	return result;
}
void blockAddBeforePosition(BlockStatement& block, int position, StatementBase* stmt)
{
	int i = 0;
	for(BlockStatement::Iterator it = block.getFirst(); it != block.getLast(); ++it, ++i)
	{
		if(i == position)
		{
			block.addBefore(it, stmt);
		}
	}
}

void renumberAEBs(AEBList* AEBs, AEBList::iterator* from)
{
	for(AEBList::iterator it = *from; it != AEBs->end(); ++it)
	{
		it->position++;
	}
}

} // Scalar
} // OPS
} // Transforms
