#include <list>
#include <vector>
#include <algorithm>
#include "Transforms/Subroutines/SubroutineSplitting.h"
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

	SubroutineSplitting::SubroutineSplitting(): TransformBase()
	{
		// Первый параметр - вызов функции
		ArgumentInfo firstArgInfo(ArgumentValue::StmtAny, _TL("Operator on which to divide a subroutine.", "Оператор, по которому нужно разбить подпрограмму."));
		this->m_argumentsInfo.push_back(firstArgInfo);
	};

    bool SubroutineSplitting::isApplicable(ProgramUnit *program, const ArgumentValues &params, std::string *message)
    {
        StatementBase* pMiddleStatement = params[0].getAsStatement();
        BlockStatement* pParentBlock = &(pMiddleStatement->getParentBlock());
        RepriseBase* pSubroutine = pParentBlock->getParent();

        if (!pSubroutine->is_a<SubroutineDeclaration>())
        {
            if (message) *message = _TL("Statement isn't immediately in the subroutine's body","Оператор находится не непосредственно в теле подпрограммы");
            return false;
        }

        SubroutineDeclaration* pDeclaration = pSubroutine->cast_ptr<SubroutineDeclaration>();
        ProgramUnit* unit = pDeclaration->findProgramUnit();
        if (unit == NULL)
        {
            if (message) *message = _TL("Can't find program unit","Не могу найти программу");
            return false;
        }
        return true;
    }

    void SubroutineSplitting::makeTransformImpl(ProgramUnit* pProgram, const ArgumentValues& params)
	{
		OPS_ASSERT(pProgram != NULL);
		OPS_ASSERT(getArgumentsInfo().validate(params));

		StatementBase* pMiddleStatement = params[0].getAsStatement();
		BlockStatement* pParentBlock = &(pMiddleStatement->getParentBlock());
		RepriseBase* pSubroutine = pParentBlock->getParent();

		SubroutineDeclaration* pDeclaration = pSubroutine->cast_ptr<SubroutineDeclaration>();
		ProgramUnit* unit = pDeclaration->findProgramUnit();

		BlockStatement::Iterator pMiddle = pParentBlock->convertToIterator(pMiddleStatement);

		////////////////////////
		// Transformation itself
		////////////////////////
		
		splitSubroutine(pDeclaration, pMiddle, unit);
	}

	namespace {
		
		bool contains(const StatementBase& parent, const StatementBase& child)
		{
			const StatementBase* node = &child;
			while(node != NULL)
			{
				if (node->getParent() == &parent)
				{
					return true;
				}
				node = node->getParent()->cast_ptr<const StatementBase>();
			}
			return false;
		}
		
		bool contains(const ExpressionBase& parent, const ExpressionBase& child)
		{
			const ExpressionBase* node = &child;
			while(node != NULL)
			{
				if (node->getParent() == &parent)
				{
					return true;
				}
				node = node->getParent()->cast_ptr<const ExpressionBase>();
			}
			return false;
		}
	}

	using namespace Reprise;

	class jumpTargetCollector {
	private:
		std::vector<StatementBase*>& m_Collection;
	public:
		jumpTargetCollector(std::vector<StatementBase*>& collection): m_Collection(collection) {}
		void operator()(GotoStatement* gotoStmt)
		{
			m_Collection.push_back(gotoStmt->getPointedStatement());
		}
	};

	bool hasWrongJumps(SubroutineDeclaration* subroutine, BlockStatement::Iterator place)
	{
		// соберем все переходы по меткам из верхней части
		NodesCollector<GotoStatement> gotoCollector;
		BlockStatement* body = &(subroutine->getBodyBlock());
		BlockStatement::Iterator pStatement = body->getFirst();
		BlockStatement::Iterator pFirstPartEnd = place;
		++pFirstPartEnd;
		OPS_ASSERT(pFirstPartEnd.isValid());
		for(; pStatement != pFirstPartEnd; ++pStatement)
		{
			pStatement->accept(gotoCollector);
		}
		std::vector<StatementBase*> jumpedStatements;
		std::for_each(gotoCollector.getCollection().begin(), gotoCollector.getCollection().end(), jumpTargetCollector(jumpedStatements));
		// проверим, не ведел ли какой-нибудь переход в нижнюю часть
		BlockStatement::Iterator pTargetStatement = pFirstPartEnd;
		for(; pTargetStatement.isValid(); ++pTargetStatement)
		{
			std::vector<StatementBase*>::iterator pJumped = jumpedStatements.begin();
			for(; pJumped != jumpedStatements.end(); ++pJumped)
			{
				if(contains(*pTargetStatement, **pJumped)) return true;
			}
		}
		return false;
	}

	class OccurenceDetector {
	private:
		ExpressionBase* m_Occurence;
	public:
		OccurenceDetector(ExpressionBase* occurence): m_Occurence(occurence)
		{}
		bool operator()(ExpressionBase* expression)
		{
			return contains(*expression, *m_Occurence);
		}
	};

	void findConflictedVariables(std::vector<VariableDeclaration*>& conflictedVariables, SubroutineDeclaration* subroutine, BlockStatement::Iterator place)
	{
		using namespace DepGraph;
		BlockStatement* block = &(subroutine->getBodyBlock());
		Id::id  bodyID(block);

		LamportGraph lamb;
		lamb.Build(bodyID);

		LampArrowIterator pArrow = lamb.Begin();
		Shared::NodesCollector<ReferenceExpression> upCollector;
		Shared::NodesCollector<ReferenceExpression> bottomCollector;
		BlockStatement::Iterator pStatement = block->getFirst();
		BlockStatement::Iterator pFirstPartEnd = place;
		++pFirstPartEnd; OPS_ASSERT(pFirstPartEnd.isValid());
		for(; pStatement != pFirstPartEnd; ++pStatement)
		{	
			pStatement->accept(upCollector);
		}
		for(; pStatement.isValid(); ++pStatement)
		{
			pStatement->accept(bottomCollector);
		}
		for(; pArrow != lamb.End(); ++pArrow)
		{
			// будем пока учитывать все зависимости
			if(true/*(*pArrow)->type == DepGraph::FLOWDEP*/)
			{
				ReferenceExpression* conflictedExprFrom = (*pArrow)->pSrcOccur;
				ReferenceExpression* conflictedExprTo = (*pArrow)->pDepOccur;
				bool fromUp = std::find(upCollector.getCollection().begin(), upCollector.getCollection().end(), conflictedExprFrom) != upCollector.getCollection().end();
				bool fromDown = std::find(bottomCollector.getCollection().begin(), bottomCollector.getCollection().end(), conflictedExprTo) != bottomCollector.getCollection().end();
				
				
				if(fromUp && fromDown) 
				{
					ReferenceExpression* conflictedReference = (*pArrow)->pSrcOccur;
					VariableDeclaration* conflictedVariable = &(conflictedReference->getReference());
					conflictedVariables.push_back(conflictedVariable);
				}
			}
		}
	}

	void swapReferences(BlockStatement* block, std::vector<VariableDeclaration*>& conflictedVariables, std::vector<std::string>& newParameters)
	{
		SubroutineDeclaration* subroutine = &(block->getParent()->cast_to<SubroutineDeclaration>());
		for(size_t nVar = 0; nVar < conflictedVariables.size(); ++nVar)
		{
			ReprisePtr<ReferenceExpression> sourceRef(new ReferenceExpression(*(conflictedVariables[nVar])));
			VariableDeclaration* newVariable = &Editing::createNewVariable(conflictedVariables[nVar]->getType(), *block, newParameters[nVar]);
			SubroutineType* type = &(subroutine->getType());
			int parametersCount = type->getParameterCount();
			int nParameter = 0;
			for(; nParameter < parametersCount; ++nParameter)
			{
				if(type->getParameter(nParameter).getName() == newParameters[nVar]) break;
			}
			if(nParameter == parametersCount) continue; //OPS_ASSERT(false);
			newVariable->setParameterReference(type->getParameter(nParameter));

			ReprisePtr<ExpressionBase> destinationRef(
					new BasicCallExpression(
						BasicCallExpression::BCK_DE_REFERENCE, 
						new ReferenceExpression(*newVariable)
					)
				);
			Transforms::Scalar::makeSubstitutionForward(*block, *sourceRef, destinationRef, true);
		}
	}

	void addOldVariables(BlockStatement* block, std::vector<VariableDeclaration*>& oldVariables)
	{
		SubroutineDeclaration* subroutine = &(block->getParent()->cast_to<SubroutineDeclaration>());
		for(size_t nVar = 0; nVar != oldVariables.size(); ++nVar)
		{
			VariableDeclaration* newVariable = new VariableDeclaration(&(oldVariables[nVar]->getType()),oldVariables[nVar]->getName());
			if(oldVariables[nVar]->hasNonEmptyInitExpression())
			{
				newVariable->setInitExpression(oldVariables[nVar]->getInitExpression());
			}
			Declarations* decls = &(subroutine->getDeclarations());
			decls->addLast(newVariable);
			if(oldVariables[nVar]->hasParameterReference()) 
			{
				ParameterDescriptor* oldParameter = &(oldVariables[nVar]->getParameterReference());
				std::string parameterName = oldParameter->getName();
				SubroutineType* type = &(subroutine->getType());
				int parametersCount = type->getParameterCount();
				for(int nParameter = 0; nParameter < parametersCount; ++nParameter)
				{
					ParameterDescriptor* par = &(type->getParameter(nParameter));
					if(par->getName() == parameterName && &(par->getType()) == &(oldVariables[nVar]->getType()))
					{
						newVariable->setParameterReference(*par);
						break;
					}
				}
			}
		}
	}

	void replaceReturns(BlockStatement* block, std::string& returnParameterName)
	{
		SubroutineDeclaration* subroutine = &(block->getParent()->cast_to<SubroutineDeclaration>());

		SubroutineType* type = &(subroutine->getType());
		int parametersCount = type->getParameterCount();
		int nParameter = 0;
		for(; nParameter < parametersCount; ++nParameter)
		{
			if(type->getParameter(nParameter).getName() == returnParameterName) break;
		}
		if(nParameter == parametersCount) return; //OPS_ASSERT(false);

		VariableDeclaration* newVariable = new VariableDeclaration(&(type->getParameter(nParameter).getType()), returnParameterName);
		block->getDeclarations().addFirst(newVariable);
		newVariable->setParameterReference(type->getParameter(nParameter));
		
		Shared::NodesCollector<ReturnStatement> returnCollector;
		block->accept(returnCollector);
		for(size_t nReturn = 0; nReturn < returnCollector.getCollection().size(); ++nReturn)
		{
			ExpressionBase* newReturnExpression = BasicLiteralExpression::createInteger(1);
			ReturnStatement* pReturn = returnCollector.getCollection()[nReturn];
			ExpressionBase* oldReturnExpression = &(pReturn->getReturnExpression());
			BlockStatement* outerBlock = &(pReturn->getParentBlock());
			OPS_ASSERT(outerBlock != NULL);
			BasicCallExpression* returnAssignment =
				new BasicCallExpression(
					BasicCallExpression::BCK_ASSIGN,
					new BasicCallExpression(BasicCallExpression::BCK_DE_REFERENCE, new ReferenceExpression(*newVariable)),
					oldReturnExpression
				);
			outerBlock->addBefore(outerBlock->convertToIterator(pReturn), new ExpressionStatement(returnAssignment));

			pReturn->setReturnExpression(newReturnExpression);
		}
		ReturnStatement* endReturn = new ReturnStatement(BasicLiteralExpression::createInteger(0));
		block->addLast(endReturn);
	}

	bool replaceCall(SubroutineCallExpression* call, std::vector<VariableDeclaration*>& conflictedVariables, SubroutineDeclaration* upSubroutine, SubroutineDeclaration* bottomSubroutine)
	{
		bool result = false;
		//TODO: проверка на применимость преобразования
		StatementBase* outerStatement = call->obtainParentStatement();
		if(outerStatement == NULL) return false;
		if(outerStatement->hasParentBlock() == false) return false;
		BlockStatement* outerBlock = &(outerStatement->getParentBlock());

		ReprisePtr<BasicType> returnVariableType(BasicType::int32Type());
		VariableDeclaration* returnVariable = &Editing::createNewVariable(*returnVariableType, *outerBlock, "__splitted_return");
		
		SubroutineCallExpression* upCall = new SubroutineCallExpression(new SubroutineReferenceExpression(*upSubroutine));
		SubroutineCallExpression* bottomCall = new SubroutineCallExpression(new SubroutineReferenceExpression(*bottomSubroutine));
		int argumentsCount = call->getArgumentCount();
		for(int nArgument = 0; nArgument < argumentsCount; ++nArgument)
		{
			upCall->addArgument(call->getArgument(nArgument).clone());
			bottomCall->addArgument(call->getArgument(nArgument).clone());
		}
		TypeBase* returnType = &(upSubroutine->getType().getParameter(upSubroutine->getType().getParameterCount() - 1).getType());
		VariableDeclaration* returnValue = &Editing::createNewVariable(*returnType, *outerBlock, "__actual_return");
		ExpressionBase* returnReference = new ReferenceExpression(*returnValue);
		
		int conflictsCount = conflictedVariables.size();
		for(int nConflict = 0; nConflict != conflictsCount; ++nConflict)
		{
			VariableDeclaration* newVar = &Editing::createNewVariable
				(
					conflictedVariables[nConflict]->getType(), 
					*outerBlock, 
					conflictedVariables[nConflict]->getName()
				);
			ExpressionBase* argumentPass = 
				new BasicCallExpression
				(
					BasicCallExpression::BCK_TAKE_ADDRESS,
					new ReferenceExpression(*newVar)
				);
			upCall->addArgument(argumentPass->clone());
			bottomCall->addArgument(argumentPass);
		}
		upCall->addArgument( new BasicCallExpression(BasicCallExpression::BCK_TAKE_ADDRESS, returnReference));
		bottomCall->addArgument( new BasicCallExpression(BasicCallExpression::BCK_TAKE_ADDRESS, returnReference));

		ExpressionBase* firstCallExpression = 
			new BasicCallExpression(
				BasicCallExpression::BCK_ASSIGN,
				new ReferenceExpression(*returnVariable),
				upCall
			);
		StatementBase* firstCall = new ExpressionStatement(firstCallExpression);
		outerBlock->addBefore(outerBlock->convertToIterator(outerStatement), firstCall);
		StatementBase* secondCall = new ExpressionStatement(bottomCall);
		outerBlock->addAfter(outerBlock->convertToIterator(firstCall), secondCall);
		EmptyStatement* guard = new EmptyStatement();
		outerBlock->addAfter(outerBlock->convertToIterator(secondCall), guard);
		guard->setUniqueLabel();
		GotoStatement* emergencyExit = new GotoStatement(guard);
		IfStatement* emergencyCheck = 
			new IfStatement(
				new BasicCallExpression(
					BasicCallExpression::BCK_EQUAL,
					new ReferenceExpression(*returnVariable),
					BasicLiteralExpression::createInteger(1)
				)
			);
		emergencyCheck->getThenBody().addFirst(emergencyExit);
		outerBlock->addBefore(outerBlock->convertToIterator(secondCall), emergencyCheck);
		
		Editing::replaceExpression(*call, ReprisePtr<ExpressionBase>(returnReference));
		result = true;
		return result;
	}

	class ArgumentRemover {
	public:
		bool operator()(VariableDeclaration* variable)
		{
			return variable->hasParameterReference();
		}
	};

	bool splitSubroutine(SubroutineDeclaration* subroutine, BlockStatement::Iterator place, ProgramUnit* unit)
	{
		bool result = false;
		// преобразование не применимо, если в теле функции есть переходы по меткам через границу
		if(hasWrongJumps(subroutine, place))
		{
			throw SubroutineSplitting::SubroutineSplittingException(_TL("Jumps across the border are not allowed","Переходы через границу не поддерживаются"));
		}
		// найдем все переменные, которые нужно вынести наружу
		// любая переменная, вхождение которой является генератором в первой части 
		// и использованием - во второй, должна быть вынесена в аргументы функций
		std::vector<VariableDeclaration*> conflictedVariables;
		findConflictedVariables(conflictedVariables, subroutine, place);
		std::sort(conflictedVariables.begin(), conflictedVariables.end());
		std::vector<VariableDeclaration*>::iterator pConflictsEnd = std::unique(conflictedVariables.begin(), conflictedVariables.end());
		conflictedVariables.erase(pConflictsEnd, conflictedVariables.end());

		SubroutineType* subroutineType = &(subroutine->getType());
		TypeBase* returnType = &(subroutineType->getReturnType());
		bool isSimpleType = returnType->is_a<BasicType>();
		bool hasReturnType = isSimpleType && returnType->cast_to<BasicType>().getKind() != BasicType::BT_VOID || !isSimpleType;

		TypeBase* newReturnType = BasicType::int32Type();
		//SubroutineType* newSubroutineType = new SubroutineType(*subroutineType);
		//newSubroutineType->setReturnType(newReturnType);
		SubroutineType* newSubroutineType = new SubroutineType(newReturnType, SubroutineType::CK_DEFAULT, false, true);
		int oldParametersCount = subroutineType->getParameterCount();
		for(int nOldParameter = 0; nOldParameter < oldParametersCount; ++nOldParameter)
		{
			ParameterDescriptor* oldParameter = &(subroutineType->getParameter(nOldParameter));
			ParameterDescriptor* newParameter = new ParameterDescriptor(oldParameter->getName(), &(oldParameter->getType()));
			newSubroutineType->addParameter(newParameter);
		}
		std::vector<VariableDeclaration*> oldVariables;
		for (Declarations::VarIterator iter = subroutine->getDeclarations().getFirstVar(); iter.isValid(); ++iter)
		{
			oldVariables.push_back(&*iter);
		}
		//удалим из списка переменных все конфликтные переменные
		std::vector<VariableDeclaration*>::iterator pConflictToDelete = conflictedVariables.begin();
		for(; pConflictToDelete != conflictedVariables.end(); ++pConflictToDelete)
		{
			std::remove(oldVariables.begin(), oldVariables.end(), *pConflictToDelete);
		}
		//удалим из списка конфликтов все переменные, уже передаваемые через параметр
		pConflictsEnd =	std::remove_if(conflictedVariables.begin(), conflictedVariables.end(), ArgumentRemover());
		conflictedVariables.erase(pConflictsEnd, conflictedVariables.end());

		std::vector<std::string> newParameters;
		for(size_t nVar = 0; nVar < conflictedVariables.size(); ++nVar)
		{
			ParameterDescriptor* newParameter = 
					new ParameterDescriptor(
					conflictedVariables[nVar]->getName() + "____new",
					new PtrType(&(conflictedVariables[nVar]->getType()))
				);
			newSubroutineType->addParameter(newParameter);
			newParameters.push_back(newParameter->getName());
		}

		std::string returnParameterName = subroutine->getName() + "__return";
		if(hasReturnType) 
		{
			ParameterDescriptor* returnParameter = 
				new ParameterDescriptor
				(
					returnParameterName, 
					new PtrType(returnType)
				); 
			newSubroutineType->addParameter(returnParameter);
		}


		// создадим две результирующие подпрограммы и раздадим им части тела исходной
		std::string upName = subroutine->getName() + "_up";
		std::string bottomName = subroutine->getName() + "_bottom";
		while(subroutine->getDeclarations().findSubroutine(upName)!=NULL) upName += "1";
		while(subroutine->getDeclarations().findSubroutine(bottomName)!=NULL) bottomName += "1";
		SubroutineDeclaration* upSubroutine = new SubroutineDeclaration(newSubroutineType->clone(), upName);
		SubroutineDeclaration* bottomSubroutine = new SubroutineDeclaration(newSubroutineType, bottomName);
		Declarations* outerDeclarations = &(subroutine->getParent()->cast_to<Declarations>());
		Declarations::Iterator pDeclaration = outerDeclarations->getFirst();
		for(; pDeclaration.isValid(); ++pDeclaration)
		{
			if((*pDeclaration).is_a<SubroutineDeclaration>())
			{
				if((*pDeclaration).cast_to<SubroutineDeclaration>().getName() == subroutine->getName()) 
					break;
			}
		}
		OPS_ASSERT(pDeclaration.isValid());
		outerDeclarations->addBefore(pDeclaration, upSubroutine);
		outerDeclarations->addBefore(pDeclaration, bottomSubroutine);

		upSubroutine->setBodyBlock(ReprisePtr<BlockStatement>(new BlockStatement()));
		bottomSubroutine->setBodyBlock(ReprisePtr<BlockStatement>(new BlockStatement()));

		BlockStatement* block = &(subroutine->getBodyBlock());
		BlockStatement::Iterator pSourceStatement = block->getFirst();
		BlockStatement::Iterator pFirstPartEnd = place;
		++pFirstPartEnd; OPS_ASSERT(pFirstPartEnd.isValid());

		for(; pSourceStatement != pFirstPartEnd; ++pSourceStatement)
		{
			upSubroutine->getBodyBlock().addLast(pSourceStatement->clone());
		}
		for(; pSourceStatement.isValid(); ++pSourceStatement)
		{
			bottomSubroutine->getBodyBlock().addLast(pSourceStatement->clone());
		}
		// добавим старые переменные
		addOldVariables(&(upSubroutine->getBodyBlock()), oldVariables);
		addOldVariables(&(bottomSubroutine->getBodyBlock()), oldVariables);
		// добавим необходимые аргументы в обе подпрограммы
		// заменим все обращения к старым переменным на новые
		swapReferences(&(upSubroutine->getBodyBlock()), conflictedVariables, newParameters);
		swapReferences(&(bottomSubroutine->getBodyBlock()), conflictedVariables, newParameters);

		if(hasReturnType)
		{
			replaceReturns(&(upSubroutine->getBodyBlock()), returnParameterName);
			replaceReturns(&(bottomSubroutine->getBodyBlock()), returnParameterName);
		}
		
		// если у исходной подпрограммы есть возвращаемое значение, будем передавать его через аргументы
		// между вызовами двух функций нужно вставить проверку того, нужно ли выполнять вторую
		// если результат уже получен, то не нужно
		Shared::NodesCollector<SubroutineCallExpression> callsCollector;
		unit->accept(callsCollector);
		std::vector<SubroutineCallExpression*>& calls = callsCollector.getCollection();
		std::vector<SubroutineCallExpression*>::iterator pCall = calls.begin();
		for(; pCall != calls.end(); ++pCall)
		{
			if(&((*pCall)->getExplicitSubroutineDeclaration()) == subroutine)
			{
				if(!replaceCall(*pCall, conflictedVariables, upSubroutine, bottomSubroutine)) return false;
			}
			result = true;
		}
		return result;
	}


}

}

}
