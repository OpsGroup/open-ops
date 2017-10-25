#include "Analysis/Profiler/ProfTextBuilder.h"
#include "Reprise/Units.h"
#include "Reprise/Expressions.h"
#include <iostream>

using namespace OPS::Reprise;

namespace OPS
{
namespace Profiling
{

ProfTextBuilder::ProfTextBuilder(OPS::Reprise::ProgramUnit* program, Profiler* profiler, ShowsProfilingResult spr, bool isOnlyForHeaders)
{
	m_BuildResultProfiling = new BuildResultProfiling(program, profiler, spr, isOnlyForHeaders);
}

ProfTextBuilder::~ProfTextBuilder(void)
{
	delete m_BuildResultProfiling;
}

std::string ProfTextBuilder::GetResult()
{
	return m_BuildResultProfiling->GetResult();
}

//***************************************************

BuildResultProfiling::BuildResultProfiling(OPS::Reprise::ProgramUnit* program, Profiler* profiler, ShowsProfilingResult spr, bool isOnlyForHeaders)
	:m_Profiler(profiler), m_Program(program), m_SPR(spr), m_IsOnlyForHeaders(isOnlyForHeaders)
{
	m_Offset = "";
	m_ProfResult.clear();	
	std::string SPRName = (m_SPR==PRF_TIME)? "Time" : ((m_SPR==PRF_SIZE_CODE)? "Size code" : "Size data");
	m_ProfResult<<"PROFILING RESULT: "<<SPRName<< std::endl<<std::endl;
	Declarations &decls = m_Program->getUnit(0).getGlobals();
	for (Declarations::SubrIterator iter = decls.getFirstSubr(); iter.isValid(); ++iter)
	{
		SubroutineDeclaration& currentFunc = (*iter);
		if (currentFunc.hasImplementation())
		{
			m_ProfResult<<m_Offset<<currentFunc.getName()<<"() ["<<GetResultProfiling(&currentFunc)<<"]"<<std::endl;
			BlockStatement& bodyBlock = currentFunc.getBodyBlock();			
			Run(&bodyBlock);
			m_ProfResult<<std::endl;
		}
	}
};

void BuildResultProfiling::Run(OPS::Reprise::RepriseBase* repriseBase)
{
	repriseBase->accept(*this);
}

void BuildResultProfiling::visit(OPS::Reprise::ExpressionStatement& exprStmt)
{
	std::string str_stmt = exprStmt.dumpState();
	str_stmt.erase(str_stmt.end()-1);
	m_ProfResult<<m_Offset<<str_stmt<<" ["<<GetResultProfiling(&exprStmt)<<"]"<<std::endl;
}

void BuildResultProfiling::visit(OPS::Reprise::BlockStatement& blockStmt)
{
	m_ProfResult<<m_Offset<<"{ ["<<GetResultProfiling(&blockStmt)<<"]"<<std::endl;
	std::string tmp = m_Offset;
	m_Offset += " ";
	BlockStatement::Iterator it = blockStmt.getFirst();
	for(; it.isValid(); ++it)
	{
		StatementBase& stmtCurrent = *it;
		Run(&stmtCurrent);
	}
	m_Offset = tmp;
	m_ProfResult<<m_Offset<<"}"<<std::endl;
}

void BuildResultProfiling::visit(OPS::Reprise::ForStatement& forStmt)
{
	std::string str_stmt = forStmt.dumpState();
	size_t found = str_stmt.find("{");
	if ((found!=std::string::npos) && (found>0))
		str_stmt.erase(str_stmt.begin()+found-1, str_stmt.end());
	m_ProfResult<<m_Offset<<str_stmt<<" ["<<GetResultProfiling(&forStmt)<<"]"<<std::endl;
	Run(&forStmt.getBody());
}

void BuildResultProfiling::visit(OPS::Reprise::WhileStatement& whileStmt)
{
	std::string str_stmt = whileStmt.dumpState();
	size_t found = str_stmt.find("{");
  if ((found!=std::string::npos) && (found>0))
		str_stmt.erase(str_stmt.begin()+found-1, str_stmt.end());
	m_ProfResult<<m_Offset<<str_stmt<<" ["<<GetResultProfiling(&whileStmt)<<"]"<<std::endl;
	Run(&whileStmt.getBody());
}	

void BuildResultProfiling::visit(OPS::Reprise::IfStatement& ifStmt)
{
	std::string str_stmt = ifStmt.dumpState();
	size_t found = str_stmt.find("{");
	if ((found!=std::string::npos) && (found>0))
		str_stmt.erase(str_stmt.begin()+found-1, str_stmt.end());
	m_ProfResult<<m_Offset<<str_stmt<<" ["<<GetResultProfiling(&ifStmt)<<"]"<<std::endl;
	Run(&ifStmt.getThenBody());	
	m_ProfResult<<m_Offset<<"else"<<std::endl;
	Run(&ifStmt.getElseBody());
}

void BuildResultProfiling::visit(OPS::Reprise::ReturnStatement& returnStmt)
{
  std::string str_stmt = returnStmt.dumpState();
	str_stmt.erase(str_stmt.end()-1);
	m_ProfResult<<m_Offset<<str_stmt<<" ["<<GetResultProfiling(&returnStmt)<<"]"<<std::endl;
}

long_long_t BuildResultProfiling::GetResultProfiling(OPS::Reprise::RepriseBase* repriseBase)
{
	switch(m_SPR) 
	{
		case PRF_TIME:				
				return m_Profiler->GetTimeCounter(repriseBase, m_IsOnlyForHeaders);
		case PRF_SIZE_CODE:
				return m_Profiler->GetSizeCommand(repriseBase, m_IsOnlyForHeaders);
		case PRF_SIZE_DATA:
				return m_Profiler->GetSizeData(repriseBase, m_IsOnlyForHeaders);
	}
	return 0;
}

}
}
