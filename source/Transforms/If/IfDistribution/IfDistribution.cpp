#include "Transforms/If/IfDistribution/IfDistribution.h"
#include "IfDistributionDeepWalker.h"

#include "Reprise/ServiceFunctions.h"

#include "Analysis/AbstractDepGraph.h"

#include "Shared/ExpressionHelpers.h"
#include "Shared/Checks.h"
#include "Shared/RepriseClone.h"
#include "OPS_Core/Localization.h"

namespace OPS
{
	namespace Transforms
	{
		using namespace OPS::Reprise;
		using namespace OPS::TransformationsHub;
		using namespace OPS::Shared::ExpressionHelpers;
		using namespace OPS::Shared;
        using namespace OPS::Analysis;

		/*
		
			IfDistribution implementation	
			
		*/

		IfDistribution::IfDistribution(): TransformBase()
		{
			// Create parameters descriptions

			// 1-st parameter is IF statement
			ArgumentInfo firstArgInfo(ArgumentValue::StmtIf, _TL("Conditional operator which'll be distributed.", "Разрезаемый условный оператор."));
			this->m_argumentsInfo.push_back(firstArgInfo);

			// 2-nd parameter is any statement
			ArgumentInfo secondArgInfo(ArgumentValue::StmtAny, _TL("Border of distribution.", "Граница разбиения."));
			this->m_argumentsInfo.push_back(secondArgInfo);

			// 3-rd parameter is boolean flag
			ArgumentInfo thirdArgInfo(ArgumentValue::Bool, _TL("Always create new variable.", "Всегда создавать новую переменную."));
			this->m_argumentsInfo.push_back(thirdArgInfo);
		};

        bool IfDistribution::isApplicable(ProgramUnit*, const ArgumentValues &params, std::string *message)
        {
            IfDistributionBasic transformation(params[0].getAsIf(), params[1].getAsStatement(), params[2].getAsBool());

            if (!transformation.analyseApplicability())
            {
                if (message) *message = transformation.getErrorMessage();
                return false;
            }
            return true;
        }

        void IfDistribution::makeTransformImpl(ProgramUnit* program, const ArgumentValues& params)
		{
			OPS_UNUSED(program);
			if(!getArgumentsInfo().validate(params))
			{
				throw IfDistributionException(_TL("Invalid arguments.", "Некорректные параметры."));
			}

			IfDistributionBasic transformation(params[0].getAsIf(), params[1].getAsStatement(), params[2].getAsBool());

			if(!transformation.analyseApplicability())
			{
				throw IfDistributionException(transformation.getErrorMessage());
			}

			transformation.makeTransformation();
		}

		/*
		
		IfDistributionBasic implementation
		
		*/
		IfDistributionBasic::IfDistributionBasic(IfStatement* pIfStatement, StatementBase* pBorderStatement, bool alwaysCreateNewVar):
			m_pIfStatement(pIfStatement), 
			m_pBorderStatement(pBorderStatement), 
			m_alwaysCreateNewVar(alwaysCreateNewVar),
			m_errorCode(IDBE_NO) 
		{
		}

		void IfDistributionBasic::initializeCommonData()
		{
		}

		bool IfDistributionBasic::analyseApplicability()
		{
			// Check arguments
			if(m_pIfStatement == NULL || m_pBorderStatement == NULL)
			{
				m_errorCode = IDBE_NULL_ARGUMENT;
				return false;
			}

			// Check that m_pIfStatement is parent of m_pBorderStatement
			if(!(m_pBorderStatement->hasParentBlock() && &(m_pBorderStatement->getParentBlock()) == &(m_pIfStatement->getThenBody()))) 
			{
				m_errorCode = IDBE_BORDER_ISNT_IN_IF;
				return false;
			}

			Checks::CompositionCheckObjects compChecksObjects;
			compChecksObjects<<Checks::CompositionCheckObjects::CCOT_BlockStatement;
			compChecksObjects<<Checks::CompositionCheckObjects::CCOT_EmptyStatement;
			compChecksObjects<<Checks::CompositionCheckObjects::CCOT_ExpressionStatement;
			compChecksObjects<<Checks::CompositionCheckObjects::CCOT_ForStatement;
			compChecksObjects<<Checks::CompositionCheckObjects::CCOT_IfStatement;
			compChecksObjects<<Checks::CompositionCheckObjects::CCOT_SwitchStatement;
			compChecksObjects<<Checks::CompositionCheckObjects::CCOT_WhileStatement;
			compChecksObjects<<Checks::CompositionCheckObjects::CCOT_Label;

			if(!Checks::makeCompositionCheck(*m_pIfStatement, compChecksObjects))
			{
				m_errorCode = IDBE_UNAVAILABLE_STMTS;
				return false;
			}

			return true;
		}

		void IfDistributionBasic::makeTransformation()
		{
			OPS_ASSERT(m_pIfStatement != NULL);
			OPS_ASSERT(m_pBorderStatement != NULL);

			// Parent block of m_pIfStatement
			BlockStatement* pParentBlock = &(m_pIfStatement->getParentBlock());

			// Result block 
			BlockStatement* pResultBlock = new BlockStatement();
			pParentBlock->addBefore(pParentBlock->convertToIterator(m_pIfStatement), pResultBlock); 

			// Result condition
			ExpressionBase* pNewCondition = &(m_pIfStatement->getCondition());
			if(m_alwaysCreateNewVar || hasSideEffect())
			{
				VariableDeclaration* pNewVariableDeclaration = &(Editing::createNewVariable(*(m_pIfStatement->getCondition().getResultType()), *pResultBlock, "IfDistribution"));
				ReferenceExpression* pNewVariableReference = new ReferenceExpression(*pNewVariableDeclaration);

				ExpressionStatement* pInitNewVariableStatement = new ExpressionStatement(&(*pNewVariableReference R_AS *m_pIfStatement->getCondition().clone()));
				pResultBlock->addFirst(pInitNewVariableStatement);

				pNewCondition = pNewVariableReference;
			}

			// Result conditional operators
			IfStatement* pResultIfStatement1 = new IfStatement(pNewCondition->clone());
			IfStatement* pResultIfStatement2 = new IfStatement(pNewCondition->clone());

			pResultBlock->addLast(pResultIfStatement1);
			pResultBlock->addLast(pResultIfStatement2);

			// Fill result conditional operators
			BlockStatement::Iterator borderIterator = m_pIfStatement->getThenBody().convertToIterator(m_pBorderStatement);
			BlockStatement::Iterator it = m_pIfStatement->getThenBody().getFirst();
            Declarations& declarations = m_pIfStatement->getRootBlock().getDeclarations();
			for(; it != borderIterator && it.isValid(); ++it)
			{
				pResultIfStatement1->getThenBody().addLast(cloneStatement(*it, declarations, declarations).get());
			}
			for(; it.isValid(); ++it)
			{
				pResultIfStatement2->getThenBody().addLast(cloneStatement(*it, declarations, declarations).get());
			}

			if(!m_pIfStatement->getElseBody().isEmpty())
			{
				pResultIfStatement2->setElseBody(cloneStatement(m_pIfStatement->getElseBody(), declarations, declarations)->cast_ptr<BlockStatement>());
			}

			// TODO: if sourceIfStatement labeled by label A => resultBlock should be labeled by A
			if(m_pIfStatement->hasLabel())
			{
				pResultBlock->setLabel(m_pIfStatement->getLabel());
			}

			// Remove old conditional operator
			pParentBlock->erase(pParentBlock->convertToIterator(m_pIfStatement));
		}

		std::string IfDistributionBasic::getErrorMessage()
		{
			std::string result = "";

			switch(m_errorCode)
			{
			case IDBE_NO:
				break;
			case IDBE_UNKNOWN:
				result = _TL("Unknown error.", "Неизвестная ошибка.");
				break;
			case IDBE_NULL_ARGUMENT:
				result = _TL("Some arguments are null.", "Некоторые параметры преобразования не заданы.");
				break;
			case IDBE_BORDER_ISNT_IN_IF:
				result = _TL("Border isn't placed directly in THEN body of sourceIfStatement.", "Граница разрезания не лежит непосредственно втнутри ветки THEN исходного условного оператора.");
				break;
			case IDBE_UNAVAILABLE_STMTS:
				result = _TL("If statement must contain only: BlockStatement, EmptyStatement, ExpressionStatement, ForStatement, IfStatement, SwitchStatement, WhileStatement.", 
					"Исходный условный оператор должен содержать только: BlockStatement, EmptyStatement, ExpressionStatement, ForStatement, IfStatement, SwitchStatement, WhileStatement.");
				break;
			OPS_DEFAULT_CASE_LABEL;
			}

			return result;
		}

		bool IfDistributionBasic::hasSideEffect()
		{
			OPS_ASSERT(m_pIfStatement != NULL);
			OPS_ASSERT(m_pBorderStatement != NULL);

			SideEffectCheckDeepWalker sideEffectDW(m_pIfStatement, m_pBorderStatement);
			sideEffectDW.visit(*m_pIfStatement);

			if(sideEffectDW.getSideEffectFlag())
			{
				return true;
			}

			// Find first statements
			FirstPartDeepWalker firstPartDW(m_pIfStatement, m_pBorderStatement);
			firstPartDW.visit(*m_pIfStatement);
			FirstPartDeepWalker::StatementsContainer firstStatements = firstPartDW.getFristStatements();

			// Build dependency graph
            std::unique_ptr<AbstractDepGraph> apDepGraph(AbstractDepGraph::buildLamportGraph(m_pIfStatement->getParentBlock(), true));
            for (DependenceList::iterator depIt = apDepGraph->begin(); depIt != apDepGraph->end(); ++depIt)
            {
                AbstractDependence* pDependence = depIt->get();
                if (pDependence->getBeginStatement() == m_pIfStatement && 
                    find(firstStatements.begin(), firstStatements.end(), pDependence->getEndStatement()) != firstStatements.end() &&
                    (pDependence->getType() == AbstractDependence::Anti || pDependence->getType() == AbstractDependence::Output)
                )
                {
                    return true;
                }
            }

			return false;
		}
	}
}
