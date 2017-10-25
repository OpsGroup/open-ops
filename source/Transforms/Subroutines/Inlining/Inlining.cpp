#include <list>
#include "Transforms/Subroutines/Inlining.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Reprise/Service/DeepWalker.h"
#include "Shared/NodesCollector.h"
#include "Shared/LabelsShared.h"
#include "FrontTransforms/Resolver.h"
#include "Analysis/CallGraph.h"
#include "Shared/ReprisePath.h"
#include "Shared/RepriseClone.h"
#include "OPS_Core/Localization.h"

using namespace OPS;
using namespace Reprise;
using namespace Shared;
using namespace Service;
using namespace TransformationsHub;

namespace OPS {

namespace Transforms {

namespace Subroutines {


	Inlining::Inlining(): TransformBase()
	{
		// Первый параметр - вызов функции
		ArgumentInfo firstArgInfo(ArgumentValue::ExprCall, _TL("Subroutine call, which will be inlined.", "Встраиваемый вызов функции."));
		this->m_argumentsInfo.push_back(firstArgInfo);
	};

    bool Inlining::isApplicable(ProgramUnit *program, const ArgumentValues &params, std::string *message)
    {
        SubroutineCallExpression* pSubroutineCall = params[0].getAsCall();
        SubroutineDeclaration* pDecl = &(pSubroutineCall->getExplicitSubroutineDeclaration());
        if(!pDecl->hasImplementation() && !pDecl->hasDefinition())
        {
            if (message) *message = _TL("Call to an explicitly defined function needed","Требуется вызов функции с телом");
            return false;
        }
        return true;
    }

    void Inlining::makeTransformImpl(ProgramUnit* pProgram, const ArgumentValues& params)
	{
		OPS_ASSERT(pProgram != NULL);
		OPS_ASSERT(getArgumentsInfo().validate(params));

        ////////////////////////
		// Transformation itself
		////////////////////////
        inlineSubstitution(params[0].getAsCall());
	}

	FullInlining::FullInlining(): TransformBase()
	{
		// Первый параметр - вызов функции
		ArgumentInfo firstArgInfo(ArgumentValue::StmtAny, _TL("Target for full substitution", "Участок кода для полной подстановки"));
		this->m_argumentsInfo.push_back(firstArgInfo);
	}

    void FullInlining::makeTransformImpl(ProgramUnit* pProgram, const ArgumentValues& params)
	{
		OPS_ASSERT(pProgram != NULL);
		OPS_ASSERT(getArgumentsInfo().validate(params));

		StatementBase* pStatement = params[0].getAsStatement();

		////////////////////////
		// Transformation itself
		////////////////////////
		fullInlining(pStatement);
	}


	class CallsCollector: public OPS::Reprise::Service::DeepWalker
	{
	public:
		class Call;
	private:
		typedef std::stack<Call*> CurrentCalls;
		CurrentCalls currentCalls;
	public:
		
		typedef std::vector<Call*> Collection;

		class Call {
		public:
			Collection calls;
			SubroutineCallExpression* pCall;
			~Call()
			{
				Collection::iterator pCall = calls.begin();
				for(; pCall != calls.end(); ++pCall)
				{
					delete *pCall;
				}
			}
		};
		
		CallsCollector() {}

		void visit(SubroutineCallExpression& call)
		{
			Call* newCall = new Call;
			newCall->pCall = &call;
			if(currentCalls.empty())
			{
				m_collection.push_back(newCall);
			} else {
				currentCalls.top()->calls.push_back(newCall);
			}
			currentCalls.push(newCall);
			OPS::Reprise::Service::DeepWalker::visit(call);
			currentCalls.pop();
		}

		~CallsCollector()
		{
			Collection::iterator pCall = m_collection.begin();
			for(; pCall != m_collection.end(); ++pCall)
			{
				delete *pCall;
			}
		}

		Collection& getCollection() { return m_collection; }
		const Collection& getCollection() const { return m_collection; }

	private:
		Collection m_collection;
	};

	class InlineWalker: public DeepWalker
	{
	private:
		std::set<SubroutineDeclaration*> visited;
	public:
		using DeepWalker::visit;
		void visit(OPS::Reprise::SubroutineDeclaration& subrDecl)
		{
			DeepWalker::visit(subrDecl);
			if(!subrDecl.hasImplementation())
			{
				if(subrDecl.hasDefinition())
				{
					subrDecl.getDefinition().accept(*this);
				}
			}
		}

		void visit(CallsCollector::Call* pCall)
		{
			CallsCollector::Collection* pParentCalls = &(pCall->calls);
			CallsCollector::Collection::iterator pNextCall = pParentCalls->begin();
			for(; pNextCall != pParentCalls->end(); ++pNextCall)
			{
				visit(*pNextCall);
			}
			pCall->pCall->accept(*this);
		}

		void visit(BlockStatement& block)
		{
			//typedef Shared::NodesCollector<SubroutineCallExpression> CallsCollector;
			//bool success = true;
			//do {
				CallsCollector collector;
				block.accept(collector);
				CallsCollector::Collection::iterator pCall = collector.getCollection().begin();
				for(; pCall != collector.getCollection().end(); ++pCall)
				{
					visit(*pCall);
					//(*pCall)->accept(*this);
				}
			//	CallsCollector::CollectionType collection = collector.getCollection();
			//	bool done = false;
			//	if(!collection.empty())
			//	{
			//		do {		
			//			SubroutineCallExpression& exprCall = *(collection.back());
			//			SubroutineDeclaration* pDeclaration = &(exprCall.getExplicitSubroutineDeclaration());
			//			// проведем полный инлайнинг в вызываемой подпрограмме
			//			if(visited.find(pDeclaration) == visited.end())
			//			{
			//				pDeclaration->accept(*this);
			//				visited.insert(pDeclaration);
			//			}
			//			// затем применим его к ней самой
			//			done = inlineSubstitution(&exprCall);
			//			collection.pop_back();
			//		} while(!collection.empty() && !done);
			//	}
			//	if(collection.empty()) success = false;
			//} while (success);
		}

		void visit(OPS::Reprise::SubroutineCallExpression& exprCall)
		{
			//DeepWalker::visit(exprCall);
			SubroutineDeclaration* pDeclaration = &(exprCall.getExplicitSubroutineDeclaration());
			// проведем полный инлайнинг в вызываемой подпрограмме
			if(visited.find(pDeclaration) == visited.end())
			{
				pDeclaration->accept(*this);
				visited.insert(pDeclaration);
			}
			// затем применим его к ней самой
			bool success = inlineSubstitution(&exprCall);
			OPS_UNUSED(success);
		}
	};


bool canInline(TranslationUnit* unit);

void DFS(const CallGraph::Node* pNode, 
		std::map<const CallGraph::Node*, bool>& visited, 
		std::map<const CallGraph::Node*, int>& nMarks, CallGraph& g)
{
	static int counter = 1;
	if(visited[pNode]) return;
	visited[pNode] = true;
	const CallGraph::SubProcCallMap& pCallMap = pNode->getCalls();
	CallGraph::SubProcCallMap::const_iterator pSubroutine = pCallMap.begin();
	// цикл по дугам, исходящим из текущей вершины
	for(; pSubroutine != pCallMap.end(); ++pSubroutine)
	{
		std::string name = pSubroutine->first->getName();
		CallGraph::NodeMap::const_iterator pSubroutine = g.getGraph().find(name);
		OPS_ASSERT(pSubroutine != g.getGraph().end());
		const CallGraph::Node* pCalledNode = pSubroutine->second;
		if(!visited[pCalledNode])
		{
			DFS(pCalledNode, visited, nMarks, g);
		}
	}
	nMarks[pNode] = counter;
	counter++;
	return;
}

bool hasCycle(const CallGraph::Node* pNode, 
		std::map<const CallGraph::Node*, bool>& visited, 
		std::map<const CallGraph::Node*, int>& nMarks, CallGraph& g)
{
	if(visited[pNode]) return false;
	visited[pNode] = true;
	const CallGraph::SubProcCallMap& pCallMap = pNode->getCalls();
	CallGraph::SubProcCallMap::const_iterator pSubroutine = pCallMap.begin();
	// цикл по дугам, исходящим из текущей вершины
	for(; pSubroutine != pCallMap.end(); ++pSubroutine)
	{
		std::string name = pSubroutine->first->getName();
		CallGraph::NodeMap::const_iterator pSubroutine = g.getGraph().find(name);
		OPS_ASSERT(pSubroutine != g.getGraph().end());
		const CallGraph::Node* pCalledNode = pSubroutine->second;
		if(!visited[pCalledNode])
		{
			if(hasCycle(pCalledNode, visited, nMarks, g)) return true;
		} else {
			if(nMarks[pNode] < nMarks[pCalledNode]) return true;
		}
	}
	return false;
}

bool canInline(TranslationUnit* unit)
{
	Resolver resolv; 
	ProgramUnit* pUnit = unit->getParent()->cast_ptr<ProgramUnit>();
	OPS_ASSERT(pUnit != NULL);
 	resolv.setProgram(*pUnit); 
 	resolv.resolve(); 

	// проверим, что в графе вызовов нет циклов
	CallGraph g(pUnit);
	std::map<const CallGraph::Node*, bool> visited;
	std::map<const CallGraph::Node*, int> marks;
	const CallGraph::NodeMap& pNodes = g.getGraph();
	bool isCycled = false;
	CallGraph::NodeMap::const_iterator pSubroutine = pNodes.begin();
	for(; pSubroutine != pNodes.end(); ++pSubroutine)
	{
		const CallGraph::Node* pNode = pSubroutine->second;
		if(!visited[pNode]) DFS(pNode, visited, marks, g);
	}
	pSubroutine = pNodes.begin();
	visited.clear();
	for(; pSubroutine != pNodes.end(); ++pSubroutine)
	{
		const CallGraph::Node* pNode = pSubroutine->second;
		if(!visited[pNode]) isCycled = hasCycle(pNode, visited, marks, g);
		if(isCycled) break;
	}
	if(isCycled) return false;
	else return true;
}

bool fullInlining(TranslationUnit* unit)
{
	if(!canInline(unit))
	{
		throw Inlining::InliningException(_TL("Recursion is not supported","Рекурсия не поддерживается"));
	}

	// рекурсивно пройдем все подпрограммы, применяя инлайнинг к каждому встретившемуся вызову 
	InlineWalker w;
	unit->accept(w);
	return true;
}


bool fullInlining(StatementBase* stmt)
{
	BlockStatement* rootBlock = &(stmt->getRootBlock());
	RepriseBase* parent = rootBlock->getParent();
	if(!parent->is_a<SubroutineDeclaration>()) 
	{
		throw Inlining::InliningException(_TL("Can't find translation unit","Не удается найти единицу трансляции"));
	}
	SubroutineDeclaration* parentSubroutine = &(parent->cast_to<SubroutineDeclaration>());
	Declarations* parentDecls = parentSubroutine->getParent()->cast_ptr<Declarations>();
	OPS_ASSERT(parentDecls);

	if(!parentDecls->getParent()->is_a<TranslationUnit>())
	{
		throw Inlining::InliningException(_TL("Can't find translation unit","Не удается найти единицу трансляции"));	
	}

	TranslationUnit* unit = parentDecls->getParent()->cast_ptr<TranslationUnit>();

	if(!canInline(unit)) 
	{
		throw Inlining::InliningException(_TL("Recursion is not supported","Рекурсия не поддерживается"));
	}

	InlineWalker w;
	stmt->accept(w);
	return true;
}

void prepareArgument(const SubroutineDeclaration* subroutineDeclaration, 
					 BlockStatement* inlinedBody, 
					 const SubroutineType* pType,
					 int nArgument, ExpressionBase* argumentValue, 
					 std::map<const VariableDeclaration*, VariableDeclaration*> variableMap,/*, std::list<VariableDeclaration*> pendingVariables*/
					 std::list<VariableDeclaration*>* argumentVars);

void removeReturn(ReturnStatement* returnStatement, bool hasReturnValue, std::string& lastLabel, StatementBase* lastStatement, VariableDeclaration* returnedValue);

void renewVariables(BlockStatement* pBlock, std::map<const VariableDeclaration*, VariableDeclaration*> variablesMap, std::map<VariableDeclaration*, Shared::ReprisePath> variablesLocations);

bool inlineSubstitution(SubroutineCallExpression* pCall, std::list<VariableDeclaration*>* pAssignmentVariables)
{
	bool result = false;
	const SubroutineDeclaration* pDeclaration = &(pCall->getCallExpression().cast_to<SubroutineReferenceExpression>().getReference());
	
	if(!(pDeclaration->hasImplementation())) 
	{
		if(!(pDeclaration->hasDefinition()))	
		{
			//throw Inlining::InliningException(_TL("Can't find subroutine body","Не удается найти тело подпрограммы"));
			return false;
		} else  {
			pDeclaration = &(pDeclaration->getDefinition());
			if(!(pDeclaration->hasImplementation()))
			{
				//throw Inlining::InliningException(_TL("Can't find subroutine body","Не удается найти тело подпрограммы"));
				return false;
			}
		}
	}

	const SubroutineType* pType = &(pDeclaration->getType());
	const TypeBase* pReturnType = &(pType->getReturnType());
	bool hasReturnValue = ((pReturnType->is_a<BasicType>() && 
		pReturnType->cast_to<BasicType>().getKind() != BasicType::BT_VOID))||(!(pReturnType->is_a<BasicType>()));

	if(!(pCall->obtainParentStatement()->hasParentBlock())) 
	{
		throw Inlining::InliningException(_TL("Can't find parent block","Не удается найти окружающий составной оператор"));
	}

	StatementBase* pCallStatement = pCall->obtainParentStatement();
		
	ExpressionBase* pCallExpression = &(pCall->obtainRootExpression());

	//// не рассматриваем процедуры
	//if(!(pCall->getParent() == pCallStatement) && !hasReturnValue) return false; 

	BlockStatement* inlinedBody = NULL;
	BlockStatement* outerBlock = NULL;
	if(pCallStatement->is_a<IfStatement>()     ||
	   pCallStatement->is_a<PlainSwitchStatement>() ||
	   pCallStatement->is_a<ExpressionStatement>() ||
	   pCallStatement->is_a<ReturnStatement>())
	{
		outerBlock = &(pCallStatement->getParentBlock());
		BlockStatement::Iterator pCallInBlock = outerBlock->convertToIterator(pCallStatement);
		inlinedBody = pDeclaration->getBodyBlock().clone();
		outerBlock->addBefore(pCallInBlock, inlinedBody);
	} else {
		// TODO: в случае циклов нужно позаботиться о том, 
		// чтобы новая переменная была видна в заголовке,
		// а ее значение вовремя изменялось
		// то есть, тело встраиваемой функции должно появляться после 
		// всех операторов цикла (если вызов функции находился не в выражении инициализации)
		// и должно гарантированно выполняться
		// то есть, все continue должны быть заменены на goto до проведения преобразования
		// кроме того, в вызове функции может использоваться счетчик цикла
		// на всякий случай вынесем выражение инициализации перед циклом
		if(pCallStatement->is_a<ForStatement>())
		{
			ForStatement* pFor = &(pCallStatement->cast_to<ForStatement>());

			if(pCallExpression == &(pFor->getFinalExpression()) ||
			   pCallExpression == &(pFor->getStepExpression()))
			{
				outerBlock = &(pFor->getBody());
				inlinedBody = pDeclaration->getBodyBlock().clone();
				outerBlock->addLast(inlinedBody);
				outerBlock = &(pFor->getParentBlock());
			} else {
				if(pCallExpression == &(pFor->getInitExpression()))
				{
					outerBlock = &(pCallStatement->getParentBlock());
					inlinedBody = pDeclaration->getBodyBlock().clone();
					BlockStatement::Iterator pCallInBlock = outerBlock->convertToIterator(pCallStatement);
					outerBlock->addBefore(pCallInBlock, inlinedBody);
				}
			}
		} else {
			if(pCallStatement->is_a<WhileStatement>())
			{
				WhileStatement* pWhile = &(pCallStatement->cast_to<WhileStatement>());
				outerBlock = &(pWhile->getBody());
				inlinedBody = pDeclaration->getBodyBlock().clone();
				outerBlock->addLast(inlinedBody);
				outerBlock = &(pWhile->getParentBlock());
			}
		}
	}
	if(inlinedBody == NULL) return false;

	OPS::Shared::doPostCloneLinkFix(pDeclaration->getBodyBlock(), *inlinedBody);
	
	// не ясно, нужно ли это тут
	if(isPossibleToGenerateNewLabels(*inlinedBody))
		generateNewLabels(*inlinedBody);
	const Declarations* subroutineDeclarations = &(pDeclaration->getDeclarations());
	//// сюда сохраним переменные, инициализаторы которых тоже нужно обойти
	//std::list<VariableDeclaration*> pendingVariables;
	
	std::map<const VariableDeclaration*, BlockStatement*> variableLocationMap;
	std::map<VariableDeclaration*, Shared::ReprisePath> variablePathMap;
	const BlockStatement* pathRoot = &(pDeclaration->getBodyBlock());
	for (Declarations::ConstVarIterator iter = subroutineDeclarations->getFirstVar(); iter.isValid(); ++iter)
	{
		const VariableDeclaration* pVar = &*iter;
		if(pVar->hasDefinedBlock())
		{
			ReprisePath path = Shared::makePath(pVar->getDefinedBlock(), pathRoot);
			BlockStatement* pDefinedBlock = Shared::findByPath(*inlinedBody, path)->cast_ptr<BlockStatement>();
			variableLocationMap[pVar] = pDefinedBlock;
		} else {
			variableLocationMap[pVar] = inlinedBody;
		}

	}

	std::map<const VariableDeclaration*, VariableDeclaration*> variableMap;
	std::vector<BlockStatement*> DestinationBlocks;
	std::vector<ExpressionBase*> Replacements;
	std::vector< ReprisePtr<ExpressionBase> > Targets;
	std::vector<VariableDeclaration*> PendingInitializers;
	for (Declarations::ConstVarIterator iter = subroutineDeclarations->getFirstVar(); iter.isValid(); ++iter)
	{
		const VariableDeclaration* pSourceVariable = &*iter;
		VariableDeclaration* pNewVariable = &Editing::createNewVariable(const_cast<TypeBase&>(pSourceVariable->getType()), *inlinedBody, pSourceVariable->getName());
		pNewVariable->declarators() = pSourceVariable->getDeclarators();
		
		BlockStatement* pDefinedBlock = variableLocationMap[pSourceVariable];
		/*if(pDefinedBlock != inlinedBody)
		{*/
		pNewVariable->setDefinedBlock(*pDefinedBlock);
		//}
		variableMap[pSourceVariable] = pNewVariable;

		variablePathMap[pNewVariable] = Shared::makePath(*pDefinedBlock, inlinedBody);

		ReprisePtr<ReferenceExpression> pNewVariableReference(new ReferenceExpression(*pNewVariable));
		ReprisePtr<ReferenceExpression> pOldVariableReference(new ReferenceExpression(const_cast<VariableDeclaration&>(*pSourceVariable)));

		if(pSourceVariable->hasNonEmptyInitExpression()) 
		{
			// инициализатор преобразуется в присваивание, если он содержит ссылки на переменные, видимые в блоке
			bool needCutInitializer = false;
			Shared::NodesCollector<ReferenceExpression> refCollector;
			const_cast<VariableDeclaration*>(pSourceVariable)->getInitExpression().accept(refCollector);
			Shared::NodesCollector<ReferenceExpression>::CollectionType::iterator pRef = refCollector.getCollection().begin();
			for(; pRef != refCollector.getCollection().end(); ++pRef)
			{
				if(variableLocationMap.find(&((*pRef)->getReference())) != variableLocationMap.end())
				{
					needCutInitializer = true;
					break;
				}
			}
			if(pSourceVariable->getType().isConst())
			{
				needCutInitializer = false;
			}
			
			if(!needCutInitializer)
			{
				pNewVariable->setInitExpression(*(pSourceVariable->getInitExpression().clone()));
				PendingInitializers.push_back(pNewVariable);
			} else {
					BasicCallExpression* newAssignment =
						new BasicCallExpression
						(
							BasicCallExpression::BCK_ASSIGN, 
							pNewVariableReference->clone(), 
							pSourceVariable->getInitExpression().clone()
						);
					pDefinedBlock->addFirst(new ExpressionStatement(newAssignment));
					//pendingVariables.push_back(pNewVariable)
			}
		}
		
		//OPS::Transforms::Scalar::makeSubstitutionForward(*pDefinedBlock, *pOldVariableReference, pNewVariableReference, true);	
		// замену нужно делать только в том блоке, в котором видна переменная
		DestinationBlocks.push_back(pDefinedBlock);
		Replacements.push_back(&(*pOldVariableReference));
		Targets.push_back(pNewVariableReference);

		pOldVariableReference.release();
	}
	// сохраним все аргументы
	int argumentNumber = pType->getParameterCount();
	
	for(int nArgument = argumentNumber - 1; nArgument >= 0; --nArgument)
	{
		prepareArgument(pDeclaration, inlinedBody, pType, nArgument, &(pCall->getArgument(nArgument)), variableMap, pAssignmentVariables/*, pendingVariables*/);
	}

	for(size_t nReplace = 0; nReplace < Replacements.size(); nReplace++)
	{
		OPS::Transforms::Scalar::makeSubstitutionForward(
			*(DestinationBlocks[nReplace]),
			*(Replacements[nReplace]),
			Targets[nReplace], true);	
		for(size_t nInitializer = 0; nInitializer < PendingInitializers.size(); nInitializer++)
		{
			if(PendingInitializers[nInitializer]->hasNonEmptyInitExpression())
			{
				OPS::Transforms::Scalar::makeSubstitutionForward(
				PendingInitializers[nInitializer]->getInitExpression(),
				*(Replacements[nReplace]),
				Targets[nReplace], true);
			}
		}
		delete Replacements[nReplace];
	}
	Replacements.clear();
	
	VariableDeclaration* returnedValue = NULL;
	if(hasReturnValue) 
	{
		returnedValue = &Editing::createNewVariable(const_cast<TypeBase&>(pType->getReturnType()),*outerBlock);
	}
	
	inlinedBody->addLast(new EmptyStatement());
	std::string lastLabel = inlinedBody->getLast()->setUniqueLabel();
	StatementBase* lastStatement = &(*(inlinedBody->getLast()));
	// найдем все операторы return в теле функции
	std::vector<ReturnStatement*> returns = collectNodes<ReturnStatement>(*inlinedBody);
	// и заменим их на присваивание возвращаемого результата (если нужно) и переход к концу тела
	for(size_t nReturn = 0; nReturn < returns.size(); ++nReturn)
	{
		removeReturn(returns[nReturn], hasReturnValue, lastLabel, lastStatement, returnedValue);
	}

	if(pCallStatement->is_a<ForStatement>())
	{
		ForStatement* pFor = &(pCallStatement->cast_to<ForStatement>());

		if(pCallExpression == &(pFor->getFinalExpression()))
		{
			BlockStatement* anotherBlock = inlinedBody->clone();
			BlockStatement* parentBlock = &(pFor->getParentBlock());
			BlockStatement::Iterator forInBlock = parentBlock->convertToIterator(pFor);

			StatementBase* pStartStatement = new ExpressionStatement(pFor->getInitExpression().clone());
			pFor->setInitExpression(new EmptyExpression());
			parentBlock->addBefore(forInBlock, pStartStatement);
			forInBlock = parentBlock->convertToIterator(pFor);

			parentBlock->addBefore(forInBlock, anotherBlock);
			renewVariables(anotherBlock, variableMap, variablePathMap);
		}
	}
	if(pCallStatement->is_a<WhileStatement>())
	{
		WhileStatement* pWhile = &(pCallStatement->cast_to<WhileStatement>());
		if(pWhile->isPreCondition())
		{
			WhileStatement* pWhile = &(pCallStatement->cast_to<WhileStatement>());
			BlockStatement* anotherBlock = inlinedBody->clone();
			BlockStatement* parentBlock = &(pWhile->getParentBlock());
			BlockStatement::Iterator whileInBlock = parentBlock->convertToIterator(pWhile);
			parentBlock->addBefore(whileInBlock, anotherBlock);
			renewVariables(anotherBlock, variableMap, variablePathMap);
			//pWhile->getBody().addLast(anotherBlock);
		}
	}

	if(pCall->getParent() == pCallStatement && pCallStatement->is_a<ExpressionStatement>()) 
	{
		// оператор с вызовом функции больше не нужен
		outerBlock->erase(outerBlock->convertToIterator(pCallStatement));
		result = true;
	} else {
		if(hasReturnValue)
		{
			// нужно заменить вызов функции с аргументами на обращение к переменной,
			// содержащей возвращенный функцией результат
			ReprisePtr<ExpressionBase> returnRef(new ReferenceExpression(*returnedValue));
			OPS::Transforms::Scalar::makeSubstitutionForward(*pCallExpression, *pCall, returnRef, true);	
			result = true;
		} else {
			//// просто удалим вызов функции
			//EmptyExpression* empty = new EmptyExpression();
			//OPS_ASSERT(pCall->getParent()->is_a<ExpressionBase>());
			//ExpressionBase* pOuterExpression = pCall->getParent()->cast_to<ExpressionBase>();
			result = false;
		}

	}
	return result;
}

void prepareArgument(const SubroutineDeclaration* subroutineDeclaration, 
					 BlockStatement* inlinedBody, 
					 const SubroutineType* pType,
					 int nArgument, ExpressionBase* argumentValue, 
					 std::map<const VariableDeclaration*, VariableDeclaration*> variableMap,/*, std::list<VariableDeclaration*> pendingVariables*/
					 std::list<VariableDeclaration*>* argumentVars)
{
	const ParameterDescriptor* argument = &(pType->getParameter(nArgument));

	//VariableDeclaration* pTempVariable = &Editing::createNewVariable(const_cast<TypeBase&>(argument->getType()), *inlinedBody, argument->getName());
	//ReferenceExpression* pTempVariableRef = new ReferenceExpression(*pTempVariable);

	const Declarations* subroutineDeclarations = &(subroutineDeclaration->getDeclarations());
	const VariableDeclaration* argumentVariable = const_cast<Declarations*>(subroutineDeclarations)->findVariable(argument->getName());
	OPS_ASSERT(argumentVariable != NULL);
	VariableDeclaration* newArgumentVariable = variableMap[argumentVariable];
	OPS_ASSERT(newArgumentVariable != NULL);
	//ReprisePtr<ExpressionBase> pRef(new ReferenceExpression(*newArgumentVariable));
	//BasicCallExpression* pArgumentAssignment = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, pTempVariableRef, argumentValue->clone()); 
	//inlinedBody->addFirst(new ExpressionStatement(pArgumentAssignment));
	newArgumentVariable->setInitExpression(*(argumentValue->clone()));
	if(argumentVars != NULL) {
		argumentVars->push_back(newArgumentVariable);
	}
	
	/*std::list<VariableDeclaration*>::iterator pVariable = pendingVariables.begin();
	for(; pVariable != pendingVariables.end(); ++pVariable)
	{
		OPS_ASSERT((*pVariable)->hasNonEmptyInitExpression());
		OPS::Transforms::Scalar::makeSubstitutionForward((*pVariable)->getInitExpression(), *pRef, ReprisePtr<ReferenceExpression>(pTempVariableRef));	
	}*/
	
	//OPS::Transforms::Scalar::makeSubstitutionForward(*inlinedBody, *pRef, ReprisePtr<ReferenceExpression>(pTempVariableRef), true);	
}

void removeReturn(ReturnStatement* returnStatement, bool hasReturnValue, std::string& lastLabel, StatementBase* lastStatement, VariableDeclaration* returnedValue)
{
	OPS_ASSERT(returnStatement->hasParentBlock());
	BlockStatement* outerBlock = &(returnStatement->getParentBlock());
	if(hasReturnValue)
	{
		ReferenceExpression* returnReference = new ReferenceExpression(*returnedValue);
		BasicCallExpression* returnAssignment = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, returnReference, returnStatement->getReturnExpression().clone());
		outerBlock->addBefore(outerBlock->convertToIterator(returnStatement), new ExpressionStatement(returnAssignment));
	}
	GotoStatement* gotoEnd = new GotoStatement(lastStatement);
	outerBlock->addAfter(outerBlock->convertToIterator(returnStatement), gotoEnd);
	outerBlock->erase(outerBlock->convertToIterator(returnStatement));
	
}

void renewVariables(BlockStatement* pBlock,
					std::map<const VariableDeclaration*, VariableDeclaration*> variablesMap,
					std::map<VariableDeclaration*, Shared::ReprisePath> variablesLocations)
{
	std::map<const VariableDeclaration*, VariableDeclaration*>::iterator pVar = variablesMap.begin();
	std::vector<BlockStatement*> DestinationBlocks;
	std::vector<ExpressionBase*> Replacements;
	std::vector< ReprisePtr<ExpressionBase> > Targets;
	std::vector<VariableDeclaration*> PendingInitializers;
	for (; pVar != variablesMap.end(); ++pVar)
	{
		VariableDeclaration* pSourceVariable = pVar->second;

		VariableDeclaration* pNewVariable = &Editing::createNewVariable(pSourceVariable->getType(), *pBlock, pSourceVariable->getName());
		pNewVariable->declarators() = pSourceVariable->getDeclarators();

		if(pSourceVariable->hasNonEmptyInitExpression())
		{
			pNewVariable->setInitExpression(*(pSourceVariable->getInitExpression().clone()));
			PendingInitializers.push_back(pNewVariable);
		}
		
		BlockStatement* pDefinedBlock = pBlock;
		Shared::ReprisePath path = variablesLocations[pVar->second];
		if(!path.empty())
		{
			pDefinedBlock = Shared::findByPath(*pBlock, path)->cast_ptr<BlockStatement>();
			OPS_ASSERT(pDefinedBlock != NULL);
		}
		pNewVariable->setDefinedBlock(*pDefinedBlock);

		ReprisePtr<ReferenceExpression> pNewVariableReference(new ReferenceExpression(*pNewVariable));
		ReprisePtr<ReferenceExpression> pOldVariableReference(new ReferenceExpression(*pSourceVariable));
	
		//OPS::Transforms::Scalar::makeSubstitutionForward(*pDefinedBlock, *pOldVariableReference, pNewVariableReference, true);		
		DestinationBlocks.push_back(pDefinedBlock);
		Replacements.push_back(&(*pOldVariableReference));
		Targets.push_back(pNewVariableReference);

		pOldVariableReference.release();
	}
	for(size_t nReplace = 0; nReplace < Replacements.size(); nReplace++)
	{
		OPS::Transforms::Scalar::makeSubstitutionForward(
			*(DestinationBlocks[nReplace]),
			*(Replacements[nReplace]),
			Targets[nReplace], true);	
		for(size_t nInitializer = 0; nInitializer < PendingInitializers.size(); nInitializer++)
		{
			if(PendingInitializers[nInitializer]->hasNonEmptyInitExpression())
			{
				OPS::Transforms::Scalar::makeSubstitutionForward(
				PendingInitializers[nInitializer]->getInitExpression(),
				*(Replacements[nReplace]),
				Targets[nReplace], true);	
			}
		}
		delete Replacements[nReplace];
	}

	if(isPossibleToGenerateNewLabels(*pBlock))
		generateNewLabels(*pBlock);

}

}
}
}

