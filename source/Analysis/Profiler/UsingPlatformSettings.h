#ifndef OPS_PROFILER_USINGPLATFORMSETTINGS_H_INCLUDED__
#define OPS_PROFILER_USINGPLATFORMSETTINGS_H_INCLUDED__

#include "ProfilingInterprocedural.h"
#include "Shared/PlatformSettings.h"

namespace OPS
{
namespace Profiling
{

/// Класс анализа программы
class UsingPlatformSettings
{
	typedef std::map<OPS::Reprise::StatementBase*, OPS::Shared::HardwareOptions> MapStatementHardwareOptions;
	typedef std::map<OPS::Reprise::SubroutineDeclaration*, OPS::Shared::HardwareOptions> MapProcedureHardwareOptions;
public:
	UsingPlatformSettings(const std::wstring	pathToFile);
	~UsingPlatformSettings(void); 
	
	/// Запуск расчета
	bool Calculate(ProgramAnalyser *pProgramAnalyser);
//ProgramAnalyser* GetProgramAnalyser() { return p_ProgramAnalyzer; };
	
	OPS::Shared::HardwareOptions& GetHardwareOptions(OPS::Reprise::StatementBase* sb, bool isOnlyForHeaders) { return (isOnlyForHeaders) ? m_StatementHardwareOptionsOnlyHeaders[sb] : m_StatementHardwareOptionsFull[sb]; };
	OPS::Shared::HardwareOptions& GetHardwareOptions(OPS::Reprise::SubroutineDeclaration* sd) { return m_ProcedureHardwareOptions[sd]; };	
private:
	bool CalculateForStatement(StatementCounter::MapStatementCounter&, MapStatementHardwareOptions&);
	bool CalculateForProcedure();
private:
	MapStatementHardwareOptions			m_StatementHardwareOptionsFull;
	MapStatementHardwareOptions			m_StatementHardwareOptionsOnlyHeaders;
	MapProcedureHardwareOptions			m_ProcedureHardwareOptions;
	OPS::Shared::PlatformSettings*	p_PlatformSettings;					
	ProgramAnalyser* 								p_ProgramAnalyzer;	
	std::wstring										m_PathToFile;				
};

} // end namespace Profiling
} // end namespace OPS

#endif
