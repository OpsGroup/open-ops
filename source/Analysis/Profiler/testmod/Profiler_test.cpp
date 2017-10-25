#include <string>
#include <iostream>
#include <fstream>

#include "Frontend/Frontend.h"
#include "Analysis/Profiler/Profiler.h"
#include "OPS_Core/IO.h"
#include "OPS_Core/Kernel.h"

using namespace std;
using namespace OPS;
using namespace Reprise;
using namespace Frontend;
using namespace OPS::Profiling;

const int ViewDetail = 1; /*	1 - основные результаты
															2 - детализация по операциям 
															3 - диалоговое профилирование  */

int main(int argc, char* argv[])
{
	bool isOnlyForHeaders= false;	//результат только для заголовков
	OPS::Frontend::Frontend frontend;
  frontend.compileSingleFile("samples\\test.c");
  std::wstring FullPathToFile = IO::combinePath(getStartupPath(), L"../../samples/PlatformSettings.xml");

	if (ViewDetail==1) //Основные результаты
	{
		OPS::Profiling::DialogProfiler h;
		if (h.Run(&frontend.getProgramUnit()))
		{
			OPS::Profiling::Profiler p(FullPathToFile);
			if (p.Run(h))
			{
				ofstream ofile("samples\\result.txt", ios::out);
				ofile<<"===== PROFILING RESULT: ====="<<std::endl;
				Declarations::ConstSubrIterator constSubIt = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr();
				for (; constSubIt.isValid(); constSubIt++)
				{
					SubroutineDeclaration *pCurrentFunc = const_cast<SubroutineDeclaration*>(&(*constSubIt));
					BlockStatement& bodyBlock = pCurrentFunc->getBodyBlock();
					ofile<<std::endl<<"PROCEDURE: "<<pCurrentFunc->getName()<<" : [Time="<<p.GetTimeCounter(pCurrentFunc)<<" ,SizeC="<<p.GetSizeCommand(pCurrentFunc)<<" ,SizeD="<<p.GetSizeData(pCurrentFunc)<<"]"<<std::endl;
					BlockStatement::Iterator iter = bodyBlock.getFirst();
					for (;iter.isValid();++iter)
					{
						StatementBase& sb = (*iter);      
						ofile<<std::endl<<"STMT: "<<sb.dumpState()<<" : [Time="<<p.GetTimeCounter(&sb, isOnlyForHeaders)<<" ,SizeC="<<p.GetSizeCommand(&sb, isOnlyForHeaders)<<" ,SizeD="<<p.GetSizeData(&sb, isOnlyForHeaders)<<"]"<<std::endl;
					}
				}
			}
		}
	}

	if (ViewDetail==2) //Информация об операциях
	{
		OPS::Profiling::DialogProfiler h;
		if (h.Run(&frontend.getProgramUnit()))
		{
			OPS::Profiling::Profiler p(FullPathToFile);
			if (p.Run(h))
			{
				ofstream ofile("samples\\result.txt", ios::out);
				ofile<<"===== PROFILING RESULT: ====="<<std::endl;
				Declarations::ConstSubrIterator constSubIt = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr();
				for (; constSubIt.isValid(); constSubIt++)
				{
					SubroutineDeclaration *pCurrentFunc = const_cast<SubroutineDeclaration*>(&(*constSubIt));
					BlockStatement& bodyBlock = pCurrentFunc->getBodyBlock();
					ofile<<std::endl<<"PROCEDURE: "<<pCurrentFunc->getName()<<" : [Time="<<p.GetTimeCounter(pCurrentFunc)<<" ,SizeC="<<p.GetSizeCommand(pCurrentFunc)<<" ,SizeD="<<p.GetSizeData(pCurrentFunc)<<"]"<<std::endl;
					BlockStatement::Iterator iter = bodyBlock.getFirst();
					for (;iter.isValid();++iter)
					{
						StatementBase& sb = (*iter);      
						ofile<<std::endl<<"STMT: "<<sb.dumpState()<<" : [Time="<<p.GetTimeCounter(&sb, isOnlyForHeaders)<<" ,SizeC="<<p.GetSizeCommand(&sb, isOnlyForHeaders)<<" ,SizeD="<<p.GetSizeData(&sb, isOnlyForHeaders)<<"]"<<std::endl;
					}
				}

				ofile<<std::endl<<"===== OPERATION: ====="<<std::endl;
				Declarations::ConstSubrIterator constSubItOp = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr();
				for (; constSubItOp.isValid(); constSubItOp++)
				{
					SubroutineDeclaration *opCurrentFunc = const_cast<SubroutineDeclaration*>(&(*constSubItOp));
					BlockStatement& opBodyBlock = opCurrentFunc->getBodyBlock();
					CounterOperationsByType cobtProc = h.GetCounterOperationsByType(opCurrentFunc);
					CounterOperationsByType::iterator itCObT;
					MapTypeCounter::iterator itMTC;

					ofile<<std::endl<<"PROCEDURE: "<<opCurrentFunc->getName()<<std::endl;
					for (itCObT=cobtProc.begin(); itCObT!=cobtProc.end(); itCObT++)
					{
						MapTypeCounter &MTC = itCObT->second;
						for (itMTC=MTC.begin(); itMTC!=MTC.end(); itMTC++)
						{
							ofile<<"Opepation="<<itCObT->first<<" ,Type="<<itMTC->first<<" ,Count="<<itMTC->second<<std::endl;
						}
					}

					BlockStatement::Iterator iterOp = opBodyBlock.getFirst();
					for (;iterOp.isValid();++iterOp)
					{
						StatementBase& sbOp = (*iterOp);        
						CounterOperationsByType cobtStmt = h.GetCounterOperationsByType(&sbOp, isOnlyForHeaders);
						CounterOperationsByType::iterator itCObTstmt;
						MapTypeCounter::iterator itMTCstmt;       
						ofile<<std::endl<<"STMT: "<<sbOp.dumpState();
						for (itCObTstmt=cobtStmt.begin(); itCObTstmt!=cobtStmt.end(); itCObTstmt++)
						{
							MapTypeCounter &MTC = itCObTstmt->second;
							for (itMTCstmt=MTC.begin(); itMTCstmt!=MTC.end(); itMTCstmt++)
							{
								ofile<<"Opepation="<<itCObTstmt->first<<" ,Type="<<itMTCstmt->first<<" ,Count="<<itMTCstmt->second<<std::endl;
							}
						}
					}
				}
			}
		}
	}

	if (ViewDetail==3) //Диалоговое профилирование
	{
		OPS::Profiling::DialogProfiler h;
		if (h.Run(&frontend.getProgramUnit()))
		{
			StatementLimitsIteration SLI = h.GetStatementLimitsIteration();
			StatementLimitsIteration::iterator itSLI=SLI.begin();
			for (; itSLI!=SLI.end(); itSLI++)
			{
				itSLI->second.min= 100;
				itSLI->second.max= 200;		
			}
			h.SetStatementLimitsIteration(SLI); 
			h.Run(&frontend.getProgramUnit());

			OPS::Profiling::Profiler p(FullPathToFile);
			if (p.Run(h))
			{
				ofstream ofile("samples\\result.txt", ios::out);
				ofile<<"===== PROFILING RESULT: ====="<<std::endl;
				Declarations::ConstSubrIterator constSubIt = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr();
				for (; constSubIt.isValid(); constSubIt++)
				{
					SubroutineDeclaration *pCurrentFunc = const_cast<SubroutineDeclaration*>(&(*constSubIt));
					BlockStatement& bodyBlock = pCurrentFunc->getBodyBlock();
					ofile<<std::endl<<"PROCEDURE: "<<pCurrentFunc->getName()<<" : [Time="<<p.GetTimeCounter(pCurrentFunc)<<" ,SizeC="<<p.GetSizeCommand(pCurrentFunc)<<" ,SizeD="<<p.GetSizeData(pCurrentFunc)<<"]"<<std::endl;
					BlockStatement::Iterator iter = bodyBlock.getFirst();
					for (;iter.isValid();++iter)
					{
						StatementBase& sb = (*iter);      
						ofile<<std::endl<<"STMT: "<<sb.dumpState()<<" : [Time="<<p.GetTimeCounter(&sb, isOnlyForHeaders)<<" ,SizeC="<<p.GetSizeCommand(&sb, isOnlyForHeaders)<<" ,SizeD="<<p.GetSizeData(&sb, isOnlyForHeaders)<<"]"<<std::endl;
					}
				}

				ofile<<std::endl<<"===== OPERATION: ====="<<std::endl;
				Declarations::ConstSubrIterator constSubItOp = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr();
				for (; constSubItOp.isValid(); constSubItOp++)
				{
					SubroutineDeclaration *opCurrentFunc = const_cast<SubroutineDeclaration*>(&(*constSubItOp));
					BlockStatement& opBodyBlock = opCurrentFunc->getBodyBlock();
					CounterOperationsByType cobtProc = h.GetCounterOperationsByType(opCurrentFunc);
					CounterOperationsByType::iterator itCObT;
					MapTypeCounter::iterator itMTC;

					ofile<<std::endl<<"PROCEDURE: "<<opCurrentFunc->getName()<<std::endl;
					for (itCObT=cobtProc.begin(); itCObT!=cobtProc.end(); itCObT++)
					{
						MapTypeCounter &MTC = itCObT->second;
						for (itMTC=MTC.begin(); itMTC!=MTC.end(); itMTC++)
						{
							ofile<<"Opepation="<<itCObT->first<<" ,Type="<<itMTC->first<<" ,Count="<<itMTC->second<<std::endl;
						}
					}

					BlockStatement::Iterator iterOp = opBodyBlock.getFirst();
					for (;iterOp.isValid();++iterOp)
					{
						StatementBase& sbOp = (*iterOp);        
						CounterOperationsByType cobtStmt = h.GetCounterOperationsByType(&sbOp, isOnlyForHeaders);
						CounterOperationsByType::iterator itCObTstmt;
						MapTypeCounter::iterator itMTCstmt;       
						ofile<<std::endl<<"STMT: "<<sbOp.dumpState();
						for (itCObTstmt=cobtStmt.begin(); itCObTstmt!=cobtStmt.end(); itCObTstmt++)
						{
							MapTypeCounter &MTC = itCObTstmt->second;
							for (itMTCstmt=MTC.begin(); itMTCstmt!=MTC.end(); itMTCstmt++)
							{
								ofile<<"Opepation="<<itCObTstmt->first<<" ,Type="<<itMTCstmt->first<<" ,Count="<<itMTCstmt->second<<std::endl;
							}
						}
					}
				}
			}
		}
	}

	system("pause");
}
