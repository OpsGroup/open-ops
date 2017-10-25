#include "Transforms/If/IfExtraction/IfExtraction.h"
#include "IfExtractionDeepWalkers.h"

#include "Reprise/ServiceFunctions.h"

#include "Shared/StatementsShared.h"
#include "Shared/Checks.h"
#include "Shared/DataShared.h"
#include "Shared/RepriseClone.h"
#include "OPS_Core/Localization.h"
#include <algorithm>

namespace OPS
{
namespace Transforms
{
	using namespace std;
	using namespace OPS::Reprise;
	using namespace OPS::TransformationsHub;
	using namespace Shared;

	IfExtraction::IfExtraction(): TransformBase(),
		m_pIfStatement(NULL),
		m_pForStatement(NULL),
		m_pLoopCounter(NULL)
	{
		// Create parameters descriptions
	
		// 1-st parameter is IF statement
		ArgumentInfo firstArgInfo(ArgumentValue::StmtIf, _TL("Conditional statement which'll be extracted.", "Условный оператор, подлежащий выносу."));
		this->m_argumentsInfo.push_back(firstArgInfo);

		// 2-nd parameter is for statement
		ArgumentInfo secondArgInfo(ArgumentValue::StmtFor, "Цикл, подлежащий разбиению.");
		this->m_argumentsInfo.push_back(secondArgInfo);
	}

    bool IfExtraction::isApplicable(ProgramUnit *program, const ArgumentValues &params, string *message)
    {
        m_pIfStatement = params[0].getAsIf();
        m_pForStatement = params[1].getAsFor();

        try
        {
            if (!checkIfInForStatement())
                return false;

            if (!checkForStatement())
                return false;

            m_pLoopCounter = &(Editing::getBasicForCounter(*m_pForStatement).getReference());

            if (!checkIfStatement())
                return false;

			return true;
        }
        catch(OPS::RuntimeError& err)
        {
            if (message) *message = err.getMessage();
            return false;
        }
    }

    void IfExtraction::makeTransformImpl(OPS::Reprise::ProgramUnit *pProgram, const OPS::TransformationsHub::ArgumentValues &params)
	{
		OPS_ASSERT(pProgram != NULL);
		OPS_ASSERT(m_argumentsInfo.validate(params) == true);
		transformNotDependentIfStatement();
	}

	bool IfExtraction::checkForStatement()
	{
		OPS_ASSERT(m_pForStatement != NULL);

		bool result = true;

		result = result && Editing::forIsBasic(*m_pForStatement);
		if(!result)
		{
			throw IfExtractionException(_TL("For statement is not canonized.", "Цикл не канонизирован."));
		}

		Checks::CompositionCheckObjects compChecksObjects;
		compChecksObjects<<Checks::CompositionCheckObjects::CCOT_BlockStatement;
		compChecksObjects<<Checks::CompositionCheckObjects::CCOT_EmptyStatement;
		compChecksObjects<<Checks::CompositionCheckObjects::CCOT_ExpressionStatement;
		compChecksObjects<<Checks::CompositionCheckObjects::CCOT_ForStatement;
		compChecksObjects<<Checks::CompositionCheckObjects::CCOT_IfStatement;
		compChecksObjects<<Checks::CompositionCheckObjects::CCOT_SwitchStatement;
		compChecksObjects<<Checks::CompositionCheckObjects::CCOT_WhileStatement;
		
		result = result && Checks::makeCompositionCheck(*m_pForStatement, compChecksObjects);
		if(!result)
		{
			throw IfExtractionException(_TL("For statement must contain only: BlockStatement, EmptyStatement, ExpressionStatement, ForStatement, IfStatement, SwitchStatement, WhileStatement.", 
				"Исходный цикл должен содержать только: BlockStatement, EmptyStatement, ExpressionStatement, ForStatement, IfStatement, SwitchStatement, WhileStatement."));
		}

		return result;
	}

	bool IfExtraction::checkIfInForStatement()
	{
		OPS_ASSERT(m_pIfStatement != NULL);
		OPS_ASSERT(m_pForStatement != NULL);

		bool result = true;

		result = result && Shared::contain(m_pForStatement, m_pIfStatement);
		if(!result)
		{
			throw IfExtractionException(_TL("For statement does not contain if statement.", "Исходный цикл не содержит исходного условного оператора."));
		}

		return result;
	}

	bool IfExtraction::checkIfStatement()
	{
		OPS_ASSERT(m_pIfStatement != NULL);
		OPS_ASSERT(m_pForStatement != NULL);
		OPS_ASSERT(m_pLoopCounter != NULL);

		bool result = true;

		// TODO: Check all loop counters(case of loop nest)
		list<VariableDeclaration*> varsInCondition = Shared::getAllVariableDeclarations(&(m_pIfStatement->getCondition()));
		result = result && find(varsInCondition.begin(), varsInCondition.end(), m_pLoopCounter) == varsInCondition.end();
		if(!result)
		{
			throw IfExtractionException(_TL("Condition of If statement contains loop counter", "Условие исходного условного оператора зависит от счетчика цикла."));
		}

		set<VariableDeclaration*> declaredVars = Shared::getDeclaredVariables(m_pForStatement);

		for(list<VariableDeclaration*>::iterator it = varsInCondition.begin(); it != varsInCondition.end(); ++it)
		{
			if(declaredVars.find(*it) != declaredVars.end())
			{
				result = false;
			}
		}

		if(!result)
		{
			throw IfExtractionException(_TL("Condition of conditional statement includes variable which is declared inside source for statement", "Условие исходного условного оператора зависит от переменных, объявленных внутри исходного цикла."));
		}

		return result;
	}
	
	void IfExtraction::transformNotDependentIfStatement()
	{
		OPS_ASSERT(m_pIfStatement != NULL);
		OPS_ASSERT(m_pForStatement != NULL);

		IfStatement* pNewIfStatement = new IfStatement(m_pIfStatement->getCondition().clone());

		BlockStatement::Iterator forStatementIterator = m_pForStatement->getParentBlock().convertToIterator(m_pForStatement);
		m_pForStatement->getParentBlock().addBefore(forStatementIterator, pNewIfStatement);

        Declarations& declarations = m_pIfStatement->getRootBlock().getDeclarations();
        ReprisePtr<StatementBase> rpSourceIfThenBranch = cloneStatement(m_pIfStatement->getThenBody(), declarations, declarations);
		BlockStatement* pSourceIfThenBranch = rpSourceIfThenBranch->cast_ptr<BlockStatement>();
        ReprisePtr<StatementBase> rpSourceIfElseBranch = cloneStatement(m_pIfStatement->getElseBody(), declarations, declarations);
		BlockStatement* pSourceIfElseBranch = rpSourceIfElseBranch->cast_ptr<BlockStatement>();

		BlockStatement::Iterator thenBlockAfterSubstitutionIt = m_pIfStatement->getParentBlock().replace(m_pIfStatement->getParentBlock().convertToIterator(m_pIfStatement), pSourceIfThenBranch);

		pNewIfStatement->getThenBody().addFirst(cloneStatement(*m_pForStatement, declarations, declarations).get());

		BlockStatement::Iterator elseBlockAfterSubstitutionIt = pSourceIfThenBranch->getParentBlock().replace(thenBlockAfterSubstitutionIt, pSourceIfElseBranch);
		OPS_UNUSED(elseBlockAfterSubstitutionIt);
	
		pNewIfStatement->getElseBody().addFirst(cloneStatement(*m_pForStatement, declarations, declarations).get());
	
		m_pForStatement->getParentBlock().erase(forStatementIterator);
	}
}
}
