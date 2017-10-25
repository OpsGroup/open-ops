#ifndef PARALLELLOOPS_H
#define PARALLELLOOPS_H

#include "Reprise/Statements.h"
#include "Reprise/Expressions.h"
#include "OPS_Core/MemoryHelper.h"
#include "Shared/ProgramContext.h"
#include "Analysis/AbstractDepGraph.h"
#include "OPS_Stage/Passes.h"

class ControlFlowGraph;

namespace OPS
{
namespace Analysis
{
namespace ParallelLoops
{

enum Target
{
	SISM = 1, // Single Instruction stream, Shared Memory
	SIDM = 2, // Single Instruction stream, Distributed Memory
	MISM = 4, // Multiple Instruction stream, Shared Memory
	MIDM = 8, // Multiple Instruction stream, Distributed Memory
	// PIPELINE = 16, // Pipeline architecture
};

class LoopParallelInfo;

/// Базовый класс для всех условий распараллеливания циклов
class ParallelConditionBase
{
public:

	virtual std::string dump() const = 0;
	virtual ~ParallelConditionBase() {}
};

/// Условие, требущее приватизации переменных
class PrivatizationCondition : public ParallelConditionBase
{
public:
	/// Список переменных
	typedef std::list<const Reprise::VariableDeclaration*> VariableList;

	PrivatizationCondition(const VariableList& privateVariables);

	/// Возвращает список переменных, которые нужно приватизировать
	const VariableList& getPrivateVariables() const;

	std::string dump() const;

private:
	VariableList m_privateVariables;
};

/// Условие, требующее редукции переменных
class ReductionCondition : public ParallelConditionBase
{
public:
	/// Редукция описывается именем переменной и операцией, по которой необходимо
	/// проводить редукцию
	ReductionCondition(Reprise::VariableDeclaration& variable,
					   Reprise::BasicCallExpression::BuiltinCallKind callKind);

	Reprise::VariableDeclaration& getVariable() const;

	Reprise::BasicCallExpression::BuiltinCallKind getOperation() const;

	std::string dump() const;

private:
	Reprise::VariableDeclaration* m_variable;
	Reprise::BasicCallExpression::BuiltinCallKind m_callKind;
};

/// Условие, требующее предварительной рассылки данных по узлам
class PreliminarySendCondition : public ParallelConditionBase
{
public:
	std::string dump() const;
};

/// Условие, требующее гнездования цикла
class StripMiningCondition : public ParallelConditionBase
{
public:
	std::string dump() const;
};

/// Базовый класс для препятствий распараллеливанию
class ParallelObstacleBase
{
public:
	virtual std::string dump() const = 0;
	virtual ~ParallelObstacleBase() {}
};

/// Вызов функций с побочными эффектами
class DirtyCallsObstacle : public ParallelObstacleBase
						 , public OPS::NonCopyableMix
{
public:
	typedef std::list<OPS::Reprise::CallExpressionBase*> CallList;
	DirtyCallsObstacle(const LoopParallelInfo& loopInfo, const CallList& calls);
	virtual std::string dump() const;

private:
	const LoopParallelInfo& m_loopInfo;
	CallList m_calls;
};

/// "Нехороший" поток управления
class BadControlFlowObstacle : public ParallelObstacleBase
{
public:
	virtual std::string dump() const;
};

/// Цикл не является Basic-циклом
class NonBasicForObstacle : public ParallelObstacleBase
{
public:
	virtual std::string dump() const;
};

/// Границы цикла неизвестны на этапе компиляции
class NonConstantBoundsObstacle : public ParallelObstacleBase
{
public:
	virtual std::string dump() const;
};

/// Запрещенные операторы в теле цикла
class ForbiddenStatementsObstacle : public ParallelObstacleBase
{
public:
	virtual std::string dump() const;
};

class DependenceObstacle : public ParallelObstacleBase
{
public:
	DependenceObstacle(AbstractDependencePtr dependence);

	AbstractDependencePtr dependence() const { return m_dependence; }
	Reprise::ExpressionBase* from() const { return m_dependence->getBeginExpr(); }
	Reprise::ExpressionBase* to() const { return m_dependence->getEndExpr(); }

	std::string getDependenceType() const;

private:
	AbstractDependencePtr m_dependence;
};

/// Циклически порожденная зависимость
class CarriedDependenceObstacle : public DependenceObstacle
{
public:
	CarriedDependenceObstacle(AbstractDependencePtr dependence);
	virtual std::string dump() const;
};

/// Зависимость снизу вверх
class BackwardDependenceObstacle : public DependenceObstacle
{
public:
	BackwardDependenceObstacle(AbstractDependencePtr dependence);
	virtual std::string dump() const;
};

typedef std::tr1::shared_ptr<ParallelConditionBase> ParallelConditionBasePtr;
typedef std::tr1::shared_ptr<ParallelObstacleBase> ParallelObstacleBasePtr;

typedef std::list<ParallelConditionBasePtr> ParallelConditionList;
typedef std::list<ParallelObstacleBasePtr> ParallelObstacleList;

/// Информация о распараллеливаемости на архитектуру
class TargetParallelInfo
{
public:

	explicit TargetParallelInfo(Target target);

	Target getTarget() const { return m_target; }

	/// Распараллеливаем ли цикл или нет
	bool isParallelizable() const { return m_obstacles.empty(); }

	/// Получить список условий распараллеливаемости
	ParallelConditionList& getConditions() { return m_conditions; }
	const ParallelConditionList& getConditions() const { return m_conditions; }

	ParallelObstacleList& getObstacles() { return m_obstacles; }
	const ParallelObstacleList& getObstacles() const { return m_obstacles; }

private:
	Target m_target;
	ParallelObstacleList m_obstacles;
	ParallelConditionList m_conditions;
};

/// Информация о параллельности цикла
class LoopParallelInfo
{
public:

	typedef std::list<Reprise::CallExpressionBase*> CallList;

	/// Цикл, для которого построена информация
	Reprise::ForStatement& getStatement() const { return *m_loop; }

	/// Содержит ли цикл вызовы функций с побочными эффектами
	bool containsDirtyCalls() const { return !m_dirtyCalls.empty(); }

	const CallList& getDirtyCalls() const { return m_dirtyCalls; }

	/// Имеет ли цикл один вход и один выход
	bool isBasicBlock() const;

	/// Возвращает количество архитектур на которые анализировался цикл
	int getTargetCount() const;

	/// Возвращает структуру с информацией о распараллеливаемости на конкретную архитектуру
	const TargetParallelInfo& getTargetInfo(Target target) const;

	std::tr1::shared_ptr<TargetParallelInfo> getTargetInfoPtr(Target target) const;

	void addTargetInfo(std::tr1::shared_ptr<TargetParallelInfo> targetInfo);

	static std::tr1::shared_ptr<LoopParallelInfo> create(Reprise::ForStatement& loop,
											  ControlFlowGraph& cfGraph);

private:

	explicit LoopParallelInfo(Reprise::ForStatement& loop);

	typedef std::map<Target, std::tr1::shared_ptr<TargetParallelInfo> > TargetToConditionListMap;

	Reprise::ForStatement* m_loop;
	CallList m_dirtyCalls;
	bool m_basicBlock;
	TargetToConditionListMap m_targetToConditionsMap;
};

typedef std::map<Reprise::ForStatement*, std::tr1::shared_ptr<LoopParallelInfo> > LoopToMarksMap;

class ParallelLoopsMarker
{
public:
	typedef std::list<Reprise::ForStatement*> LoopList;

	explicit ParallelLoopsMarker(Target target);
	explicit ParallelLoopsMarker(unsigned targets);

	void setUseLatticeGraphs(bool use);
	void setUseMontegoDepGraph(bool use);

	std::tr1::shared_ptr<LoopParallelInfo> markLoop(Reprise::ForStatement& forStmt, OPS::Shared::ProgramContext* context = 0) const;
	std::tr1::shared_ptr<LoopToMarksMap> markLoops(Reprise::BlockStatement& block,  OPS::Shared::ProgramContext* context = 0) const;
	std::tr1::shared_ptr<LoopToMarksMap> markLoops(Reprise::SubroutineDeclaration& subroutine,  OPS::Shared::ProgramContext* context = 0) const;
    std::unique_ptr<LoopToMarksMap> markLoops(Reprise::ProgramUnit& program, OPS::Shared::ProgramContext* context = 0) const;

private:
	std::tr1::shared_ptr<LoopToMarksMap> markLoops(Reprise::BlockStatement& parentBlock, const LoopList& loops,  OPS::Shared::ProgramContext* context = 0) const;

	unsigned m_targets;
	bool m_useLatticeGraphs;
	bool m_useMontegoDepGraph;
};

}

}

namespace Stage
{

class FindParallelLoopsPass : public PassBase
{
public:
    explicit FindParallelLoopsPass(bool useMontegoGraph = true, bool useLatticeGraph = true);
    bool run();

    AnalysisUsage getAnalysisUsage() const;

private:
    bool m_useMontegoGraph;
    bool m_useLatticeGraph;
};

}
}

#endif // PARALLELLOOPS_H
