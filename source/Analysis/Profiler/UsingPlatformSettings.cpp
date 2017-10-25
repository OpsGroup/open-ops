#include "UsingPlatformSettings.h"
#include "OPS_Core/IO.h"
#include "OPS_Core/Kernel.h"

using namespace OPS::Reprise;
using namespace OPS::Shared;

namespace OPS
{
namespace Profiling
{
  
UsingPlatformSettings::UsingPlatformSettings(const std::wstring	pathToFile)
	: m_PathToFile(pathToFile) 
{
	if (m_PathToFile==L"")
	{
		m_PathToFile = IO::combinePath(getStartupPath(), L"../samples/Analysis/Profiling/PlatformSettings.xml");
	}
	p_PlatformSettings = new PlatformSettings(m_PathToFile);
}
  
UsingPlatformSettings::~UsingPlatformSettings(void) 
{
	delete p_PlatformSettings;
}

bool UsingPlatformSettings::Calculate(ProgramAnalyser *pProgramAnalyser)
{
	p_ProgramAnalyzer = pProgramAnalyser;
	CalculateForStatement(p_ProgramAnalyzer->GetStmtCounter(false)->GetMapStatementCounter(), m_StatementHardwareOptionsFull);
	CalculateForStatement(p_ProgramAnalyzer->GetStmtCounter(true)->GetMapStatementCounter(), m_StatementHardwareOptionsOnlyHeaders);
	return CalculateForProcedure();
}

bool UsingPlatformSettings::CalculateForStatement(StatementCounter::MapStatementCounter &MSCstmt, MapStatementHardwareOptions	&m_StatementHardwareOptions)
{
	//StatementCounter::MapStatementCounter &MSCstmt = p_ProgramAnalyzer->GetStmtCounter()->GetMapStatementCounter();
	StatementCounter::MapStatementCounter::iterator itMSCstmt;
  CounterOperationsByType::iterator itCObTstmt;
  MapTypeCounter::iterator itMTCstmt;
	for (itMSCstmt=MSCstmt.begin(); itMSCstmt!=MSCstmt.end(); itMSCstmt++)
	{//обход stmt
		StatementBase *stmt = itMSCstmt->first; //stmt		
		HardwareOptions hardwareOptionsStmt;

    CounterOperationsByType &CObTstmt = itMSCstmt->second;
		for (itCObTstmt=CObTstmt.begin(); itCObTstmt!=CObTstmt.end(); itCObTstmt++)
		{
			int callKind = itCObTstmt->first; //kind	
      MapTypeCounter &MTCstmt = itCObTstmt->second;
			for (itMTCstmt=MTCstmt.begin(); itMTCstmt!=MTCstmt.end(); itMTCstmt++)
			{
				BasicType::BasicTypes basicTypes = itMTCstmt->first; //type
				long_long_t mult = itMTCstmt->second;	//counter				
				
				HardwareOptions hoCurrentOperationAndType = p_PlatformSettings->GetValue(callKind, basicTypes);
				hoCurrentOperationAndType = hoCurrentOperationAndType * mult;
				hardwareOptionsStmt = hardwareOptionsStmt + hoCurrentOperationAndType;

				m_StatementHardwareOptions[stmt]=hardwareOptionsStmt;
			}
		}
	}
	return true;
}

bool UsingPlatformSettings::CalculateForProcedure()
{
	ProcedureCounter::MapProcedureCounter &MSCproc = p_ProgramAnalyzer->GetProcCounter()->GetMapProcedureCounter();
	ProcedureCounter::MapProcedureCounter::iterator itMSCproc;
    CounterOperationsByType::iterator itCObTproc;
    MapTypeCounter::iterator itMTCproc;
	for (itMSCproc=MSCproc.begin(); itMSCproc!=MSCproc.end(); itMSCproc++)
	{//обход proc
		SubroutineDeclaration *proc = itMSCproc->first; //proc		
		HardwareOptions hardwareOptionsProc;

        CounterOperationsByType &CObTproc = itMSCproc->second;
		for (itCObTproc=CObTproc.begin(); itCObTproc!=CObTproc.end(); itCObTproc++)
		{
			int callKind = itCObTproc->first; //kind	
			//
            MapTypeCounter &MTCproc = itCObTproc->second;
			for (itMTCproc=MTCproc.begin(); itMTCproc!=MTCproc.end(); itMTCproc++)
			{
				BasicType::BasicTypes basicTypes = itMTCproc->first; //type
				long_long_t mult = itMTCproc->second;	//counter				
				
				HardwareOptions hoCurrentOperationAndType = p_PlatformSettings->GetValue(callKind, basicTypes);
				hoCurrentOperationAndType = hoCurrentOperationAndType * mult;
				hardwareOptionsProc = hardwareOptionsProc + hoCurrentOperationAndType;

				m_ProcedureHardwareOptions[proc]=hardwareOptionsProc;
			}
		}
	}
	return true;
}


}
}
