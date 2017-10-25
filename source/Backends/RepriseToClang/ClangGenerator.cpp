/// ClangGenerator.cpp
/// Created: 19.02.2013

#include "Backends/RepriseToClang/ClangGenerator.h"

#include "ClangWalker.h"
#include "RepriseClangifier.h"

using namespace OPS::Backends::Clang;
using namespace OPS::Reprise;
using namespace OPS::TransformationsHub;

using namespace OPS::Backends::Clang::Internal;

using namespace std;

//////////////////////////////////////////////////////////////////////
// Generator

Generator::Generator(
	clang::TargetOptions& rTargetOptions)
	:m_pCompilerInstance(0),
	m_ASTContextParams(rTargetOptions)

{
	//
}

Generator::Generator(
	clang::CompilerInstance& rCompilerInstance)
	: m_pCompilerInstance(&rCompilerInstance)
{
	//
}

//////////////////////////////////////////////////////////////////////
// Generator attributes

const ASTContexts& Generator::getResult() const
{
	return m_Contexts;
}

//////////////////////////////////////////////////////////////////////
// Generator implementation

void Generator::makeTransformImpl(
	ProgramUnit* pProgramUnit,
	const ArgumentValues& rcParams)
{
	OPS_UNUSED(rcParams);
	
	// Просмотр всех транслируемых модулей

	if (m_pCompilerInstance != 0)
		m_ASTContextParams.initialize(*m_pCompilerInstance);

	//RepriseClangifier clangifier;
	//pProgramUnit->accept(clangifier);

	ClangWalker builder(m_Contexts, m_ASTContextParams);
	pProgramUnit->accept(builder);
}

// End of File
