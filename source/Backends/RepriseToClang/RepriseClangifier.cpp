/// RepriseClangifier.cpp
///   Preparation visitor for ClangWalker

#include "RepriseClangifier.h"

#include <string>

using namespace OPS::Backends::Clang::Internal;
using namespace OPS::Reprise;

//////////////////////////////////////////////////////////////////////
// RepriseClangifier

RepriseClangifier::RepriseClangifier()
{
	//
}

//////////////////////////////////////////////////////////////////////
// RepriseClangifier overrides

void RepriseClangifier::visit(Declarations& rDeclarations)
{
	Declarations::Iterator it = rDeclarations.getFirst();
	do
	{
		if ((*it).is_a<VariableDeclaration>())
		{
			VariableDeclaration& rVarDecl = (*it).cast_to<VariableDeclaration>();
			if(rVarDecl.hasDefinedBlock())
			{
				BlockStatement& rDefinedBlock =
					rVarDecl.getDefinedBlock();
				RepriseVarDeclWrap* pVarDeclWrap =
					new RepriseVarDeclWrap(rVarDecl.clone());
				rDefinedBlock.addFirst(pVarDeclWrap);

				Declarations::Iterator itToDelete = it;
				it++;
				rDeclarations.erase(itToDelete); // корректно ли?
			}
			else
			{
				it++;
			}
		}
		else
		{
			it++;
		}
	}
	while (it.isValid() && it != rDeclarations.getLast());

	DeepWalker::visit(rDeclarations);
}

//////////////////////////////////////////////////////////////////////
// RepriseVarDeclWrap

RepriseVarDeclWrap::RepriseVarDeclWrap(
	VariableDeclaration *pVariableDeclaration)
	: m_varDecl(pVariableDeclaration)
{
	//
}

RepriseVarDeclWrap::RepriseVarDeclWrap(const RepriseVarDeclWrap& other) : StatementBase(other),
	m_varDecl(other.m_varDecl->clone())
{
	//
}

const VariableDeclaration& RepriseVarDeclWrap::get(void) const
{
	return *m_varDecl;
}

VariableDeclaration& RepriseVarDeclWrap::get(void)
{
	return *m_varDecl;
}

void RepriseVarDeclWrap::set(VariableDeclaration * const pVariableDeclaration)
{
	m_varDecl.reset(pVariableDeclaration);
}

void RepriseVarDeclWrap::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	OPS_UNUSED(source)
	OPS_UNUSED(destination)
	throw RepriseError("Unexpected expression replacement in RepriseVarDeclWrap.");
}

//	RepriseVarDeclWrap - RepriseBase implementation
int RepriseVarDeclWrap::getChildCount(void) const
{
	return 1;
}

RepriseBase& RepriseVarDeclWrap::getChild(const int index)
{
	switch (index)
	{
	case 0:
		return *m_varDecl;
	default:
		throw UnexpectedChildError("RepriseVarDeclWrap::getChild");
	}
}

std::string RepriseVarDeclWrap::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += m_varDecl->dumpState() + "\n";
	return state;
}

// End of File
