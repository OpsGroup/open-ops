#ifndef OPS_ALIASANALYSIS_ALIASANALYSIS_H_INCLUDED__
#define OPS_ALIASANALYSIS_ALIASANALYSIS_H_INCLUDED__

#include "Analysis/AliasAnalysis/ProcAnalyzer.h"

namespace OPS
{
namespace AliasAnalysis
{

/// Класс анализа указателей
class AliasAnalyzer
{
public:
	AliasAnalyzer(ProgramUnit* aProgramUnit);
	~AliasAnalyzer(void);	
	///	Проверка на псевдоним
	AliasResult IsAlias(ReferenceExpression* firstPointer, SubroutineCallExpression* firstExprCall, ReferenceExpression* secondPointer, SubroutineCallExpression* secondExprCall);
	///	Возвращает расширенный граф вызовов, содержащий информацию об указателях
	const CallGraphEx *GetCallGraphEx() const { return m_CallGraphEx; }
private:
	///	Запуск анализа
	bool Make(void);
	/// Проверка параметров
	bool IsParameters(void);
	/// Поиск вызовов
	ExprCallEx* GetSubCallExpr(ReferenceExpression* pointer);
private:
	ProgramUnit				*m_ProgramUnit;		///	Класс программы
	TranslationUnit			*m_MainUnit;		/// Модуль содержащий main
	SubroutineDeclaration	*m_MainProc;		/// Функция main
	CallGraphEx				*m_CallGraphEx;		/// Расширенный граф вызовов
	MemoryManager			*m_MemManager;		///	Менаджер памяти
	ProcAnalyzer			*m_ProcAnalyzer;	/// Анализ процедуры	
};

} // end namespace AliasAnalysis
} // end namespace OPS


#endif
