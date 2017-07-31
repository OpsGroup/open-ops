#include "Reprise/Service/DeepWalker.h"
#include "Frontend/Frontend.h"
#include "FrontTransforms/CantoToReprise.h"
#include "Reprise/ServiceFunctions.h"
#include "Backends/OutToC/OutToC.h"
#include "F2003Parser/FParser.h"
#include "Reprise/Lifetime.h"
#include <iostream>

//#include <stdio.h>

using namespace std;
using namespace OPS::Frontend;
using namespace OPS::Reprise;


class StatsWalker : public OPS::Reprise::Service::DeepWalker
{
public:


	
	void visit(Canto::HirFAltResultExpression&)
	{
		m_stats["HirFAltResultExpression"] += 1;
	}
	
	void visit(Canto::HirFAsteriskExpression&)
	{
		m_stats["HirFAsteriskExpression"] += 1;
	}
	
	void visit(Canto::HirFDimensionExpression&)
	{
		m_stats["HirFDimensionExpression"] += 1;
	}

	void visit(Canto::HirFArrayShapeExpression&)
	{
		m_stats["HirFArrayShapeExpression"] += 1;
	}
	
	void visit(Canto::HirFArrayIndexRangeExpression&)
	{
		m_stats["HirFArrayIndexRangeExpression"] += 1;
	}
	
	void visit(Canto::HirFImpliedDoExpression&)
	{
		m_stats["HirFImpliedDoExpression"] += 1;
	}
	
	void visit(Canto::HirFArgumentPairExpression&)
	{
		m_stats["HirFArgumentPairExpression"] += 1;
	}
	
	void visit(Canto::HirFIntrinsicCallExpression&)
	{
		m_stats["FIntrinsicCallExpression"] += 1;
	}
	
	void visit(Canto::HirFIntrinsicReferenceExpression&)
	{
		m_stats["IntrinsicReferenceExpression"] += 1;
	}

	void visit(Canto::HirFBasicType&)
	{
		m_stats["HirFBasicType"] += 1;
	}

	void visit(Canto::HirFArrayType&)
	{
		m_stats["HirFArrayType"] += 1;
	}


	void print()
	{
		for (TStats::const_iterator it = m_stats.begin(); it != m_stats.end(); ++it)
		{
			cout << it->first << ": " << it->second << "\n";
		}
	}

	typedef std::map<std::string, unsigned> TStats;

	TStats m_stats;
};


int main()
{
	{
		Frontend frontend;
		//frontend.options().convertGeneralExpressions = false;
		//frontend.options().convertOtherExpressions = false;
//		frontend.options().convertBreakContinue = false;

//		string filePath = "../../../bin/samples/ACELAN/composeV4_100_corr_phi.FOR";
//		string filePath = "tests/c2r1.c";
		string filePath = "tests/array_struct1.c";
		const CompileResult& result = frontend.compileSingleFile(filePath);

		if (result.errorCount() == 0)
		{
			//StatsWalker statsWalker;
			//frontend.getProgramUnit().accept(statsWalker);
			//statsWalker.print();

			//ReprisePtr<SubroutineDeclaration> decl(frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(0).clone());
			//decl.reset(0);

			//C2R::convertTypes(frontend.getProgramUnit());
			//C2R::convertExpressions(frontend.getProgramUnit(), true, true, true);
			//C2R::convertLiterals(frontend.getProgramUnit());
			//C2R::convertVariablesInit(frontend.getProgramUnit());
			//C2R::convertBreakContinue(frontend.getProgramUnit());

			//C2R::convertHirFArrays(frontend.getProgramUnit());

			//statsWalker.m_stats.clear();
//			frontend.getProgramUnit().accept(statsWalker);
			cout << "After Canto:\n";
//			statsWalker.print();


			cout << "Compiled successful.\n";
			OPS::Backends::OutToC writer(std::cout);
			writer.visit(frontend.getProgramUnit().getUnit(0));
			cout << "=========================== dumpState =========================" << endl;
			cout << frontend.getProgramUnit().dumpState();
			cout << "=========================== convertTest ======================="  << endl;

		}
		cout << Coordinator::instance().getStatistics();
	}
	cout << OPS::Reprise::Coordinator::instance().getStatistics();

	return 0;
}
