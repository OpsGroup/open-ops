#pragma once

#include <string>
#include <map>

#include "Reprise/Reprise.h"
#include "ProgramState.h"
#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"
#include "FunctionContext.h"
#include "Analysis/Montego/AliasAnalysis/SetAbstractMemoryCell2.h"
#include "Analysis/Montego/SafeStatus.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/ControlFlowGraph.h"

namespace OPS
{
namespace Montego
{
class BasicOccurrence;
class OccurrenceContainer;
class ExpressionAnalyser;
class MemoryCellContainer;
class AliasAnalysisContext;
class AliasInformationContainer;

/// Класс анализа процедуры
class AliasProcedureAnalyzer : public OPS::NonCopyableMix
{
public:

    explicit AliasProcedureAnalyzer(AliasAnalysisContext* context);
    ~AliasProcedureAnalyzer();

    /// Запуск анализа главной процедуры (main в C)
    /// Возвращает 0 - все плохо, 1 - проработала нормально
    bool Run(OPS::Reprise::SubroutineDeclaration *subDeclaration);

    /// Запуск анализа процедуры, по текущему состоянию программы
    /// Состояние state изменяется!
    /// Возвращает 0 - все плохо, 1 - проработала нормально
    bool Run(OPS::Reprise::SubroutineDeclaration* sdecl,
        const FunctionContext* funContext, ProgramState& state);


    SetAbstractMemoryCell getReturnSAMC();
    SuboffsetsContent getReturnedStruct();

private:

    /// Обход графа потока управления
    void GirthCFG(ControlFlowGraphExpr::BasicBlock& block);
    
    std::unique_ptr<ProgramState> StepOfGirthCFG(ControlFlowGraphExpr::BasicBlock & block);

    bool m_thereIsSyncPoint;
    int m_syncCounter;
    const ControlFlowGraphExpr::BasicBlock* m_syncPoint;

    void clear();

    void visitExpr(const ExpressionBase& expr);

    ProgramState*		    m_currentProgramState;	// текущее состояние программы
    ProgramState*		    m_finalProgramState;	// объединение состояний программы в конце работы анализатора
    const FunctionContext*	m_funContext;			// текущий контекст
    ExpressionAnalyser*	    m_exprAnalyzer;			// Класс анализа выражения
    SavedProgramStates<ControlFlowGraphExpr::BasicBlock>	m_savedProgramStates;   // состояния программы на "перекрестках" графа потока управления
    SetAbstractMemoryCell*  m_returnSAMC;           // множество возвращаемых функцией ячеек
    SuboffsetsContent m_returnedStruct;
    AliasAnalysisContext* m_analysisContext;
    int m_exprAnalyserRetCode; //код ошибки, возвращенный анализатором выражений

#ifdef OPS_BUILD_DEBUG
    int depth;
#endif

    friend class AliasInterface;
};

} // end namespace
} // end namespace
