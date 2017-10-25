#ifndef OPS_PROFILER_PROFILER_H_INCLUDED__
#define OPS_PROFILER_PROFILER_H_INCLUDED__

#include "Reprise/Types.h"
#include "Reprise/Declarations.h"

namespace OPS
{
namespace Profiling
{
class ProgramAnalyser;

/// Режим профилирования
struct ProfilingMode
{
  ProfilingMode()
    : prfWithSubroutineCalls(false) {}

  bool prfWithSubroutineCalls;  // Анализ с учетом вызовов процедур (самой горячей будет main)
};

/// Объем кода: <операция, <тип данных, количество>>
typedef std::map<OPS::Reprise::BasicType::BasicTypes, long_long_t> MapTypeCounter;
typedef std::map<int, MapTypeCounter> CounterOperationsByType;

//Статический аппаратно-независимый профилировщик
class HardwareIndependentProfiler
{
	friend class Profiler;
public:
/* Параметры профилирования:
     prfWithSubroutineCalls - профилирование с учетом времени дочерних процедур (при таком режите main - самая горячая точка) */
	explicit HardwareIndependentProfiler(bool prfWithSubroutineCalls = false);
	~HardwareIndependentProfiler(void);

	/// Запуск профилировщика
	bool Run(OPS::Reprise::RepriseBase *pCodeForAnalysis);

	/// Результаты профилирования (isOnlyForHeaders - результат только для заголовков выражений for, while, if и т.д.)
  CounterOperationsByType GetCounterOperationsByType(OPS::Reprise::StatementBase* stmt, bool isOnlyForHeaders = false);
  CounterOperationsByType GetCounterOperationsByType(OPS::Reprise::SubroutineDeclaration* proc);
protected:
	ProgramAnalyser* p_ProgramAnalyzer;
private:
	ProfilingMode    m_ProfilingMode;
};

/// Диапазон итераций цикла
struct LimitsIteration
{
    LimitsIteration(): min(-1), max(-1) {};
    long_long_t min;
    long_long_t max;
};

/// Map операторов и диапазонов итераций циклов
typedef std::map<OPS::Reprise::StatementBase*, LimitsIteration> StatementLimitsIteration;


/// Диалоговое профилирование
class DialogProfiler: public HardwareIndependentProfiler
{
public:	
/* Параметры профилирования:
     prfWithSubroutineCalls - профилирование с учетом времени дочерних процедур (при таком режите main - самая горячая точка) */
	DialogProfiler(bool prfWithSubroutineCalls = false);
	
	StatementLimitsIteration GetStatementLimitsIteration(void);							// Возвращает невычисленные операторы
	void SetStatementLimitsIteration(StatementLimitsIteration stmtLimIter);	// Получает информацию от пользователя
private:
	StatementLimitsIteration	m_OutStmtLimIter;	// Информация о невычисляемых операторах
	StatementLimitsIteration	m_InStmtLimIter;	// Информация от пользователя
};

class UsingPlatformSettings;

/// Статический профилировщик
class Profiler
{
public:
/* Параметры профилирования:
     pathToPlatformSettingsFile - полный путь к файлу с параметрами платформы */
	Profiler(const std::wstring pathToPlatformSettingsFile);
	~Profiler(void);

	/// Запуск профилировщика
	bool Run(const HardwareIndependentProfiler &hardwareIndependentProfiler);

	/// Результаты профилирования (StatementBase или SubroutineDeclaration), isOnlyForHeaders - результат только для заголовков (for, while, if и т.д.)
	long_long_t GetTimeCounter(OPS::Reprise::RepriseBase*, bool isOnlyForHeaders = false);
	long_long_t GetSizeCommand(OPS::Reprise::RepriseBase*, bool isOnlyForHeaders = false);
	long_long_t GetSizeData(OPS::Reprise::RepriseBase*, bool isOnlyForHeaders = false);
protected:
	UsingPlatformSettings* p_UsingPlatformSettings;
};

} // end namespace Profiling
} // end namespace OPS

#endif
