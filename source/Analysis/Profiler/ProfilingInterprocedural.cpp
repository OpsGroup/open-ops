#include "ProfilingInterprocedural.h"
#include "Analysis/CallGraph.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Profiling
{
  
ProgramAnalyser::ProgramAnalyser(ProfilingMode profilingMode)
	: m_ProfilingMode(profilingMode)
{
	p_InStmtLimIter = NULL;
	p_OutStmtLimIter = NULL;	
}
  
void ProgramAnalyser::SetLimits(StatementLimitsIteration* pInStmtLimIter, StatementLimitsIteration* pOutStmtLimIter)
{
	p_InStmtLimIter = pInStmtLimIter;
	p_OutStmtLimIter = pOutStmtLimIter;
}
  
bool ProgramAnalyser::Run(OPS::Reprise::RepriseBase* pRepriseBase)
{
	if (!pRepriseBase) 
		throw RuntimeError("Unit is null");
	if (pRepriseBase->is_a<ProgramUnit>())
	{
		ProgramUnit *programUnit = pRepriseBase->cast_ptr<ProgramUnit>();
		int translationUnitCount = programUnit->getUnitCount();
		for(int tu = 0; tu < translationUnitCount; ++tu)
			BuildProcList(&programUnit->getUnit(tu));
	}
	else 
		if (pRepriseBase->is_a<TranslationUnit>())
		{
			TranslationUnit *translationUnit = pRepriseBase->cast_ptr<TranslationUnit>();
			BuildProcList(translationUnit);
		}
		else
			throw RuntimeError("Incorrect unit");	
	GirthListProc();	
	return (m_SubDeclarations.size()>0);
}

void ProgramAnalyser::GirthListProc()
{
	VectorSubroutineDeclaration::iterator it;
	for(it=m_SubDeclarations.begin();it!=m_SubDeclarations.end();++it) 
	{
		SubroutineDeclaration* sd = (*it);
		if (sd->hasImplementation())
		{
			ProfilingResults profResSubDeclaration;
			WithinSubroutineAnalysis wsa(&sd->getBodyBlock(), &profResSubDeclaration, m_ProfilingMode
																	,&m_StmtCounterFull, &m_StmtCounterOnlyHeaders, &m_ProcCounter, p_InStmtLimIter, p_OutStmtLimIter);
            m_ProcCounter.SetProfilingResults(sd, profResSubDeclaration.CObT);
		}
	}
}

void ProgramAnalyser::BuildProcList(TranslationUnit *translationUnit)
{
	if (m_ProfilingMode.prfWithSubroutineCalls)
		BuildListWithCall(translationUnit);
	else
		BuildSimpleList(translationUnit);
}

void ProgramAnalyser::BuildSimpleList(TranslationUnit *translationUnit)
{	
	m_SubDeclarations.clear();
	for (Declarations::SubrIterator iter = translationUnit->getGlobals().getFirstSubr(); iter.isValid(); ++iter)
	{
		if (iter->hasImplementation())
		{
			m_SubDeclarations.push_back(&*iter);
		}
	}
}

bool ProgramAnalyser::BuildListWithCall(TranslationUnit *translationUnit) 
{
	CallGraph* pCallGraph;
	try
	{
		pCallGraph = new CallGraph(translationUnit);
	}
	catch(const OPS::Exception)
	{		
		return false; //throw RuntimeError("Failed to create a CallGraph.");  
	}

	if (!pCallGraph->getGraph().empty())
	{
		m_SubDeclarations.clear();
		const CallGraph::NodeMap &m_Graph = pCallGraph->getGraph();
		CallGraph::NodeMap::const_iterator c_it;

		//без вызовов
		for(c_it=m_Graph.begin();c_it!=m_Graph.end();++c_it) 
		{
			if (c_it->second->getCalls().size() == 0)
				m_SubDeclarations.push_back(c_it->second->getSubProc());
		}

		//с вызовами
		while (m_SubDeclarations.size() < m_Graph.size())
		{
			int counter = 0;
			for(c_it=m_Graph.begin();c_it!=m_Graph.end();++c_it) 
			{
				if(find(m_SubDeclarations.begin(), m_SubDeclarations.end(), c_it->second->getSubProc()) == m_SubDeclarations.end())
				{
					//проверка, что все вызовы уже обработаны
					bool flag = true;
					const CallGraph::SubProcCallMap& vCalls = c_it->second->getCalls();
					CallGraph::SubProcCallMap::const_iterator c_q;              
					for (c_q=vCalls.begin(); c_q!=vCalls.end(); ++c_q)
					{
						//if(find(m_SubDeclarations.begin(), m_SubDeclarations.end(), c_q->first) == m_SubDeclarations.end())
						bool isExist =  false;
						for(size_t j=0; j<m_SubDeclarations.size(); j++)
							if(m_SubDeclarations[j]->getName()==c_q->first->getName())
								isExist = true;
						if (!isExist)
							flag = false;
					}
					if (flag)
					{
						m_SubDeclarations.push_back(c_it->second->getSubProc());
						counter++;
					}
				}
			}
			if ((counter==0) && (m_SubDeclarations.size() < m_Graph.size()))
			{				
				return false; //throw RuntimeError("Recursion not supported.");
			}
		}
	}
	return true;
}

}
}
