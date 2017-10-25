#include "Transforms/If/IfSplitting/IfSplitting.h"

#include "Reprise/Service/DeepWalker.h"
#include "Shared/RepriseClone.h"
#include "OPS_Core/Helpers.h"
#include "OPS_Core/Localization.h"

#include <iostream>

namespace OPS
{
namespace Transforms
{
	using namespace OPS::Reprise;
	using namespace OPS::TransformationsHub;
    using namespace OPS::Shared;

	IfSplitting::IfSplitting(): TransformBase()
	{
		// Create parameters descriptions

		// 1-st parameter is IF statement
		ArgumentInfo firstArgInfo(ArgumentValue::StmtIf, _TL("Conditional operator which'll be splitted.", "Расщепляемый условный оператор."));
		this->m_argumentsInfo.push_back(firstArgInfo);
	};

    bool IfSplitting::isApplicable(ProgramUnit *program, const ArgumentValues &params, std::string *message)
    {
        IfStatement* pSourceIfStatement = params[0].getAsIf();

        // Check special restrictions
        BasicCallExpression* pConditionExpr = pSourceIfStatement->getCondition().cast_ptr<BasicCallExpression>();

        if (pConditionExpr == NULL)
        {
            if (message) *message = _TL("Expected (a && b), (a || b) or (!a) in condition.", "Ожидалось (a && b), (a || b) or (!a) в условии.");
            return false;
        }

        BasicCallExpression::BuiltinCallKind operType = pConditionExpr->getKind();
        if(!(operType == BasicCallExpression::BCK_LOGICAL_AND ||
            operType == BasicCallExpression::BCK_LOGICAL_OR ||
            operType == BasicCallExpression::BCK_LOGICAL_NOT))
        {
            if (message) *message = _TL("Expected (a && b), (a || b) or (!a) in condition.", "Ожидалось (a && b), (a || b) or (!a) в условии.");
            return false;
        }

        return true;
    }

    void IfSplitting::makeTransformImpl(ProgramUnit* pProgram, const ArgumentValues& params)
	{
		// Basic validations

		// Check arguments
		OPS_ASSERT(pProgram != NULL);
		OPS_ASSERT(getArgumentsInfo().validate(params));

		////////////////////////
		// Transformation itself
		////////////////////////

        IfStatement* pSourceIfStatement = params[0].getAsIf();
        BasicCallExpression* pConditionExpr = pSourceIfStatement->getCondition().cast_ptr<BasicCallExpression>();
        BasicCallExpression::BuiltinCallKind operType = pConditionExpr->getKind();

        Declarations& declarations = pSourceIfStatement->getRootBlock().getDeclarations();
		BlockStatement* pThenBody = &(pSourceIfStatement->getThenBody());
		BlockStatement* pElseBody = &(pSourceIfStatement->getElseBody());

		IfStatement* pResultIf = new IfStatement();

		if(operType == BasicCallExpression::BCK_LOGICAL_NOT)
		{
			pResultIf->setCondition(pConditionExpr->getArgument(0).clone());

			pResultIf->setThenBody(cloneStatement(*pElseBody, declarations, declarations)->cast_ptr<BlockStatement>());
			pResultIf->setElseBody(cloneStatement(*pThenBody, declarations, declarations)->cast_ptr<BlockStatement>());
		}
		else
		{
			ExpressionBase* pLeftExpr  = pConditionExpr->getArgument(0).clone();
			ExpressionBase* pRightExpr = pConditionExpr->getArgument(1).clone();

			pResultIf->setCondition(pLeftExpr);

			// Inner conditional operator
			IfStatement* pInnerIf = new IfStatement(pRightExpr);
			pInnerIf->setThenBody(cloneStatement(*pThenBody, declarations, declarations)->cast_ptr<BlockStatement>());
			pInnerIf->setElseBody(cloneStatement(*pElseBody, declarations, declarations)->cast_ptr<BlockStatement>());

			if(operType == BasicCallExpression::BCK_LOGICAL_AND)
			{
				pResultIf->getThenBody().addFirst(pInnerIf);
				pResultIf->getElseBody().addFirst(cloneStatement(*pElseBody, declarations, declarations)->cast_ptr<BlockStatement>());
			}
			else if(operType == BasicCallExpression::BCK_LOGICAL_OR)
			{
				pResultIf->getThenBody().addFirst(cloneStatement(*pThenBody, declarations, declarations)->cast_ptr<BlockStatement>());
				pResultIf->getElseBody().addFirst(pInnerIf);
			}
			else
			{
				throw IfSplittingException(_TL("Expected (a && b), (a || b) or (!a) in condition.", "Ожидалось (a && b), (a || b) or (!a) в условии."));
			}
		}

		// Replace source conditional operator by result conditional operator 
		BlockStatement& upperBlock = pSourceIfStatement->getParentBlock();
		BlockStatement::Iterator it  = upperBlock.convertToIterator(pSourceIfStatement);
		upperBlock.replace(it, pResultIf);
	}
}
}
