#include "Analysis/Profiler/Profiler.h"
#include "UsingPlatformSettings.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Profiling
{

//--------	HardwareIndependentProfiler class implementation --------

HardwareIndependentProfiler::HardwareIndependentProfiler(bool prfWithSubroutineCalls)
{
  m_ProfilingMode.prfWithSubroutineCalls = prfWithSubroutineCalls;
	p_ProgramAnalyzer = new ProgramAnalyser(m_ProfilingMode);	
}

HardwareIndependentProfiler::~HardwareIndependentProfiler(void)
{
	delete p_ProgramAnalyzer;
}

bool HardwareIndependentProfiler::Run(OPS::Reprise::RepriseBase *pCodeForAnalysis)
{
	if (pCodeForAnalysis->is_a<ProgramUnit>())
		return p_ProgramAnalyzer->Run(pCodeForAnalysis->cast_ptr<ProgramUnit>());
	else
		if (pCodeForAnalysis->is_a<TranslationUnit>())
			return p_ProgramAnalyzer->Run(pCodeForAnalysis->cast_ptr<TranslationUnit>());
		else
			return false;
}

CounterOperationsByType HardwareIndependentProfiler::GetCounterOperationsByType(OPS::Reprise::StatementBase* stmt, bool isOnlyForHeaders)
{
	return p_ProgramAnalyzer->GetStmtCounter(isOnlyForHeaders)->GetProfilingResults(stmt); 
};

CounterOperationsByType HardwareIndependentProfiler::GetCounterOperationsByType(OPS::Reprise::SubroutineDeclaration* proc)
{ 
	return p_ProgramAnalyzer->GetProcCounter()->GetProfilingResults(proc); 
};


//--------	DialogProfiler class implementation --------

DialogProfiler::DialogProfiler(bool prfWithSubroutineCalls)
	: HardwareIndependentProfiler(prfWithSubroutineCalls)
{
	m_InStmtLimIter.clear();
	m_OutStmtLimIter.clear();
	p_ProgramAnalyzer->SetLimits(&m_InStmtLimIter, &m_OutStmtLimIter);
}

StatementLimitsIteration DialogProfiler::GetStatementLimitsIteration(void)
{
	return m_OutStmtLimIter;
}

void DialogProfiler::SetStatementLimitsIteration(StatementLimitsIteration stmtLimIter)
{
	m_InStmtLimIter = stmtLimIter;
}


//--------	Profiler class implementation --------

Profiler::Profiler(const std::wstring pathToPlatformSettingsFile)
{
	p_UsingPlatformSettings = new UsingPlatformSettings(pathToPlatformSettingsFile);	
}

bool Profiler::Run(const HardwareIndependentProfiler &hardwareIndependentProfiler)
{
	return p_UsingPlatformSettings->Calculate(hardwareIndependentProfiler.p_ProgramAnalyzer);
}

Profiler::~Profiler(void)
{
	delete p_UsingPlatformSettings;
}

long_long_t Profiler::GetTimeCounter(OPS::Reprise::RepriseBase *pRepriseBase, bool isOnlyForHeaders)
{
	if (pRepriseBase->is_a<StatementBase>())
		return p_UsingPlatformSettings->GetHardwareOptions(pRepriseBase->cast_ptr<StatementBase>(), isOnlyForHeaders).Complexity;		
	else
		if (pRepriseBase->is_a<SubroutineDeclaration>())
			return p_UsingPlatformSettings->GetHardwareOptions(pRepriseBase->cast_ptr<SubroutineDeclaration>()).Complexity;				
		else
			throw RuntimeError("Result for this object is not defined");
}

long_long_t Profiler::GetSizeCommand(OPS::Reprise::RepriseBase *pRepriseBase, bool isOnlyForHeaders)
{
	if (pRepriseBase->is_a<StatementBase>())
		return p_UsingPlatformSettings->GetHardwareOptions(pRepriseBase->cast_ptr<StatementBase>(), isOnlyForHeaders).SizeCommand;		
	else
		if (pRepriseBase->is_a<SubroutineDeclaration>())
			return p_UsingPlatformSettings->GetHardwareOptions(pRepriseBase->cast_ptr<SubroutineDeclaration>()).SizeCommand;				
		else
			throw RuntimeError("Result for this object is not defined");
}

long_long_t Profiler::GetSizeData(OPS::Reprise::RepriseBase *pRepriseBase, bool isOnlyForHeaders)
{
	if (pRepriseBase->is_a<StatementBase>())
		return p_UsingPlatformSettings->GetHardwareOptions(pRepriseBase->cast_ptr<StatementBase>(), isOnlyForHeaders).SizeData;		
	else
		if (pRepriseBase->is_a<SubroutineDeclaration>())
			return p_UsingPlatformSettings->GetHardwareOptions(pRepriseBase->cast_ptr<SubroutineDeclaration>()).SizeData;				
		else
			throw RuntimeError("Result for this object is not defined");
}

}
}
