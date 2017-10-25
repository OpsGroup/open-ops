#include "Analysis/ControlFlowGraph.h"
#include "Analysis/Montego/AliasAnalysis/AliasImplementation.h"
#include "Analysis/Montego/AliasAnalysis/CanonicalForm.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Backends/OutToC/OutToC.h"
#include "Backends/RepriseXml/RepriseXml.h"
#include "ClangParser/clangParser.h"
#include "Frontend/Frontend.h"
#include "FrontTransforms/Resolver.h"
#include "OPS_Core/Kernel.h"
#include "Reprise/Reprise.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "OPS_Core/msc_leakcheck.h"//контроль утечек памяти должен находиться в конце всех include !!!

using namespace std;

using namespace OPS;
using namespace OPS::Reprise;
using namespace OPS::Montego;

std::string toDot(ControlFlowGraphExpr& cfg)
{
    std::ostringstream dumper;
    std::ostringstream edgesDumper;
    dumper << "digraph ControlFlowGraphExpr {" << std::endl;
    dumper << "node [shape = box, labeljust=\"l\"];" << std::endl;

    ControlFlowGraphExpr::BasicBlockList::const_iterator it = cfg.getBlocks().begin(),
            itEnd = cfg.getBlocks().end();

    for(; it != itEnd; ++it)
    {
        dumper << "N" << *it << "[ label = \"";
        for(size_t i = 0; i < (*it)->size(); ++i)
        {
            dumper << StrictLiteralExpression::stringToEscapedString(Backends::OutToC::expressionToString(*(*it)->at(i))) << "\\l";
        }
        dumper << "\"";
        if ((*it)->isEntry())
            dumper << ", fillcolor=green, style=filled";
        if ((*it)->isExit())
            dumper << ", fillcolor=red,style=filled";
        dumper << "];" << std::endl;
        for(size_t i = 0; i < (*it)->getOutBlocks().size(); ++i)
        {
            edgesDumper << "N" << *it << "->N" << (*it)->getOutBlocks()[i] << ";" << std::endl;
        }
    }

    dumper << edgesDumper.str();
    dumper << "}" << std::endl;
    return dumper.str();
}

void testCFG(TranslationUnit& unit)
{
    ControlFlowGraphExpr graph;
    ControlFlowGraphExprBuilder builder;

    vector<string> subs;
    //subs.push_back("empty");
    //subs.push_back("multiBlocks");
    //subs.push_back("linear");
    //subs.push_back("ifStmt");
    //subs.push_back("switchStmt");
    //subs.push_back("forStmt");
    //subs.push_back("forStmtGoto");
    //subs.push_back("whileStmt");
    //subs.push_back("doWhileStmt");
    /*
    subs.push_back("main");
    subs.push_back("recollect");
    subs.push_back("maxel");
    subs.push_back("write_to_file");
    subs.push_back("read_file");
    */
    subs.push_back("Verify");
    subs.push_back("DC");
    subs.push_back("main");
    subs.push_back("GenerateADC");
    subs.push_back("GetNextTuple");
    subs.push_back("CalculateVeiwSizes");

    for(size_t i = 0; i < subs.size(); ++i)
    {
        SubroutineDeclaration* sub = unit.getGlobals().findSubroutine(subs[i]);
        builder.build(sub->getDefinition().getBodyBlock(), graph);

        ofstream of(("cfgs\\" + subs[i] + ".dot").c_str());
        of << toDot(graph);
    }
}

typedef std::map<std::string, std::vector<std::string> > TestMap;

int main()
{
    OPS::Frontend::Frontend frontend;
    frontend.clangSettings() = clang::ClangParserSettings::msvcSettings();
    frontend.clangSettings().m_useStandardIncludes = true;

    frontend.clangSettings().initWithStdSettingsForCurrentPlatform();

    frontend.clangSettings().defineMacro("SPEC_CPU", "1");

    TestMap tests;
    tests["01_linpack"].push_back("tests/linpackc.new.c");
    tests["02_md"].push_back("tests/md.c");
    tests["03_whetstone"].push_back("tests/whetstone.c");
    tests["cfg"].push_back("tests/cfg.c");
    tests["04_smith"].push_back("tests/smith.c");
    tests["05_gazaryan"].push_back("tests/Gazaryan_mainBlock.c");
    tests["06_revrina"].push_back("tests/RevRinaBlock.c");
    tests["07_lbm"].push_back("tests/lbm/lbm.c");
    tests["07_lbm"].push_back("tests/lbm/main.c");
//    tests["08_dc"].push_back("tests/dc/dc_inline.c");
    tests["09_hirschberg"].push_back("tests/Hirschberg.c");
    tests["10_hirschberg2"].push_back("tests/Hirschberg2.c");
    tests["11_quagga"].push_back("tests/quagga/isis_spf.c");

    TestMap::iterator it = tests.begin();

    ofstream timings("logs/timings.log");

    for(; it != tests.end(); ++it)
    {
        frontend.clearSources();
        for(size_t i = 0; i < it->second.size(); ++i)
            frontend.addSourceFile(it->second[i]);

        timings << it->first << "\t";

        if (frontend.compile()) {
            dword start = OPS::getTickCount();
            apply4CanonicalTransforms(frontend.getProgramUnit());

            //Backends::OutToC writer(cout);
            //frontend.getProgramUnit().getUnit(0).accept(writer);

            //testCFG(frontend.getProgramUnit().getUnit(0));

            OccurrenceContainer occurs(frontend.getProgramUnit());
            AliasAnalysisOptions opts;
            opts.recursionType = Montego::RecursionByDecl;
            opts.debug = false;
            opts.meshingIfBranches = true;
            opts.cacheProcedureAnalys = false;
            AliasImplementation ai(frontend.getProgramUnit(), occurs, opts);

            try {
                ai.runAliasAnalysis();
            }
            catch (AliasCheckerException exception) {
                timings << "With exception [" << exception.getMessage() << "]" << "\t";
            }
            timings << OPS::getTickCount() - start << std::endl;

            ofstream results(("logs/" + it->first + ".log").c_str());
            results << ai.toString(true);
        }
        else
        {
            std::cerr << "Error compiling source file";
            for (int i = 0; i < frontend.getResultCount(); ++i)
                std::cerr << frontend.getResult(i).errorText();
        }
    }
    return 0;
}
