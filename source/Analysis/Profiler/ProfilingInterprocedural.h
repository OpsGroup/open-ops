#ifndef OPS_PROFILER_PROFILINGINTERPROCEDURAL_H_INCLUDED__
#define OPS_PROFILER_PROFILINGINTERPROCEDURAL_H_INCLUDED__

#include "ProfilingWithinSubroutine.h"

namespace OPS
{
namespace Profiling
{

/// Класс анализа программы
class ProgramAnalyser
{
	typedef std::vector<OPS::Reprise::SubroutineDeclaration*> VectorSubroutineDeclaration;
public:
	ProgramAnalyser(ProfilingMode);

	/// Запуск анализа процедуры
	bool Run(OPS::Reprise::RepriseBase*);
  /// Передача списков границ циклов
	void SetLimits(StatementLimitsIteration*, StatementLimitsIteration*);	
		
	/// Возвращает информацию об операторах
	StatementCounter* GetStmtCounter(bool isOnlyHeaders) { return (isOnlyHeaders) ? &m_StmtCounterOnlyHeaders : &m_StmtCounterFull; }
	/// Возвращает информацию о подпрограммах
	ProcedureCounter* GetProcCounter() { return &m_ProcCounter; }
	
private:
	// Создание списка процедур
	void BuildProcList(OPS::Reprise::TranslationUnit *translationUnit);
	// Создает список без учета вызовов подпрограмм
	void BuildSimpleList(OPS::Reprise::TranslationUnit *translationUnit);
	// Создает список с учетом вызовов подпрограмм
	bool BuildListWithCall(OPS::Reprise::TranslationUnit *translationUnit);
	// Обработка списка процедур
	void GirthListProc();
private:
	VectorSubroutineDeclaration		m_SubDeclarations;					// Список процедур  
	ProfilingMode									m_ProfilingMode;						// Тип профилирования
	StatementCounter							m_StmtCounterFull;					// Сложность операторов
	StatementCounter							m_StmtCounterOnlyHeaders;		// Сложность операторов
	ProcedureCounter							m_ProcCounter;							// Сложность подпрограмм	
	StatementLimitsIteration*			p_InStmtLimIter;						// Информация от пользователя
	StatementLimitsIteration*			p_OutStmtLimIter;						// Информация о невычисляемых операторах
};

} // end namespace Profiling
} // end namespace OPS

#endif
