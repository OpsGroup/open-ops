#include "Transforms/Scalar/ConstantPropagation/ConstantPropagation.h"

#include "ConstantPropagationWalker.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

using namespace OPS::Reprise;

void makeConstantPropagation(BlockStatement& blockStatement)
{
	ConstantPropagationWalker constantPropagationWalker;
	constantPropagationWalker.visit(blockStatement);
}

bool isConstantVariable(OPS::Reprise::VariableDeclaration &variable)
{
	return variable.getType().isConst() && variable.hasNonEmptyInitExpression();
}

typedef std::set<VariableDeclaration*> VariableSet;

class ConstantVariablePropagationWalker : public OPS::Reprise::Service::DeepWalker
{
public:
	explicit ConstantVariablePropagationWalker(const VariableSet& variables)
		:m_variables(variables)
	{
	}

	void visit(ReferenceExpression& refExpr)
	{
		VariableSet::const_iterator it = m_variables.find(&refExpr.getReference());
		if (it != m_variables.end())
		{
			ReprisePtr<ExpressionBase> initExpr((*it)->getInitExpression().clone());
			initExpr->accept(*this);
			OPS::Reprise::Editing::replaceExpression(refExpr, initExpr);
		}
	}

private:
	const VariableSet& m_variables;
};

void propagateConstantVariable(OPS::Reprise::VariableDeclaration &variable)
{
	VariableSet variables;
	variables.insert(&variable);
	return propagateConstantVariables(variables);
}

void propagateConstantVariables(const VariableSet &variables)
{
	if (variables.empty()) return;

	for(VariableSet::const_iterator it = variables.begin(); it != variables.end(); ++it)
		if (!isConstantVariable(**it))
			throw OPS::RuntimeError("propagateConstantVariable : variable is not constant");

	ConstantVariablePropagationWalker walker(variables);
	if (ProgramUnit* program = (*variables.begin())->findProgramUnit())
	{
		program->accept(walker);
	}
	else if (TranslationUnit* unit = (*variables.begin())->findTranslationUnit())
	{
		unit->accept(walker);
	}
	else
	{
		throw OPS::RuntimeError("Could not find program or translation unit");
	}

	// Удаляем переменные
	for(VariableSet::const_iterator it = variables.begin(); it != variables.end(); ++it)
	{
		Declarations& decls = (*it)->getParent()->cast_to<OPS::Reprise::Declarations>();
		Declarations::Iterator itVariable = decls.convertToIterator(*it);
		decls.erase(itVariable);
	}
}

class ConstantVariableFinder : public OPS::Reprise::Service::DeepWalker
{
public:
	std::vector<VariableDeclaration*> variables;

	void visit(VariableDeclaration& variable)
	{
		if (isConstantVariable(variable))
			variables.push_back(&variable);
	}
};

void propagateAllConstantVariables(OPS::Reprise::RepriseBase& fragment)
{
	ConstantVariableFinder finder;
	fragment.accept(finder);

	VariableSet variables(finder.variables.begin(), finder.variables.end());

	propagateConstantVariables(variables);
}

} // Scalar
} // OPS
} // Transforms
