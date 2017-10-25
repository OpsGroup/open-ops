#include "Reprise/Reprise.h"
#include "Reprise/Lifetime.h"
#include "Frontend/Frontend.h"
#include "Analysis/DominanceFrontierGraph.h"
#include "Backends/OutToC/OutToC.h"
#ifdef _MSC_VER
#pragma warning(disable: 4099)
#endif

#include <iostream>
#include <sstream>
using namespace OPS::Reprise;
using namespace OPS::Frontend;
using namespace OPS::Analysis::ControlFlowGraphs::DominanceFrontierGraph;
using namespace OPS::Analysis::ControlFlowGraphs;
using namespace OPS::Analysis::ControlFlowGraphs::SSAControlFlowGraph;

using namespace std;


std::string statementGraphToSting(StatementGraph& toPrint, ControlFlowGraphEx& mainGraph)
{
	std::string toReturn;
	for(StatementGraph::iterator from = toPrint.begin();
		from != toPrint.end();
		++from)
	{
		std::stringstream ss;
		ss<<mainGraph.getBlockNum(from->first);
		toReturn += ss.str() + " => ";
		for(StatementVectorList::iterator to = from->second->begin();
			to != from->second->end();
			++to)
		{
			std::stringstream ss;
			ss<<mainGraph.getBlockNum(*to);
			toReturn += ss.str() + ";";
		}
		toReturn += "\n";
	}
	return toReturn;
}

std::string stashToString(Stash& stash)
{
	std::ostringstream sstream;
	OPS::Backends::OutToC writer(sstream);
	if(stash.getAsDeclaration()) stash.getAsDeclaration()->accept(writer);
	else if (stash.getAsExpression()) stash.getAsExpression()->accept(writer);
	else if (stash.getAsStatement()) stash.getAsStatement()->accept(writer);
	return sstream.str();
}

std::string cfGraphBlocksToString(StatementVectorList& blocks, ControlFlowGraphEx& mainGraph)
{
	std::string toReturn;
	for(StatementVectorList::iterator block = blocks.begin();
		block != blocks.end();
		++block)
	{
		std::stringstream ss;
		ss<<mainGraph.getBlockNum(*block);
		toReturn = toReturn + ss.str();
		toReturn += " = {";
		for(StatementVector::iterator stmt = (*block)->begin();
			stmt != (*block)->end();
			++stmt )
		{
			toReturn += stashToString(*stmt) + "\n";
		}
		toReturn += "}\n\n";
	}
	return toReturn;
}


int main()
{

	
	std::cout<<"Testmod for SSAFrom started...\n";

	try
	{
		Frontend frontend;
		const CompileResult& result = frontend.compileSingleFile("Samples/Loop.cpp");

		if (result.errorCount() == 0)
		{
			cout << "Compiled successful.\n";
			
			OPS::Backends::OutToC writer(std::cout);
//			writer.visit(frontend.getProgramUnit().getUnit(0));

			Declarations& decls = frontend.getProgramUnit().getUnit(0).getGlobals();
			SubroutineDeclaration& subroutine = *decls.getFirstSubr();


			BlockStatement& bodyBlock = subroutine.getBodyBlock();
 			try
 			{
				DominanceFrontier dfgraph(bodyBlock);
				ControlFlowGraphEx& cfgex = dfgraph.getCFGex();				
				writer.visit(bodyBlock);

				cout<<endl<<"---Blocks----------------------------------"<<endl;				
				std::string blocks_out = cfGraphBlocksToString(cfgex.getSubblocks(), cfgex);
				cout<<blocks_out;

				cout<<endl<<"---CFGraph---------------------------------"<<endl;				
				std::string graph_out = statementGraphToSting(cfgex.getSucc(), cfgex);				
				cout<<graph_out;

				cout<<endl<<"---Dominators------------------------"<<endl;				
				graph_out = statementGraphToSting(dfgraph.getDoms(), cfgex);				
				cout<<graph_out;

				cout<<endl<<"---DominanceFrontier-----------------"<<endl;				
				graph_out = statementGraphToSting(dfgraph.getDomF(), cfgex);				
				cout<<graph_out;



				


 			}
 			catch(OPS::Exception e)
 			{
 				cout<<"OPS exception: "<<e.getMessage()<<endl;
 			}

			


		}
		
		cout << OPS::Reprise::Coordinator::instance().getStatistics();

	}
	catch(OPS::Reprise::RepriseError e)
	{
		std::cout<<"OPS Error! " + e.getMessage() + "\n";
	}
	catch(exception e)
	{
		cout<<"Some problems:\n\t"<<e.what();
	}

	std::cout<<"\nTestmod for SSAForm finised...\n";

	system("pause");
	
	return 0;
}
