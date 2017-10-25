#include "Reprise/Reprise.h"
#include "ClangParser/clangParser.h"
#include "Frontend/Frontend.h"
#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/LatticeGraph/ElemLatticeGraph.h"	
#include "Analysis/LatticeGraph/ExtendedQuast.h"	
#include "Analysis/LatticeGraph/ParamPoint.h"	
#include "Analysis/LatticeGraph/RationalNumber.h"	
#include "Analysis/LatticeGraph/ComplexCondition.h"	
#include "Analysis/DepGraph/Status.h"
#include "Backends/RepriseXml/RepriseXml.h"
#include "Backends/OutToC/OutToC.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraphArc.h"
#include "Analysis/Montego/DependenceGraph/IndexAnalysis.h"

#include <string>
#include <iostream>
#include<fstream>

#include "OPS_Core/msc_leakcheck.h"//контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS::Reprise;
using namespace OPS::Montego;
using namespace LatticeGraph;
using namespace std;

std::vector<BasicOccurrencePtr> getAllxs(OccurrenceContainer& ocont)
{
    std::vector<BasicOccurrencePtr> result;
    std::vector<BasicOccurrencePtr> all = ocont.getAllBasicOccurrencesIn(ocont.getProgramPart());
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (all[i]->getName().m_varDecl->getName() == (std::string)"x")
            result.push_back(all[i]);
    }
    return result;
}

/*
std::vector<DependenceGraphAbstractArc> getAllArcsBetween(BasicOccurrencePtr srcEntry, BasicOccurrencePtr depEntry, const DependenceGraph& depgraph)
{
    std::vector<DependenceGraphAbstractArc> result;    
    DependenceGraph::ArcList al = depgraph.getAllArcsBetween(*srcEntry->getParentStatement(), *depEntry->getParentStatement());
    DependenceGraph::ArcList::iterator it;
    for (it = al.begin(); it != al.end(); ++it)
    {
        if (((*it)->getStartVertex().getSourceOccurrence() == srcEntry) &&
            ((*it)->getEndVertex().getSourceOccurrence() == depEntry) )
                    result.push_back(**it);
    }
    return result;
};
*/

int main_func();
int mainForAlias();

int main()
{
    int res = main_func();
    //_CrtDumpMemoryLeaks();
    return res;
}

int main_func()
{
    //_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	OPS::Backends::OutToC writer(std::cout);
    OPS::Frontend::Frontend frontend;
    frontend.compileSingleFile("test\\test1.c"); 
    BlockStatement& mainBodyBlock = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr()->getBodyBlock() ;

    DependenceGraph depgraph(*mainBodyBlock.getFirst());
	depgraph.removeCounterArcs();
	//AliasInterface* ai = depgraph.getAliasInterface().get();
    //std::vector<BasicOccurrencePtr> xs = getAllxs(*(depgraph.getOccurrenceContainer()));
	//BasicOccurrencePtr srcEntry = xs[0], depEntry = xs[1];


    //std::vector<DependenceGraphAbstractArc> arcs = getAllArcsBetween(srcEntry, depEntry, depgraph);
    //std::vector<bool> depSup = findDepSupp(arcs[0], depgraph);
    //bool arcWasRemoved = depgraph.refineSelectedArcWithLatticeGraph(arcs[0]);
    //cout << "arc num = " << arcs.size() << "\n";
    //cout << "arcWasRemoved = " << arcWasRemoved << "\n";
    
    
    //std::vector<OPS::Reprise::VariableDeclaration*> arbitraryParamVector;
    //BlockStatement::Iterator itt = mainBodyBlock.getFirst();
    //BlockStatement& block = itt->cast_to<ForStatement>().getBody();
    //ElemLatticeGraph latTrue(mainBodyBlock, *ai, *srcEntry, *depEntry, DependenceGraphTrueArc::AT_OUTPUT_DEPENDENCE, true);
    //ElemLatticeGraph latAnti(*srcEntry,*depEntry,ANTIDEP,false);
    //ElemLatticeGraph latOut(*srcEntry,*depEntry,OUTPUTDEP,false);
    //std::cout << latTrue.toString()<<"\n";
    //std::ofstream res("latticeGraph.txt");
    //res<<latTrue.toString()<<"\n";

    //    _CrtDumpMemoryLeaks();

    char b;
    std::cin>>b;
    return 0;

}
