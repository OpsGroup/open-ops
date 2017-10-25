#ifndef OPS_ALIASANALYSIS_EXPRANALYZER_H_INCLUDED__
#define OPS_ALIASANALYSIS_EXPRANALYZER_H_INCLUDED__

#include "Analysis/AliasAnalysis/DistributionValues.h"

namespace OPS
{
namespace AliasAnalysis
{

/// Класс анализа выражения
class ExprAnalyzer
{
public:
	ExprAnalyzer(MemoryManager* aMemManager): m_MemManager(aMemManager) { m_DistributionValues = new DistributionValues(m_MemManager); };
	virtual ~ExprAnalyzer(void) { delete m_DistributionValues; };
	/// Установка значений переменных
	void Initialize(const ExprName& procName, ExprCallEx* exprCallEx, StatementBase* pStmt, ControlFlowGraph* pCfg, const ValuesPointerMap& globalParam, const ValuesPointerMap& actualParam);
	///	Запуск анализа
	bool Parse(const ExprName& procName, ExprCallEx* exprCallEx, StatementBase* pStmt, ControlFlowGraph* pCfg);
	/// Добавить вхождение указателя
	void AddEntriesPointers(ReferenceExpression* exprData, SetAbstractMemoryCell& m_samc);
private:
	/// Выставление параметров
	void SetVarParam(const ExprName& procName, ExprCallEx* exprCallEx, StatementBase* pStmt, ControlFlowGraph* pCfg);
	///	Разбор выражения
	void ParseAssign(RepriseBase& node);
private:
	ReferenceExpression* m_exprData;
	SetAbstractMemoryCell m_samc;
	bool m_isExprPointer;
	StatementBase* m_pStmt;
	ControlFlowGraph* m_pCfg;
	ExprName m_ProcName;
	ExprCallEx* m_ExprCallEx;
	DistributionValues* m_DistributionValues;
	MemoryManager* m_MemManager;
	EntriesPointersTable* m_PointersTable;		
};

/// Поиск генераторов указателей
class ExprWalker: public OPS::Reprise::Service::DeepWalker
{
public:
	ExprWalker(MemoryManager* aMemManager);
	bool Run(RepriseBase& node);	
	bool isExprPointer() { return m_IsExprPointer; }
	OPS::Reprise::ReferenceExpression* getExprLeft() {return m_ExprLeft; }
	OPS::Reprise::ExpressionBase* getExprRight() {return m_ExprRight; }
private:
	void visit(OPS::Reprise::BasicCallExpression& basicCallExpression);
	void visit(ForStatement& forStatement);
	void visit(WhileStatement& whileStatement);
	void visit(IfStatement& ifStatement);
	void visit(SwitchStatement& switchStatement);

	bool m_IsExprPointer;
	OPS::Reprise::ReferenceExpression* m_ExprLeft;
	OPS::Reprise::ExpressionBase* m_ExprRight;	
	MemoryManager* m_MemManager;
};

/// Поиск генераторов указателей
class RightExprWalker: public OPS::Reprise::Service::DeepWalker
{
public:
	RightExprWalker(MemoryManager* aMemManager, EntriesPointersTable* aPointersTable, SetAbstractMemoryCell* aSamc)
		: m_MemManager(aMemManager), m_PointersTable(aPointersTable), m_SAMC(aSamc) {};
	void visit(OPS::Reprise::ReferenceExpression& referenceExpression);	
	void visit(OPS::Reprise::BasicCallExpression& basicCallExpression);		
	bool Run(ExpressionBase* expressionBase);
private:
	bool HasTemplateSimpleAssign(BasicCallExpression& basicCallExpression);
	bool HasTemplateArrayAssign(BasicCallExpression& basicCallExpression);
	bool HasTemplateAssignPtrOprConst(BasicCallExpression& basicCallExpression);
	MemoryManager* m_MemManager;
	EntriesPointersTable* m_PointersTable;
	SetAbstractMemoryCell* m_SAMC;	
};

} // end namespace AliasAnalysis
} // end namespace OPS


#endif
