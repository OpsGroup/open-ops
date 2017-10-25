#include "ConstantPropagationWalker.h"

#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

using OPS::Reprise::ExpressionBase;
using OPS::Reprise::BasicCallExpression;

ConstantPropagationWalker::ConstantPropagationWalker()
: m_propagatedVariablesFromCurrentBlock(NULL)
, m_nestedBlock(false)
{
}

ConstantPropagationWalker::~ConstantPropagationWalker()
{
	OPS_ASSERT(m_propagatedVariablesFromCurrentBlock == NULL);
}

void ConstantPropagationWalker::visit(BlockStatement& blockStatement)
{
	if (!m_nestedBlock)
	{
		m_nestedBlock = true;

		// If we go into if-then-block, if-else-block, loop-body etc, we add new variable list for this block and set it as current.
		m_propagatedVariables.push_front(PropagatedVariableList2Block(&blockStatement, PropagatedVariableList()));
		PropagatedVariableList* propagatedVariablesFromPreviousBlock = m_propagatedVariablesFromCurrentBlock;
		m_propagatedVariablesFromCurrentBlock = &m_propagatedVariables.begin()->second;

		DeepWalker::visit(blockStatement);

		// When we leave loop-body etc, we really don't know if it is going to be executed.
		// So we can not use information about variables from this block outside of it, and we simply remove this information.
		m_propagatedVariablesFromCurrentBlock = propagatedVariablesFromPreviousBlock;
		m_propagatedVariables.pop_front();
	}
	else
	{
		DeepWalker::visit(blockStatement);
	}
}

void ConstantPropagationWalker::visit(ExpressionStatement& expressionStatement)
{
	// TODO: Need to use SubstitutuionForward for ExpressionStatement&
	// TODO: ExpressionStatement need to be simplified.

	ExpressionBase& expression = expressionStatement.get();
	propagateVars(expression);
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
	// variable = constant
	StrictLiteralExpression* literalExpression = dynamic_cast<StrictLiteralExpression*>(&assignExpression->getArgument(1));
	if (literalExpression == NULL)
	{
		return;
	}

	bool newVariable = true;
	// If variable already is in the list, then we change it's actual value.
	for (PropagatedVariableList::iterator it = m_propagatedVariablesFromCurrentBlock->begin(); it != m_propagatedVariablesFromCurrentBlock->end(); ++it)
	{
		OPS_ASSERT(it->first != NULL);
		if (it->first->isEqual(*referenceExpression))
		{
			OPS_ASSERT(it->second != NULL);
			it->second = literalExpression;
			newVariable = false;
			break;
		}
	}
	// If variable is not in the list, then we add a new pair variable-value.
	if (newVariable)
	{
		m_propagatedVariablesFromCurrentBlock->push_back(Constant2Variable(referenceExpression, literalExpression));

		// If variable was added in another block, then we remove it from that block.
		OPS_ASSERT(m_propagatedVariables.size() != 0);
		for (PropagatedVariableListWithBlocks::iterator it = ++m_propagatedVariables.begin(); it != m_propagatedVariables.end(); ++it)
		{
			for (PropagatedVariableList::iterator _it = it->second.begin(); _it != it->second.end(); ++_it)
			{
				if (_it->first->isEqual(*referenceExpression))
				{
					it->second.erase(_it);
					return;
				}
			}
		}
	}
}

void ConstantPropagationWalker::visit(ForStatement& forStatement)
{
	// TODO: Need to use SubstitutuionForward for ExpressionBase& in for control expression.
	propagateVars(forStatement.getInitExpression());
	propagateVars(forStatement.getFinalExpression());
	propagateVars(forStatement.getStepExpression());

	m_nestedBlock = false;
	visit(forStatement.getBody());
}

void ConstantPropagationWalker::visit(WhileStatement& whileStatement)
{
	// TODO: Need to use SubstitutuionForward for ExpressionBase& in while control expression.
	propagateVars(whileStatement.getCondition());

	m_nestedBlock = false;
	visit(whileStatement.getBody());
}

void ConstantPropagationWalker::visit(IfStatement& ifStatement)
{
	// TODO: Need to use SubstitutuionForward for ExpressionBase& in if control expression.
	propagateVars(ifStatement.getCondition());

	m_nestedBlock = false;
	visit(ifStatement.getThenBody());
	m_nestedBlock = false;
	visit(ifStatement.getElseBody());
}

void ConstantPropagationWalker::propagateVars( ExpressionBase& expr )
{
	for (PropagatedVariableList::iterator it = m_propagatedVariablesFromCurrentBlock->begin(); it != m_propagatedVariablesFromCurrentBlock->end(); ++it)
	{
		makeSubstitutionForward(expr, *it->first, ReprisePtr<ExpressionBase>(it->second->clone()));
	}
}
}	// Scalar
}	// Transforms
}	// OPS
