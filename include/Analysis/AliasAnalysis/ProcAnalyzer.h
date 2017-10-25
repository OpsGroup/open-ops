#ifndef OPS_ALIASANALYSIS_PROCANALYZER_H_INCLUDED__
#define OPS_ALIASANALYSIS_PROCANALYZER_H_INCLUDED__

#include "Analysis/AliasAnalysis/ExprAnalyzer.h"

namespace OPS
{
namespace AliasAnalysis
{
/// Класс анализа процедуры
class ProcAnalyzer
{
public:
	ProcAnalyzer(CallGraphEx* aCallGraphEx, MemoryManager* aMemManager): m_CallGraphEx(aCallGraphEx), m_MemManager(aMemManager) { m_ExprAnalyzer = new ExprAnalyzer(m_MemManager); };
	~ProcAnalyzer(void) { delete m_ExprAnalyzer; }	
	/// Запуск анализа процедуры
	void Run(ExprName procName, ExprCallEx* exprCallEx);
private:
	/// Инициализация
	void Initialize(ExprCallEx* exprCallEx);
	/// Обход графа управления потоком
	void GirthGraph(const ExprName& procName, ExprCallEx* exprCallEx);
	/// Разбор sub процедуры
	SetAbstractMemoryCell Parse(const ExprName& procName, ExprCallEx* exprCallEx, StatementBase* pStmt, ControlFlowGraph* pCfg, SubroutineCallExpression* subExprCall, const ExprName& resultName);
	/// Установка параметров вызова процедуры
	void SetParam(ExprCallEx* exprCallEx, ExprCallEx* subExprCallEx, SubroutineCallExpression* subExprCall, const VariablesList& convertParam);
	/// Проверка параметров
	void CheckParam(const ExprName& resultName, ValuesPointerMap& globalParam, ValuesPointerMap& actualParam);
	/// Возвращает тело процедуры
	BlockStatement* GetSubProcBody(const ExprName& subProcName, TranslationUnit* aTranslationUnit);
private:
	CallGraphEx*   m_CallGraphEx;	///	Расширенный граф вызовов
	MemoryManager* m_MemManager;	///	Менаджер памяти
	ExprAnalyzer*  m_ExprAnalyzer;	///	Класс анализа выражения
};

/// Обработчик вызовов процедур
class ProcWalker: public OPS::Reprise::Service::DeepWalker
{
public:
	ProcWalker(MemoryManager* aMemManager);
	bool Run(StatementBase* statementBase);
	void visit(OPS::Reprise::SubroutineCallExpression& callExpr);
	void visit(OPS::Reprise::BasicCallExpression& basicCallExpression);
	bool isExistCall() { return m_IsCall; }
	OPS::Reprise::SubroutineCallExpression* getSubCallExpr() { return m_CallExpr; }
	OPS::Reprise::ReferenceExpression* getResultExprData() {return m_ResultExpr; }
	OPS::Reprise::BasicLiteralExpression* getLiteralExpr() {return m_LiteralExpr; }
	long_long_t getOffset() { return m_Offset; }
private:
	bool HasTemplateAssignFuncPlusImm(OPS::Reprise::BasicCallExpression& basicCallExpression);
	bool HasTemplateSimpleAssign(OPS::Reprise::BasicCallExpression& basicCallExpression);
	bool m_IsCall;
	OPS::Reprise::SubroutineCallExpression* m_CallExpr;
	OPS::Reprise::ReferenceExpression* m_ResultExpr;
	OPS::Reprise::BasicLiteralExpression* m_LiteralExpr;
	long_long_t m_Offset;
	MemoryManager* m_MemManager;
};

/// Поиск вызовов процедуры
class SearchProcCall: public OPS::Reprise::Service::DeepWalker
{
public:
	SearchProcCall(std::string procName): m_ProcName(procName) { m_CallExpr = 0; };
	void visit(OPS::Reprise::BlockStatement& blockStatement) { DeepWalker::visit(blockStatement); }
	void visit(OPS::Reprise::SubroutineCallExpression& callExpr);	
	OPS::Reprise::SubroutineCallExpression* getSubCallExpr() { return m_CallExpr; }
private:
	std::string m_ProcName;
	OPS::Reprise::SubroutineCallExpression* m_CallExpr;
};

} // end namespace AliasAnalysis
} // end namespace OPS

#endif
