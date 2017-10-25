#include "Transforms/Subroutines/InliningWithSubstitution.h"
#include "Transforms/Scalar/SSASubstitutionForward/SSASubstitutionForward.h"
#include "FrontTransforms/CantoToReprise.h"
#include "OPS_Core/Localization.h"

namespace OPS
{
namespace Transforms
{
namespace Subroutines
{

using namespace OPS::TransformationsHub;
using namespace OPS::Reprise;
using namespace OPS::Analysis::SSAForms;

typedef std::list<VariableDeclaration*> VariableList;
typedef std::list<StatementBase*> StatementList;

void makeInliningWithSubstitution(OPS::Reprise::SubroutineCallExpression* pCall)
{
	VariableList variables;
	inlineSubstitution(pCall, &variables);

	if (variables.empty())
		return;

	// Нужно чтобы все переменные были объявлены в одном блоке
	for(VariableList::iterator it = variables.begin(); it != variables.end(); ++it)
	{
		OPS_ASSERT(&(*it)->getDefinedBlock() == &variables.front()->getDefinedBlock());
	}

	// Запомним этот блок
	BlockStatement& block = variables.front()->getDefinedBlock();
	StatementBase* firstStatement = block.isEmpty() ? 0 : &*block.getFirst();

	StatementList assigns;

	for(VariableList::iterator it = variables.begin(); it != variables.end(); ++it)
	{
        StatementBase* assign = OPS::Frontend::C2R::convertVariableInit(**it, firstStatement);
		assigns.push_back(assign);
	}

	for(StatementList::iterator it = assigns.begin(); it != assigns.end(); ++it)
	{
		SSAForm ssa(block);

		//Scalar::makeSSASubstitutionForward(ssa, );
	}
}

InliningWithSubtitution::InliningWithSubtitution()
{
	ArgumentInfo firstArgInfo(ArgumentValue::ExprCall, _TL("Subroutine call, which will be inlined.", "Встраиваемый вызов функции."));
	this->m_argumentsInfo.push_back(firstArgInfo);
}

void InliningWithSubtitution::makeTransformImpl(OPS::Reprise::ProgramUnit *program, const OPS::TransformationsHub::ArgumentValues &params)
{
	OPS_ASSERT(program != NULL);
	OPS_ASSERT(getArgumentsInfo().validate(params));

	SubroutineCallExpression* pSubroutineCall = params[0].getAsCall();

	makeInliningWithSubstitution(pSubroutineCall);
}

}
}
}
