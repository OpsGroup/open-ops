#ifndef OPS_PROFILER_PROFILINGWITHINSUBROUTINE_H_INCLUDED__
#define OPS_PROFILER_PROFILINGWITHINSUBROUTINE_H_INCLUDED__

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <functional>

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include "Analysis/Profiler/Profiler.h"

namespace OPS
{
namespace Profiling
{

class ProfilingResults
{
public:
    ProfilingResults();
    ProfilingResults(const CounterOperationsByType& aCObT);

    void Add(int CallKind, OPS::Reprise::BasicType::BasicTypes basicTypes);
    void Sum(ProfilingResults &psOWN);
    void Mult(long_long_t m);
    double Count();
public:
    CounterOperationsByType CObT;
};

/// Vector операторов в порядке возрастания их времени выполнения
typedef std::vector<OPS::Reprise::StatementBase*> StatementVector;

/// Класс сбора информации об операторах
class StatementCounter
{
public:
    typedef std::map<OPS::Reprise::StatementBase*, CounterOperationsByType> MapStatementCounter;
public:
    StatementCounter();
    void SetProfilingResults(OPS::Reprise::StatementBase *sb, CounterOperationsByType &cobt);
    CounterOperationsByType GetProfilingResults(OPS::Reprise::StatementBase *sb);
    MapStatementCounter& GetMapStatementCounter();
private:
    MapStatementCounter m_mapStmtCounter;
};


/// Класс сбора информации о подпрограммах
class ProcedureCounter
{
public:
    typedef std::map<OPS::Reprise::SubroutineDeclaration*, CounterOperationsByType> MapProcedureCounter;
public:
    ProcedureCounter();
    void SetProfilingResults(OPS::Reprise::SubroutineDeclaration *sd, CounterOperationsByType& cobt);
    CounterOperationsByType GetProfilingResults(OPS::Reprise::SubroutineDeclaration *sd);
    MapProcedureCounter& GetMapProcedureCounter();
private:
    MapProcedureCounter m_mapProcCounter;
};

/// Внутрипроцедурный анализ
class WithinSubroutineAnalysis :
    public OPS::BaseVisitor,
    public OPS::Visitor<OPS::Reprise::BasicLiteralExpression>,
    public OPS::Visitor<OPS::Reprise::StrictLiteralExpression>,
    public OPS::Visitor<OPS::Reprise::ReferenceExpression>,
    public OPS::Visitor<OPS::Reprise::StructAccessExpression>,
    public OPS::Visitor<OPS::Reprise::EnumAccessExpression>,
    public OPS::Visitor<OPS::Reprise::BasicCallExpression>,
    public OPS::Visitor<OPS::Reprise::SubroutineCallExpression>,

    public OPS::Visitor<OPS::Reprise::ExpressionStatement>,
    public OPS::Visitor<OPS::Reprise::BlockStatement>,
    public OPS::Visitor<OPS::Reprise::ForStatement>,
    public OPS::Visitor<OPS::Reprise::WhileStatement>,
    public OPS::Visitor<OPS::Reprise::IfStatement>,
    public OPS::Visitor<OPS::Reprise::GotoStatement>,
    public OPS::Visitor<OPS::Reprise::ReturnStatement>,
    public OPS::NonCopyableMix
{
    typedef std::list<OPS::Reprise::StatementBase*> StatementList;
public:
    WithinSubroutineAnalysis(OPS::Reprise::RepriseBase*, ProfilingResults*, ProfilingMode, StatementCounter*, StatementCounter*, ProcedureCounter*, StatementLimitsIteration*, StatementLimitsIteration*);
    WithinSubroutineAnalysis(OPS::Reprise::RepriseBase*, ProfilingResults*, const WithinSubroutineAnalysis*);
private:
    void Run(OPS::Reprise::RepriseBase* repriseBase);
    OPS::Reprise::BasicType::BasicTypes GetBasicTypes(OPS::Reprise::TypeBase* typeBase);
    OPS::Reprise::BasicType::BasicTypes GetTypes(OPS::Reprise::ExpressionBase* e);
    void ParseLeftExpression(OPS::Reprise::ExpressionBase&);
    void AddReadForDynamicArray(OPS::Reprise::ExpressionBase&);
    StatementList GetStatementListForGoto(OPS::Reprise::GotoStatement&);
    long_long_t GetCountIterations(OPS::Reprise::ForStatement& forStmt);
    long_long_t UserDialog(OPS::Reprise::StatementBase* stmt);

    void visit(OPS::Reprise::BasicLiteralExpression&);
    void visit(OPS::Reprise::StrictLiteralExpression&);
    void visit(OPS::Reprise::ReferenceExpression&);
    void visit(OPS::Reprise::StructAccessExpression&);
    void visit(OPS::Reprise::EnumAccessExpression&);
    void visit(OPS::Reprise::BasicCallExpression&);
    void visit(OPS::Reprise::SubroutineCallExpression&);
    void visit(OPS::Reprise::ExpressionStatement&);
    void visit(OPS::Reprise::BlockStatement&);
    void visit(OPS::Reprise::IfStatement&);
    void visit(OPS::Reprise::ForStatement&);
    void visit(OPS::Reprise::WhileStatement&);
    void visit(OPS::Reprise::GotoStatement&);
    void visit(OPS::Reprise::ReturnStatement&);
public:
    ProfilingMode							m_ProfilingMode;
    StatementCounter*					p_StmtCounterFull;
    StatementCounter*					p_StmtCounterOnlyHeaders;
    ProcedureCounter*					p_ProcCounter;
    StatementLimitsIteration*	p_InStmtLimIter;
    StatementLimitsIteration*	p_OutStmtLimIter;
    ProfilingResults*					p_ProfilingResults;
};

} // end namespace Profiling
} // end namespace OPS

#endif
