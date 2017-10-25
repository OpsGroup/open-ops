#ifndef OPS_ALIASANALYSIS_POINTERSTABLE_H_INCLUDED__
#define OPS_ALIASANALYSIS_POINTERSTABLE_H_INCLUDED__

#include "Analysis/AliasAnalysis/SetAbstractMemoryCell.h"

namespace OPS
{
namespace AliasAnalysis
{

///	Map вхождений указателей
typedef std::map<ReferenceExpression*, SetAbstractMemoryCell> EntriesPointerMap;

/// Таблица вхождений указателей
class EntriesPointersTable
{		
	EntriesPointerMap m_entriesPointerMap;	/// Таблица указателей	
public:
	EntriesPointersTable(void) { m_entriesPointerMap.clear(); };
	EntriesPointersTable(EntriesPointerMap entriesPointerMap): m_entriesPointerMap(entriesPointerMap) {};
	/// Добавляет множество абстрактных ячеек памяти
	void SetSAMC(ReferenceExpression* referenceExpression, SetAbstractMemoryCell samc);
	/// Возвращает множество абстрактных ячеек памяти
	SetAbstractMemoryCell GetSAMC(ReferenceExpression* referenceExpression);
	/// Объединение множеств абстрактных ячеек памяти
	void UnionSAMC(ReferenceExpression* referenceExpression, SetAbstractMemoryCell samc2);
	/// Очищает таблицу
	void Clear();
};

///	Значения указателей
typedef std::pair<ExprName, SetAbstractMemoryCell> ValuesPointerPair;
typedef std::map<ExprName, SetAbstractMemoryCell> ValuesPointerMap;

/// Параметры вызова процедур
struct CallParameters
{
	ValuesPointerMap globalParam;
	ValuesPointerMap actualParam;				
};

/// Запись активации
struct ActivationRecord
{
	CallParameters InParam;
	CallParameters OutParam;
	SetAbstractMemoryCell ProcResult;
};

/// Расширенный класс вызова подпрограммы
class ExprCallEx
{
	const SubroutineCallExpression* exprCall;	///	Точка вызова
	BlockStatement* procBody;					///	Тело процедуры
	EntriesPointersTable* pointersTable;		///	Таблица вхождений указателей
	ActivationRecord* activationRecord;			/// Запись активации проседцры
public:		
	ExprCallEx() { exprCall = NULL; procBody = NULL; pointersTable = new EntriesPointersTable(); activationRecord = new ActivationRecord(); };
	ExprCallEx(const SubroutineCallExpression* pExprCall, BlockStatement* pProcBody): exprCall(pExprCall), procBody(pProcBody) { pointersTable = new EntriesPointersTable(); activationRecord = new ActivationRecord(); };
	virtual ~ExprCallEx(void) { delete pointersTable; };

	const SubroutineCallExpression* GetExprCall() const { return exprCall; }
	void SetExprCall(const SubroutineCallExpression* pExprCall) { exprCall=pExprCall; }

	BlockStatement* GetProcBlock() { return procBody; }
	void SetProcBlock(BlockStatement* pProcBody) { procBody=pProcBody; }

	EntriesPointersTable* GetPointersTable() { return pointersTable; }
	void SetPointersTable(EntriesPointersTable* pPointersTable) { pointersTable=pPointersTable; }		

	ActivationRecord* GetActivationRecord() { return activationRecord; }
	void SetActivationRecord(ActivationRecord* pActivationRecord) { activationRecord=pActivationRecord; }
};

} // end namespace AliasAnalysis
} // end namespace OPS


#endif
