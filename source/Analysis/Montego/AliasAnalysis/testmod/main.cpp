#include "AliasAnalysisTester.h"
#include "Analysis/Montego/AliasAnalysis/AliasImplementation.h"
#include "Analysis/Montego/AliasAnalysis/CanonicalForm.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Backends/OutToC/OutToC.h"
#include "Backends/RepriseXml/RepriseXml.h"
#include "ClangParser/clangParser.h"
#include "Frontend/Frontend.h"
#include "FrontTransforms/Resolver.h"
#include "Reprise/Reprise.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "OPS_Core/msc_leakcheck.h"//контроль утечек памяти должен находиться в конце всех include !!!

using namespace std;

using namespace OPS;
using namespace OPS::Montego;
using namespace OPS::Reprise;

void test3()
{
    OPS::Frontend::Frontend frontend;
    clang::ClangParserSettings& sets = frontend.clangSettings();
    //sets.addIncludePath("test\\include");
    //sets.defineMacro("__int64","long int");
    //string testSource = "test\\test3.c";
    //string testSource = "test\\test4.c";
    string testSource = "test/test5.c";
    frontend.addSourceFile(testSource);
    auto compileResult = frontend.compile();
    if (compileResult)
    {
        OPS::Transforms::Resolver resolver;
        resolver.setProgram(frontend.getProgramUnit());
        resolver.resolve();

        apply4CanonicalTransforms(frontend.getProgramUnit());

        Backends::OutToC writer(cout);
        //frontend.getProgramUnit().getUnit(0).accept(writer);

        // std::ofstream res("xml.xml");
        // XmlBuilder builder(res);
        // Backends::RepriseXml reprXml(builder);
        // frontend.getProgramUnit().getUnit(0).accept(reprXml);
        // res.close();

        AliasAnalysisOptions opt;
        opt.debug = true;

        OccurrenceContainer occurs(frontend.getProgramUnit());
        AliasImplementation ai(frontend.getProgramUnit(), occurs, opt);

        ai.runAliasAnalysis();
        //DependenceGraph graph(&frontend.getProgramUnit().getUnit(0).getGlobals().getLastSubr()->getBodyBlock());
        cout << "Alias analysis result: \n";
        cout << ai.toString() << "\n";

        BlockStatement& mainBody = frontend.getProgramUnit().getUnit(0).getGlobals().getLastSubr()->getBodyBlock();
        VariableDeclaration* a = mainBody.getDeclarations().findVariable("a");
        VariableDeclaration* b = mainBody.getDeclarations().findVariable("b");
        VariableDeclaration* c = mainBody.getDeclarations().findVariable("c");
        VariableDeclaration* pa = mainBody.getDeclarations().findVariable("pa");
        VariableDeclaration* pb = mainBody.getDeclarations().findVariable("pb");
        VariableDeclaration* pc = mainBody.getDeclarations().findVariable("pc");
        cout << "isAlias(pa, pb): " << ai.isAlias(*pa, *pb) << endl;
        cout << "isAlias(a, pa): " << ai.isAlias(*a, *pa) << endl;
        cout << "isAlias(b, pa): " << ai.isAlias(*b, *pa) << endl;
        cout << "isAlias(pa, b): " << ai.isAlias(*pa, *b) << endl;
    }
    else
    {
        cout << "Error: '" << testSource << "' not compile." << endl;
    }
}

int main()
{
    /*ofstream testfile("asdklfasdlkfj.log");
    testfile << "Anytext" << endl;
    testfile.close();*/
    test3();
    /*
	OPS::Frontend::Frontend frontend;
	clang::ClangParserSettings& sets = frontend.clangSettings();
	sets.addIncludePath("test\\include");
	//sets.defineMacro("__int64","long int");
    string testSource = "test\\test3.c";
	frontend.addSourceFile(testSource);
    auto compileResult = frontend.compile();
	if (compileResult)
	{

        OPS::Transforms::Resolver resolver;
		resolver.setProgram(frontend.getProgramUnit());
		resolver.resolve();

		apply4CanonicalTransforms(frontend.getProgramUnit());

		//Backends::OutToC writer(cout);
		//frontend.getProgramUnit().getUnit(0).accept(writer);

		//
		//
        //std::ofstream res("xml.xml");
        //XmlBuilder builder(res);
		//Backends::RepriseXml reprXml(builder);
		//frontend.getProgramUnit().getUnit(0).accept(reprXml);
		//res.close();
		//


		OccurrenceContainer occurs(frontend.getProgramUnit());
        AliasImplementation ai(frontend.getProgramUnit(), occurs);

        ai.runAliasAnalysis();
		//DependenceGraph graph(&frontend.getProgramUnit().getUnit(0).getGlobals().getLastSubr()->getBodyBlock());
		cout << "Alias analysis result: \n";
		cout << ai.toString() << "\n";

		BlockStatement& mainBody = frontend.getProgramUnit().getUnit(0).getGlobals().getLastSubr()->getBodyBlock();
		VariableDeclaration* a = mainBody.getDeclarations().findVariable("a");
		VariableDeclaration* b = mainBody.getDeclarations().findVariable("b");
		VariableDeclaration* c = mainBody.getDeclarations().findVariable("c");
		cout << ai.isAlias(*a,*b) << "  " << ai.isAlias(*a,*c) << "  " << ai.isAlias(*b,*c) << "\n";
	}
    else
    {
        cout << "Error: '" << testSource << "' not compile." << endl;
    }
    */
    /*
    int testNum = 2;

	if (testNum == 1)
	{
		AliasAnalysisTester ait;
		std::string outputMes;
		int code = ait.runTest("test\\470.lbm.c", outputMes);
		//int code = ait.runTest("test\\test1.c", outputMes);
		cout << "Code = " << code << "\n";
		cout << outputMes;
	}
    */
    /*
    if (testNum == 2)
	{
		OPS::Frontend::Frontend frontend;
		//const CompileResult& result = frontend.compileSingleFile("test\\test1.c");
		clang::ClangParserSettings& sets = frontend.clangSettings();
		sets.addIncludePath("test\\include");
		sets.defineMacro("__int64","long int");
		frontend.addSourceFile("test\\470.lbm.c");

		if (frontend.compile())
		{
            OPS::Transforms::Resolver resolver;
			resolver.setProgram(frontend.getProgramUnit());
			resolver.resolve();


            std::ofstream res("xml0.xml");
            XmlBuilder builder(res);
			Backends::RepriseXml reprXml(builder);
			frontend.getProgramUnit().getUnit(0).accept(reprXml);
			res.close();

			apply4CanonicalTransforms(frontend.getProgramUnit().getUnit(0));

			OccurrenceContainer occurs(frontend.getProgramUnit());
            AliasImplementation ai(frontend.getProgramUnit(), occurs);
            ai.runAliasAnalysis();

			cout << "Alias analysis result: \n";
			cout << ai.toString() << "\n";

			Backends::OutToC writer(cout);
			frontend.getProgramUnit().getUnit(0).accept(writer);
		}
	}
    */
	return 0;
}
