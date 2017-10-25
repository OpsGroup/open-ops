#include "Transforms/Scalar/SwitchToIf/SwitchToIf.h"
#include "Reprise/Reprise.h"
#include "Shared/LabelsShared.h"
#include "Shared/NodesCollector.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

	using namespace Reprise;

IfStatement* makeIfFromCase(CaseVariantDescriptor* caseVariant, VariableDeclaration* value);

class IfsCollector {
private:
	StatementBase* m_CurrentIf;
	BlockStatement* m_Default;
public:
	IfsCollector(BlockStatement* defaultBranch): m_CurrentIf(NULL), m_Default(defaultBranch) {}
	void operator()(IfStatement* argument) 
	{
		if(m_CurrentIf == NULL) 
		{
			argument->setElseBody(m_Default);
			m_CurrentIf = argument;
		} else {
			argument->getElseBody().addFirst(m_CurrentIf);
			m_CurrentIf = argument;
		}
	}
	StatementBase* getResult() { return m_CurrentIf; }
};

class breakRemover {
private:
	StatementBase* m_Exit;
public:
	breakRemover(StatementBase* exit): m_Exit(exit)
	{}
    void operator()(Canto::HirBreakStatement* breakStmt)
	{
		ReprisePtr<GotoStatement> gotoStmt(new GotoStatement(m_Exit));
		Editing::replaceStatement(*breakStmt, gotoStmt);
	}

};


void makeIfFromSwitch(SwitchStatement *pSwitch)
{
	OPS_ASSERT(pSwitch != NULL);
	OPS_ASSERT(pSwitch->hasParentBlock());
	BlockStatement* outerBlock = &(pSwitch->getParentBlock());
	
	ExpressionBase* condition = &(pSwitch->getCondition());
	ReprisePtr<TypeBase> switchConditionType(condition->getResultType());
	VariableDeclaration* switchCondition = &Editing::createNewVariable(*switchConditionType, *outerBlock, "condition");
	BasicCallExpression* conditionAssignment = 
		new BasicCallExpression
		(
			BasicCallExpression::BCK_ASSIGN,
			new ReferenceExpression(*switchCondition), 
			condition->clone()
		);
	outerBlock->addBefore(outerBlock->convertToIterator(pSwitch), new ExpressionStatement(conditionAssignment));
	std::list<IfStatement*> resultIfs;
	int caseCount = pSwitch->getCaseCount();
	for(int nCase = 0; nCase < caseCount; ++nCase)
	{
		resultIfs.push_back(makeIfFromCase(&(pSwitch->getCase(nCase)), switchCondition));
	}
	IfsCollector i = std::for_each(resultIfs.rbegin(), resultIfs.rend(), IfsCollector(&(pSwitch->getDefaultBody())));
	StatementBase* resultIf = i.getResult();
	outerBlock->addBefore(outerBlock->convertToIterator(pSwitch), resultIf);
	
	StatementBase* guard = new EmptyStatement();
	outerBlock->addAfter(outerBlock->convertToIterator(resultIf), guard);
    Shared::NodesCollector<Canto::HirBreakStatement> breakCollector;
	resultIf->accept(breakCollector);

	guard->setUniqueLabel();
	std::for_each(breakCollector.getCollection().begin(), breakCollector.getCollection().end(), breakRemover(guard));
	
	outerBlock->erase(outerBlock->convertToIterator(pSwitch));
}

class ConditionsCollector {
private:
	ExpressionBase* m_CurrentCondition;
public:
	ConditionsCollector(): m_CurrentCondition(NULL) {}
	void operator()(ExpressionBase* argument) 
	{
		if(m_CurrentCondition == NULL) 
		{
			m_CurrentCondition = argument;
		} else {
			BasicCallExpression* newCondition =
				new BasicCallExpression 
				(
				BasicCallExpression::BCK_LOGICAL_OR,
					argument,
					m_CurrentCondition
				);
			m_CurrentCondition = newCondition;
		}
	}
	ExpressionBase* getResult() { return m_CurrentCondition; }
};

IfStatement* makeIfFromCase(CaseVariantDescriptor* caseVariant, VariableDeclaration* value)
{
	std::list<ExpressionBase*> partialConditions;
	ReprisePtr<ReferenceExpression> valueReference(new ReferenceExpression(*value));
	int partsCount = caseVariant->getVariantCount();
	for(int nPart = 0; nPart < partsCount; ++nPart)
	{
		partialConditions.push_back(
			new BasicCallExpression
				(
				BasicCallExpression::BCK_EQUAL,
					valueReference->clone(),
					caseVariant->getVariant(nPart).clone()
				)
			);
	}
	ConditionsCollector c = std::for_each(partialConditions.rbegin(), partialConditions.rend(), ConditionsCollector());
	ExpressionBase* condition = c.getResult();
	OPS_ASSERT(condition != NULL);
	IfStatement* result = new IfStatement(condition);
	result->setThenBody(caseVariant->getBody().clone());
	return result;
}

void makeIfFromSwitch(PlainSwitchStatement* pSwitch)
{
	OPS_ASSERT(pSwitch != NULL);
	OPS_ASSERT(pSwitch->hasParentBlock());
	BlockStatement* outerBlock = &(pSwitch->getParentBlock());
	
	ExpressionBase* condition = &(pSwitch->getCondition());
	ReprisePtr<TypeBase> switchConditionType(condition->getResultType());
	VariableDeclaration* switchCondition = &Editing::createNewVariable(*switchConditionType, *outerBlock, "condition");
	BasicCallExpression* conditionAssignment = 
		new BasicCallExpression
		(
			BasicCallExpression::BCK_ASSIGN,
			new ReferenceExpression(*switchCondition), 
			condition->clone()
		);
	outerBlock->addBefore(outerBlock->convertToIterator(pSwitch), new ExpressionStatement(conditionAssignment));
	int casesNumber = pSwitch->getLabelCount();
	for(int nCase = 0; nCase < casesNumber; ++nCase)
	{
		PlainCaseLabel* label = &(pSwitch->getLabel(nCase));
		if(label->isDefault()) {
			label->getStatement().setUniqueLabel();
			GotoStatement* newGoto = new GotoStatement(&(label->getStatement()));
			pSwitch->getBody().addFirst(newGoto);
		}
	}
	for(int nCase = casesNumber - 1; nCase >= 0; --nCase)
	{
		PlainCaseLabel* label = &(pSwitch->getLabel(nCase));
		if(label->isDefault()) continue;
		BasicCallExpression* labelCondition = 
			new BasicCallExpression(
				BasicCallExpression::BCK_EQUAL,
				new ReferenceExpression(*switchCondition),
				BasicLiteralExpression::createInteger(label->getValue())
			);
		IfStatement* newIf = new IfStatement(labelCondition);
		label->getStatement().setUniqueLabel();
		GotoStatement* newGoto = new GotoStatement(&(label->getStatement()));
		newIf->getThenBody().addFirst(newGoto);
		pSwitch->getBody().addFirst(newIf);
	}
	BlockStatement* newBlock = pSwitch->getBody().clone();
	outerBlock->addBefore(outerBlock->convertToIterator(pSwitch), newBlock);
	StatementBase* guard = new EmptyStatement();
	outerBlock->addAfter(outerBlock->convertToIterator(newBlock), guard);
    Shared::NodesCollector<Canto::HirBreakStatement> breakCollector;
	newBlock->accept(breakCollector);

	guard->setUniqueLabel();
	std::for_each(breakCollector.getCollection().begin(), breakCollector.getCollection().end(), breakRemover(guard));
	
	if(Shared::isPossibleToGenerateNewLabels(*newBlock))
	{
		Shared::generateNewLabels(*newBlock);
	}
	outerBlock->erase(outerBlock->convertToIterator(pSwitch));
}

}

}

}
