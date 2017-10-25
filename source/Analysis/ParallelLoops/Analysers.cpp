#include "Analysers.h"
#include "Shared/Checks.h"
#include "Analysis/ExpressionOrder.h"
#include "Privatization.h"

namespace OPS
{
namespace Analysis
{
namespace ParallelLoops
{

bool contains(const StatementBase& parent, const StatementBase& child)
{
	const StatementBase* node = &child;

	while(node)
	{
		if (node->getParent() == &parent)
		{
			return true;
		}

		node = node->getParent()->cast_ptr<const StatementBase>();
	}

	return false;
}

class BlockPred
{
public:
	BlockPred(BlockStatement& block):m_block(block) {}
	bool operator()(AbstractDependence& dep) const
	{
		return contains(m_block, *dep.getBeginOccurrence()->getStatement()) &&
			   contains(m_block, *dep.getEndOccurrence()->getStatement());
	}
	BlockStatement& m_block;
};

LoopAnalyserBase::LoopAnalyserBase(Target target, AbstractDepGraph &depGraph)
	:m_target(target)
	,m_depGraph(depGraph)
{
}

std::tr1::shared_ptr<TargetParallelInfo> LoopAnalyserBase::analyse(const LoopParallelInfo &loop)
{
	std::tr1::shared_ptr<TargetParallelInfo> info(new TargetParallelInfo(m_target));

	bool basicCheckFailed = false;

	if (!loop.isBasicBlock())
	{
		info->getObstacles().push_back(ParallelObstacleBasePtr(new BadControlFlowObstacle));
		basicCheckFailed = true;
	}

	if (loop.containsDirtyCalls())
	{
		info->getObstacles().push_back(ParallelObstacleBasePtr(new DirtyCallsObstacle(loop, loop.getDirtyCalls())));
		basicCheckFailed = true;
	}

	OPS_UNUSED(basicCheckFailed);

/*	if (basicCheckFailed)
	{
		return info;
	}
*/
	analyseImpl(loop, *info);

	return info;
}

MISMAnalyser::MISMAnalyser(AbstractDepGraph &depGraph)
	:LoopAnalyserBase(MISM, depGraph)
{
}

void MISMAnalyser::analyseImpl(const LoopParallelInfo &loop, TargetParallelInfo &targetInfo)
{
	// построить подграф графа зависимостей
	std::unique_ptr<AbstractDepGraph> subGraph( m_depGraph.getSubGraphByCarrier(loop.getStatement()) );
	subGraph.reset(subGraph->getSubGraphByPred(BlockPred(loop.getStatement().getBody())));

	// сделать приватизацию
	makePrivatization(loop, *subGraph, targetInfo);

	// определить редукцию
	recognizeReduction(loop, *subGraph, targetInfo);

	// проверить чтобы на подграфе были только дуги входной зависимости
	DependenceList::iterator it = subGraph->begin();
	for(; it != subGraph->end(); ++it)
	{
		if ((*it)->getType() != AbstractDependence::Input)
		{
			targetInfo.getObstacles().push_back(ParallelObstacleBasePtr(new CarriedDependenceObstacle(*it)));
		}
	}
}

struct ScalarSelfOutDepPred
{
	bool operator()(AbstractDependence& dep)
	{
		return  dep.getType() == AbstractDependence::Output && // выходная зависимость
				dep.getBeginOccurrence() == dep.getEndOccurrence() && // самозависимость
				dep.isScalarVariable(); // скалярная переменная
	}
};

struct DepFromSourcePed
{
	DepFromSourcePed(AbstractDependence::Type type, ExpressionBase* expr)
		:m_type(type),m_srcOccur(expr) {}
	bool operator()(AbstractDependence& dep)
	{
		return dep.getType() == m_type &&
			   dep.getBeginExpr() == m_srcOccur;
	}
	AbstractDependence::Type m_type;
	const ExpressionBase* m_srcOccur;
};

bool getReductionOperation(StatementBase* statement, Reprise::BasicCallExpression::BuiltinCallKind& op)
{
	if (ExpressionStatement* exprStmt = statement->cast_ptr<ExpressionStatement>())
	{
		if (BasicCallExpression* assignExpr = exprStmt->get().cast_ptr<BasicCallExpression>())
		{
			if (assignExpr->getKind() == BasicCallExpression::BCK_ASSIGN)
			{
				if (ReferenceExpression* refExpr = assignExpr->getArgument(0).cast_ptr<ReferenceExpression>())
				{
					if (BasicCallExpression* opExpr = assignExpr->getArgument(1).cast_ptr<BasicCallExpression>())
					{
						if (ReferenceExpression* ref2Expr = opExpr->getArgument(0).cast_ptr<ReferenceExpression>())
						{
							if (&refExpr->getReference() == &ref2Expr->getReference())
							{
								op = opExpr->getKind();
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

void MISMAnalyser::recognizeReduction(const LoopParallelInfo &loop, AbstractDepGraph &subGraph, TargetParallelInfo &targetInfo)
{
	// ищем выходную самозависимость по скалярной переменной
	std::unique_ptr<AbstractDepGraph> selfOutDeps( subGraph.getSubGraphByPred(ScalarSelfOutDepPred()) );

	if (!selfOutDeps->isEmpty())
	{
		// берем только те дуги, которые внтури цикла
		std::unique_ptr<AbstractDepGraph> loopDeps( m_depGraph.getSubGraphByLoopBody(loop.getStatement()) );

		for(DependenceList::iterator it = selfOutDeps->begin(); it != selfOutDeps->end(); ++it)
		{
			std::unique_ptr<AbstractDepGraph> flowDeps( loopDeps->getSubGraphByPred(DepFromSourcePed(AbstractDependence::Flow,(*it)->getBeginExpr())));

			if (flowDeps->getDependenceCount() == 1 && // должна быть только одна
				(*flowDeps->begin())->isSelfStatementDependence() ) // и должна быть в том-же операторе
			{
				// Может еще быть антидуга, нужно её найти, чтобы потом удалить
				std::unique_ptr<AbstractDepGraph> antiDeps(loopDeps->getSubGraphByPred(
							   DepFromSourcePed(AbstractDependence::Anti, (*flowDeps->begin())->getEndExpr())));
				BasicCallExpression::BuiltinCallKind operation;
				if (getReductionOperation((*it)->getBeginStatement(), operation))
				{
					ReferenceExpression* refExpr = (*it)->getBeginExpr()->cast_ptr<ReferenceExpression>();
					VariableDeclaration& var = refExpr->getReference();
					targetInfo.getConditions().push_back(ParallelConditionBasePtr(new ReductionCondition(var, operation)));

					// удаляем дуги, которые должны пропасть после редукции
					subGraph.remove(*it);
					subGraph.remove(*flowDeps->begin());
					if (!antiDeps->isEmpty())
						subGraph.remove(*antiDeps->begin());
				}
			}
		}
	}
}

void MISMAnalyser::makePrivatization(const LoopParallelInfo &loop, AbstractDepGraph &subGraph, TargetParallelInfo &targetInfo)
{
	PrivatizationHelper helper(loop.getStatement().getBody(), subGraph);
	std::set<const VariableDeclaration*> privatizableVars = helper.analyse();

	if (!privatizableVars.empty())
	{
		std::list<const VariableDeclaration*> vars(privatizableVars.begin(), privatizableVars.end());
		targetInfo.getConditions().push_back(ParallelConditionBasePtr(new PrivatizationCondition(vars)));
		helper.removeDependenciesByVariables(subGraph, privatizableVars);
	}
}

MIDMAnalyser::MIDMAnalyser(AbstractDepGraph &depGraph)
	:LoopAnalyserBase(MIDM, depGraph)
{
}

void MIDMAnalyser::analyseImpl(const LoopParallelInfo &loop, TargetParallelInfo &targetInfo)
{
	std::unique_ptr<AbstractDepGraph> subGraph( m_depGraph.getSubGraphByCarrier(loop.getStatement()) );
	subGraph.reset(subGraph->getSubGraphByPred(BlockPred(loop.getStatement().getBody())));

	if (!subGraph->isEmpty())
	{
		for(DependenceList::iterator it = subGraph->begin(); it != subGraph->end(); ++it)
		{
			const AbstractDependence& dep = **it;

			if (dep.getType() != AbstractDependence::Flow &&
				dep.getType() != AbstractDependence::Output)
				targetInfo.getObstacles().push_back(ParallelObstacleBasePtr(new CarriedDependenceObstacle(*it)));
		}

		if (targetInfo.getObstacles().empty())
			targetInfo.getConditions().push_back(ParallelConditionBasePtr(new PreliminarySendCondition));
	}
}

SIMDAnalyser::SIMDAnalyser(AbstractDepGraph &depGraph, MemoryKind memKind)
	:LoopAnalyserBase((memKind == SharedMemory) ? SISM : SIDM, depGraph)
	,m_memKind(memKind)
{
}

void SIMDAnalyser::analyseImpl(const LoopParallelInfo &loop, TargetParallelInfo &targetInfo)
{
	/*
		TODO:
			Могут быть вызовы функций с такими же операторами
	*/

	// Цикл должен быть простым
	if (Editing::forIsBasic(loop.getStatement()))
	{
		// Границы цикла должны быть известны на момент компиляции
		if (!Editing::getBasicForInitExpression(loop.getStatement()).is_a<StrictLiteralExpression>() ||
			!Editing::getBasicForFinalExpression(loop.getStatement()).is_a<StrictLiteralExpression>())
		{
			targetInfo.getConditions().push_back(ParallelConditionBasePtr(new StripMiningCondition));
			//targetInfo.getObstacles().push_back(ParallelObstacleBasePtr(new NonConstantBoundsObstacle));
		}
	}
	else
	{
		targetInfo.getObstacles().push_back(ParallelObstacleBasePtr(new NonBasicForObstacle));
	}

	// В теле цикла могуть быть только операторы присваивания и циклы DO без выходов и гоуту
	OPS::Shared::Checks::CompositionCheckObjects restrictions;
	restrictions << OPS::Shared::Checks::CompositionCheckObjects::CCOT_BlockStatement;
	restrictions << OPS::Shared::Checks::CompositionCheckObjects::CCOT_EmptyStatement;
	restrictions << OPS::Shared::Checks::CompositionCheckObjects::CCOT_ExpressionStatement;
	restrictions << OPS::Shared::Checks::CompositionCheckObjects::CCOT_ForStatement;
	restrictions << OPS::Shared::Checks::CompositionCheckObjects::CCOT_Label;

	if (!OPS::Shared::Checks::makeCompositionCheck(loop.getStatement(), restrictions))
	{
		targetInfo.getObstacles().push_back(ParallelObstacleBasePtr(new ForbiddenStatementsObstacle));
	}

	// отделить дуги, которые внутри цикла
	std::unique_ptr<AbstractDepGraph> subGraph( m_depGraph.getSubGraphByCarrier(loop.getStatement()) );
	subGraph.reset(subGraph->getSubGraphByPred(BlockPred(loop.getStatement().getBody())));

	MISMAnalyser::makePrivatization(loop, *subGraph, targetInfo);

	// Не должно быть дуг снизу вверх
	DependenceList::iterator it = subGraph->begin(),
							end = subGraph->end();

	ControlFlowGraph cfg(loop.getStatement().getBody());

	for(; it != end; ++it)
	{
		const AbstractDependence& dep = **it;

		// проверить, что дуга идет снизу вверх
		bool isDownUpDependence = false;

		// Дуга не должна вести из/в заголовка цикла
		if (dep.getBeginStatement() != &loop.getStatement() &&
			dep.getEndStatement() != &loop.getStatement())
		{
			if (dep.getBeginStatement() == dep.getEndStatement())
			{
				// определим порядок вычисления выражений
				int order = getExpressionOrder(*dep.getEndExpr(), *dep.getBeginExpr());
				if (order > 0)
					isDownUpDependence = true;
				else if (order == 0)
					isDownUpDependence = (dep.getType() == AbstractDependence::Flow ||
										  dep.getType() == AbstractDependence::Output);
			}
			else
				isDownUpDependence = cfg.hasPath(dep.getEndStatement(), dep.getBeginStatement());
		}
		else
			isDownUpDependence = true; // непонятно какая дуга

		if (isDownUpDependence && dep.getType() != AbstractDependence::Input)
		{
			targetInfo.getObstacles().push_back(ParallelObstacleBasePtr(new BackwardDependenceObstacle(*it)));
		}
		// для распределенной памяти: не должно быть входных дуг зависимости
		else if (m_memKind == DistributedMemory && dep.getType() == AbstractDependence::Input)
		{
			targetInfo.getObstacles().push_back(ParallelObstacleBasePtr(new CarriedDependenceObstacle(*it)));
		}
	}
}

}
}
}
