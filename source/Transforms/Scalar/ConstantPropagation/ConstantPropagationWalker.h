#ifndef _CONSTANT_PROPAGATION_WALKER_H_INCLUDED_
#define _CONSTANT_PROPAGATION_WALKER_H_INCLUDED_

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"

#include <list>

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

using OPS::Reprise::Service::DeepWalker;

using OPS::Reprise::BlockStatement;
using OPS::Reprise::ExpressionStatement;
using OPS::Reprise::ForStatement;
using OPS::Reprise::WhileStatement;
using OPS::Reprise::IfStatement;
using OPS::Reprise::ReferenceExpression;
using OPS::Reprise::StrictLiteralExpression;

class ConstantPropagationWalker: public DeepWalker
{
public:
	ConstantPropagationWalker();
	~ConstantPropagationWalker();

	void visit(BlockStatement& blockStatement);
	void visit(ExpressionStatement& expressionStatement);
	void visit(ForStatement& forStatement);
	void visit(WhileStatement& whileStatement);
	void visit(IfStatement& ifStatement);
	void propagateVars(ExpressionBase& expr);

private:
	typedef std::pair<ReferenceExpression*, StrictLiteralExpression*> Constant2Variable;
	typedef std::list<Constant2Variable> PropagatedVariableList;
	typedef std::pair<BlockStatement*, PropagatedVariableList> PropagatedVariableList2Block;
	typedef std::list<PropagatedVariableList2Block> PropagatedVariableListWithBlocks;

	PropagatedVariableListWithBlocks m_propagatedVariables;
	PropagatedVariableList* m_propagatedVariablesFromCurrentBlock;
	bool m_nestedBlock;
};

}	// Scalar
}	// Transforms
}	// OPS

#endif	// _CONSTANT_PROPAGATION_WALKER_H_INCLUDED_
