#include <list>
#include <vector>
#include <algorithm>
#include "Transforms/Subroutines/FragmentToSubroutine.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Shared/NodesCollector.h"
#include "Shared/LabelsShared.h"
#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/DepGraph/id.h"	
#include "OPS_Core/Localization.h"

using namespace OPS;
using namespace Reprise;
using namespace Shared;
using namespace TransformationsHub;

namespace OPS {

namespace Transforms {

namespace Subroutines {

	FragmentToSubroutine::FragmentToSubroutine(): TransformBase()
	{
		// Первый параметр - вызов функции
		ArgumentInfo firstArgInfo(ArgumentValue::StmtAny, _TL("First operator of new function's body.", "Первый оператор тела новой функции."));
		ArgumentInfo secondArgInfo(ArgumentValue::StmtAny, _TL("Last operator of new function's body.", "Последний оператор тела новой функции."));
		this->m_argumentsInfo.push_back(firstArgInfo);
		this->m_argumentsInfo.push_back(secondArgInfo);
	};

    bool FragmentToSubroutine::isApplicable(ProgramUnit *program, const ArgumentValues &params, std::string *message)
    {
        StatementBase* pFirstStatement = params[0].getAsStatement();
        StatementBase* pSecondStatement = params[1].getAsStatement();

        BlockStatement* pParentBlock = &(pFirstStatement->getParentBlock());
        if(&(pSecondStatement->getParentBlock()) != pParentBlock)
        {
            if (message) *message = _TL("Statements are not in the same block","Операторы не из одного блока");
            return false;
        }
        return true;
    }

    void FragmentToSubroutine::makeTransformImpl(ProgramUnit* pProgram, const ArgumentValues& params)
	{
		OPS_ASSERT(pProgram != NULL);
		OPS_ASSERT(getArgumentsInfo().validate(params));

		StatementBase* pFirstStatement = params[0].getAsStatement();
		StatementBase* pSecondStatement = params[1].getAsStatement();
		BlockStatement* pParentBlock = &(pFirstStatement->getParentBlock());

		BlockStatement::Iterator pFirst = pParentBlock->convertToIterator(pFirstStatement);
		BlockStatement::Iterator pSecond = pParentBlock->convertToIterator(pSecondStatement);
		////////////////////////
		// Transformation itself
		////////////////////////
		
		fragmentToSubroutine(pFirst, pSecond);
	}


	typedef std::vector<VariableDeclaration*> Variables;

	class VariablesConverter {
	public:
			virtual ExpressionBase* operator()(VariableDeclaration* variable)
			{
				return new ReferenceExpression(*variable);
			}
	};

	class VariablesConverterPtr: public VariablesConverter {
	public:
		ExpressionBase* operator()(VariableDeclaration* variable)
		{
			return new BasicCallExpression(
					BasicCallExpression::BCK_DE_REFERENCE,
					new ReferenceExpression(*variable)				
				);
		}
	};

	class VariablesConverterAddr: public VariablesConverter {
	public:
		ExpressionBase* operator()(VariableDeclaration* variable)
		{
			return new BasicCallExpression(
					BasicCallExpression::BCK_TAKE_ADDRESS,
					new ReferenceExpression(*variable)				
				);
		}
	};

	void swapVariables(Variables& oldVars, Variables& newVars, BlockStatement* block, VariablesConverter* converter)
	{
		OPS_ASSERT(oldVars.size() == newVars.size());
		int variablesCount = oldVars.size();
		for(int nVariable = 0; nVariable < variablesCount; ++nVariable)
		{
			ReprisePtr<ExpressionBase> oldReference(new ReferenceExpression(*oldVars[nVariable]));
			ReprisePtr<ExpressionBase> newReference((*converter)(newVars[nVariable]));
			Transforms::Scalar::makeSubstitutionForward(*block, *oldReference, newReference, true);
		}
	}


    OPS::Reprise::SubroutineDeclaration* fragmentToSubroutine(Reprise::BlockStatement::Iterator first, Reprise::BlockStatement::Iterator last)
	{
        OPS_ASSERT(first->hasParentBlock());
		OPS_ASSERT(last.isValid());
		BlockStatement* block = &(first->getParentBlock());
		Shared::NodesCollector<GotoStatement> gotoCollector;

		// соберем все ссылки на переменные, имеющиеся в заданных операторах
		Shared::NodesCollector<ReferenceExpression> occurenceCollector;
		BlockStatement::Iterator pStatement = first;
		BlockStatement::Iterator pEnd = last; ++pEnd;
		for(; pStatement != pEnd; ++pStatement) { pStatement->accept(occurenceCollector); }
		for(pStatement = first; pStatement != pEnd; ++pStatement) { pStatement->accept(gotoCollector); }
		// операторы goto пока не поддерживаются
		if(gotoCollector.getCollection().size() != 0) throw FragmentToSubroutine::FragmentToSubroutineException(_TL("Goto operators are not allowed.","Операторы goto не поддерживаются."));

		std::set<ReferenceExpression*> occurences;
		for(size_t nReference = 0; nReference < occurenceCollector.getCollection().size(); ++nReference)
		{
			occurences.insert(occurenceCollector.getCollection().at(nReference));
		}
		
		// найдем переменные, к которым идут дуги зависимости снаружи или 
		// из которых идут дуги наружу (только потоковая зависимость)
		// "наружу" - означает в вышележащий блок или в другие операторы того же блока
		StatementBase* outerStatement = block; outerStatement = outerStatement->getParent()->cast_ptr<StatementBase>();
		if(outerStatement == NULL) 
		{
			outerStatement = block;
		} else {
			while(outerStatement != NULL && !outerStatement->is_a<BlockStatement>()) 
			{ 
				outerStatement = outerStatement->getParent()->cast_ptr<StatementBase>(); 
			}
			OPS_ASSERT(outerStatement != NULL && outerStatement->is_a<BlockStatement>());
		}
		BlockStatement* surroundingBlock = &(outerStatement->cast_to<BlockStatement>());

		using namespace DepGraph::Id;
		using namespace DepGraph;

		id BlockId(*surroundingBlock);
		LamportGraph depGraph;
		depGraph.Build(BlockId);
		LampArrowIterator pArrow = depGraph.Begin();

		Variables conflictedVariables;
		Variables localVariables;
		std::map<ReferenceExpression*, bool> VariablesTypes;
		for(std::set<ReferenceExpression*>::iterator pOccur = occurences.begin(); pOccur != occurences.end(); ++pOccur)
		{
			VariablesTypes[*pOccur] = true;
		}
		for(; pArrow != depGraph.End(); ++pArrow)
		{
			ReferenceExpression* source = (*pArrow)->pSrcOccur;
			ReferenceExpression* destination = (*pArrow)->pDepOccur;
			
			bool sourceInOccurences = (occurences.find(source) != occurences.end());
			bool destinationInOccurences = (occurences.find(destination) != occurences.end());

			if(sourceInOccurences || destinationInOccurences)
			{
				if(sourceInOccurences && destinationInOccurences)
				{
					// do nothing
					//VariablesTypes[source] = true;
					//VariablesTypes[destination] = true;
				} else {
					VariablesTypes[source] = false;
					VariablesTypes[destination] = false;
				}

			}

		}
		for(std::map<ReferenceExpression*, bool>::iterator pOccur = VariablesTypes.begin(); pOccur != VariablesTypes.end(); ++pOccur)
		{
			if(pOccur->second)
			{
				Variables::iterator pVar = std::find(localVariables.begin(), localVariables.end(), &(pOccur->first->getReference())); 				
				Variables::iterator pVar1 = std::find(conflictedVariables.begin(), conflictedVariables.end(), &(pOccur->first->getReference())); 				
				if(pVar == localVariables.end() && pVar1 == conflictedVariables.end())
					localVariables.push_back(&(pOccur->first->getReference()));
			} else
			{
				Variables::iterator pVar = std::find(conflictedVariables.begin(), conflictedVariables.end(), &(pOccur->first->getReference())); 				
				Variables::iterator pVar1 = std::find(localVariables.begin(), localVariables.end(), &(pOccur->first->getReference())); 				
				if(pVar == conflictedVariables.end() && pVar1 == localVariables.end())
					conflictedVariables.push_back(&(pOccur->first->getReference()));
			}
		}

		// создадим новую подпрограмму
		BlockStatement* rootBlock = &(surroundingBlock->getRootBlock());
		SubroutineDeclaration* outerSubroutine = &(rootBlock->getParent()->cast_to<SubroutineDeclaration>());
		Declarations* parentDeclarations = &(outerSubroutine->getParent()->cast_to<Declarations>());
		std::string newSubroutineName = "fragmentSubroutine";
		while(parentDeclarations->findSubroutine(newSubroutineName) != NULL) newSubroutineName = newSubroutineName + "_1";
		SubroutineType* newSubroutineType = new SubroutineType(BasicType::voidType());
		SubroutineDeclaration* newSubroutine = new SubroutineDeclaration(newSubroutineType, newSubroutineName);
		newSubroutine->setBodyBlock(ReprisePtr<BlockStatement>(new BlockStatement()));
		parentDeclarations->addBefore(parentDeclarations->convertToIterator(outerSubroutine), newSubroutine);
		BlockStatement* newBlock = &(newSubroutine->getBodyBlock());
		// скопируем в ее тело все указанные операторы
		for(pStatement = first; pStatement != pEnd; ++pStatement) 
		{
			newBlock->addLast(pStatement->clone());
		}
		// создадим новые переменные 
		Variables newLocalVariables;
		Variables newConflictedVariables;
		for(Variables::iterator pVar = localVariables.begin(); pVar != localVariables.end(); ++pVar)
		{
			VariableDeclaration* newVariable = &Editing::createNewVariable((*pVar)->getType(), *newBlock, (*pVar)->getName());
			newVariable->declarators() = (*pVar)->declarators();
			if((*pVar)->hasNonEmptyInitExpression())
			{
				newVariable->setInitExpression(*((*pVar)->getInitExpression().clone()));
			}
			newLocalVariables.push_back(newVariable);
		}

		for(Variables::iterator pVar = conflictedVariables.begin(); pVar != conflictedVariables.end(); ++pVar)
		{
			//&Editing::createNewVariable((*pVar)->getType(), *newBlock, (*pVar)->getName()+"_parameter");
			std::string newName = (*pVar)->getName() + "_parameter";
			while(newSubroutine->getDeclarations().findVariable(newName) != NULL) newName += "_1";
			VariableDeclaration* newVariable = new VariableDeclaration((*pVar)->getType().clone(), newName);
			ParameterDescriptor* newParameter = new ParameterDescriptor(newName, new PtrType((*pVar)->getType().clone()));
			newSubroutineType->addParameter(newParameter);
			newVariable->setParameterReference(*newParameter);
			newSubroutine->getDeclarations().addVariable(newVariable);
			newConflictedVariables.push_back(newVariable);
		}

		VariablesConverter converter;
		VariablesConverterPtr converterPtr;
		swapVariables(localVariables, newLocalVariables, newBlock, &converter);
		swapVariables(conflictedVariables, newConflictedVariables, newBlock, &converterPtr);

		// создадим вызов функции в блоке
		SubroutineCallExpression* newSubroutineCall = new SubroutineCallExpression(new SubroutineReferenceExpression(*newSubroutine));
		ExpressionStatement* newCallStatement = new ExpressionStatement(newSubroutineCall);
		block->addBefore(first, newCallStatement);

		StatementBase* lastRetrieveStatement = NULL;
		for(Variables::iterator pVar = conflictedVariables.begin(); pVar != conflictedVariables.end(); ++pVar)
		{
			VariableDeclaration* newParameterVariable = &Editing::createNewVariable((*pVar)->getType(), *block, (*pVar)->getName() + "_parameter");
			ExpressionBase* expressionStore 
				= new BasicCallExpression(
					BasicCallExpression::BCK_ASSIGN,
					new ReferenceExpression(*newParameterVariable),
					new ReferenceExpression(**pVar)
				);
			ExpressionBase * expressionRetrieve 
				= new BasicCallExpression(
					BasicCallExpression::BCK_ASSIGN,
					new ReferenceExpression(**pVar),
					new ReferenceExpression(*newParameterVariable)
				);
			StatementBase* retrieveStatement = new ExpressionStatement(expressionRetrieve);
			if(lastRetrieveStatement == NULL) lastRetrieveStatement = retrieveStatement;
			block->addBefore(block->convertToIterator(newCallStatement), new ExpressionStatement(expressionStore));
			block->addAfter(block->convertToIterator(newCallStatement), retrieveStatement);
			newSubroutineCall->addArgument(
					new BasicCallExpression(
						BasicCallExpression::BCK_TAKE_ADDRESS,
						new ReferenceExpression(*newParameterVariable)
					)
				);
		}
		// удалим старые операторы
		StatementBase* lastStatement = &(*last);
		if(lastRetrieveStatement == NULL)
			pStatement = block->convertToIterator(newCallStatement);
		else 
			pStatement = block->convertToIterator(lastRetrieveStatement);
		pStatement++; 
		while(pStatement.isValid() && &(*pStatement) != lastStatement) 
		{
			block->erase(pStatement);
			if(lastRetrieveStatement == NULL)
				pStatement = block->convertToIterator(newCallStatement);
			else 
				pStatement = block->convertToIterator(lastRetrieveStatement);
			pStatement++;
		}
		if(pStatement.isValid()) block->erase(pStatement);

		return newSubroutine;
	}

}

}

}
