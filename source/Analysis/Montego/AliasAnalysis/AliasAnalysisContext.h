#pragma once

#include <memory>
#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"
#include "Navigation.h"

namespace OPS
{
namespace Montego
{
class OccurrenceContainer;
class AliasAnalysisOptions;
class Navigation;

class ControlFlowGraphBuilder
{
public:
	ControlFlowGraphBuilder() {}
	~ControlFlowGraphBuilder();
    ControlFlowGraphExpr& buildControlFlowGraph(const Reprise::BlockStatement &block);

private:
    typedef std::map<const OPS::Reprise::BlockStatement*, ControlFlowGraphExpr*> BlockToGraphMap;
	BlockToGraphMap m_blockToGraph;
};

class AliasProcedureCallContext
{
    std::string m_name;
    const FunctionContext* m_context;
public:

    AliasProcedureCallContext(std::string name, const FunctionContext* context): m_name(name), m_context(context)
    {}

    friend class AliasProcedureCallContextComparer;
};

struct AliasProcedureCallContextComparer
{
    bool operator()(const AliasProcedureCallContext& first, const AliasProcedureCallContext& second) const
    {
        int s_cmp = first.m_name.compare(second.m_name);
        if(s_cmp == 0)
        {
            return first.m_context < second.m_context;
        }
        return s_cmp < 0;
    }
};

class AliasAnalysisContext
{
public:
    typedef std::set<Reprise::SubroutineDeclaration*> SubroutineSet;
    typedef std::set<ControlFlowGraphExpr::BasicBlock*> BasicBlockSet;

    AliasAnalysisContext(Reprise::ProgramUnit &pu, OccurrenceContainer *occurs, AliasAnalysisOptions options);
    ~AliasAnalysisContext();

	OccurrenceContainer& occurrenceContainer() { return *m_occurs; }

    MemoryCellContainer& memoryCellContainer() { return *m_memoryCellContainer; }

    AliasInformationContainer& aliasInformationContainer() { return *m_aliasInformationContainer; }

	ControlFlowGraphBuilder& cfgBuilder() { return m_cfgBuilder; }

    AliasAnalysisOptions& options() { return m_options; }

	ProgramStateForProcs& visitedProcs() { return m_visitedProcs; }

	bool isSafeSubroutine(OPS::Reprise::SubroutineDeclaration* decl);

    std::map<const OPS::Reprise::ExpressionBase*, int> m_exprVisitsCount;

    std::map<AliasProcedureCallContext, ProgramState, AliasProcedureCallContextComparer> m_runCache;

    Navigation m_navigation; // класс, рассчитывающий траекторию обхода программы анализатором

private:
	OccurrenceContainer* m_occurs; // Контейнер вхождений
    MemoryCellContainer* m_memoryCellContainer; // контейнер ячеек памяти
    AliasInformationContainer* m_aliasInformationContainer; // контейнер результатов посещения вхождений анализатором выражений
	ControlFlowGraphBuilder m_cfgBuilder;
    AliasAnalysisOptions m_options;
    BasicBlockSet m_visitedBlocks; //сюда заносим все посещенные операторы во всех функциях, чтобы потом можно было проверить
	ProgramStateForProcs m_visitedProcs; //состояния программы при входе в каждую функцию
};

}
}
