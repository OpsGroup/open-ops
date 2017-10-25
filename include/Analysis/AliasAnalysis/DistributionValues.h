#ifndef OPS_ALIASANALYSIS_DOMINATORTREE_H_INCLUDED__
#define OPS_ALIASANALYSIS_DOMINATORTREE_H_INCLUDED__

#include "Analysis/AliasAnalysis/MemoryManager.h"

namespace OPS
{
namespace AliasAnalysis
{

/// Класс распространения значений
class DistributionValues
{
	typedef std::map<StatementBase*, int> ProperNumbering;
	typedef std::pair<StatementBase*, int> PairStatement;
	typedef std::list<PairStatement> ListStatementEx;
public:
	DistributionValues(MemoryManager* aMemManager): m_MemManager(aMemManager){};	
	/// Установка текущей таблицы указателей
	void SetPointsTo(EntriesPointersTable* pointersTable) {	m_PointersTable = pointersTable; };
	/// Создание дерева
	void Build(const ExprName& procCallName, ControlFlowGraph* pCfg, ExprCallEx* exprCallEx, const StatementBase* pStmt, const ExprName& exprName, const SetAbstractMemoryCell& samcSrc);
private:
	/// Правильная нумерация
	void ProperNumberingBuild(ControlFlowGraph* pCfg, ProperNumbering& properNumbering);
	/// Поиск достигающих определений
	bool PushNextStmt(StatementBase* currentStmt, int currentStmtOwner, ControlFlowGraph* pCfg, ProperNumbering properNumbering, ListStatementEx& bufStmt);
	/// Возвращает следующее выражение
	bool GetNextStmt(StatementBase* pElemList, int currentStmtNum, int currentStmtOwner, ControlFlowGraph* pCfg, ProperNumbering properNumbering, ListStatementEx& tmpList);
	/// Обновляет записи активации, если достигнут конец подпрограммы
	void UpdateActivationRecord(ExprCallEx* exprCallEx, const ExprName& exprName, const SetAbstractMemoryCell& samcSrc);
	/// Удаляет выражения после генератора
	void DeleteStmt(int currentStmtOwner, ListStatementEx& bufStmt);
private:
	MemoryManager* m_MemManager;
	EntriesPointersTable* m_PointersTable;	
};

/// Распространитель значений переменных
class DistributionWalker: public OPS::Reprise::Service::DeepWalker
{
public:
	DistributionWalker(const ExprName& exprName, const SetAbstractMemoryCell& samcSrc, const ExprName& procCallName, ExprCallEx* exprCallEx, EntriesPointersTable* aPointersTable, MemoryManager* aMemManager);
	bool Run(StatementBase* stmt);
	bool IsExistGenerator() { return isGenerator; }

	void visit(ReferenceExpression& referenceExpression);
	void visit(BasicCallExpression& basicCallExpr);
	void visit(SubroutineCallExpression& exprCall);
	void visit(ReturnStatement& returnStatement);
	void visit(IfStatement& ifStatement);
	void visit(ForStatement& forStatement);
	void visit(WhileStatement& whileStatement);
	void visit(SwitchStatement& switchStatement);
private:
	bool Parse(RepriseBase* node);

	ExprName m_ExprName;
	SetAbstractMemoryCell m_SamcSrc;
	ExprName m_ProcCallName;
	ExprCallEx* m_ExprCallEx;
	EntriesPointersTable* m_PointersTable;
	MemoryManager* m_MemManager;
	bool isGenerator;
};

} // end namespace AliasAnalysis
} // end namespace OPS


#endif
