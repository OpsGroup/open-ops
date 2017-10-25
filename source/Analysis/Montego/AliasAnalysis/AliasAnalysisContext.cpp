#include <iostream>

#include "Analysis/ControlFlowGraph.h"
#include "AliasAnalysisContext.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "ProgramState.h"

#include "Navigation.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "MemoryCellContainer.h"

namespace OPS
{
namespace Montego
{
ControlFlowGraphBuilder::~ControlFlowGraphBuilder()
{
	BlockToGraphMap::iterator it = m_blockToGraph.begin();
	for(; it != m_blockToGraph.end(); ++it)
	{
		delete it->second;
	}
}

ControlFlowGraphExpr& ControlFlowGraphBuilder::buildControlFlowGraph(const Reprise::BlockStatement &block)
{
	BlockToGraphMap::const_iterator it = m_blockToGraph.find(&block);
	if (it == m_blockToGraph.end())
	{
        ControlFlowGraphExpr* graphExpr = new ControlFlowGraphExpr;

        ControlFlowGraphExprBuilder builder;
        builder.build(const_cast<BlockStatement&>(block), *graphExpr);

        m_blockToGraph.insert(std::make_pair(&block, graphExpr));
        return *graphExpr;
	}
	else
		return *it->second;
}

AliasAnalysisContext::AliasAnalysisContext(ProgramUnit& pu, OccurrenceContainer* occurs, AliasAnalysisOptions options)
	:m_navigation(pu, options)
	,m_occurs(occurs)
	,m_options(options)
	,m_runCache()
{
    //если контейнер был построен не для всей программы, то его надо расширить.
    //При этом существующие вхождения не должны быть затронуты
    if (occurs->getProgramPart() != &pu)
        occurs->addAllBasicOccurrencesIn(pu, false);

    m_memoryCellContainer = new MemoryCellContainer();
    m_aliasInformationContainer = new AliasInformationContainer(*m_memoryCellContainer);
}

AliasAnalysisContext::~AliasAnalysisContext()
{
    delete m_memoryCellContainer; m_memoryCellContainer = 0;
    delete m_aliasInformationContainer; m_aliasInformationContainer = 0;
    // do not remove
}

bool AliasAnalysisContext::isSafeSubroutine(OPS::Reprise::SubroutineDeclaration *decl)
{
    SubroutineSet::const_iterator it = m_options.safeFuncs.find(decl);

    if (it != m_options.safeFuncs.end())
        return true;

    it = m_options.unsafeFuncs.find(decl);
    if (it != m_options.unsafeFuncs.end())
        return false;

    if (m_options.treatUnknownFuncsAsUnsafe)
	{
		return false;
	}
	else
	{
#if OPS_BUILD_DEBUG
		std::string s =decl->getName();
        if (s!="printf" && s!="fprintf" && s!="_vswprintf_c_l" && s!="strnlen" && s!="wcsnlen" && s!="_wctime64"
			&& s!="_wctime64_s" && s!="_localtime64_s" && s!="_mktime64" && s!="_mkgmtime64" && s!="_time64"
            && s!="clock" && s!="_difftime64" && s!="_ctime64" && s!="_ctime64_s" && s!="_gmtime64"
            && s!="_gmtime64_s" && s!="_localtime64" && s!="free" && s!="strcmp" && s!="polybench_timer_print" && s!="polybench_timer_stop"
            && s!="polybench_timer_start" && s!="sqrt")
            std::cout << "Warning: Function " + decl->getName() + " has no implementation and was treated as safe!\n";
#endif
		m_options.safeFuncs.insert(decl);
		return true;
	}
}

}
}
