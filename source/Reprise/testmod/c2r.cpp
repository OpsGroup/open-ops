#include "c2r.h"

#include "Frontend/Frontend.h"
#include "FrontTransforms/CantoToReprise.h"
#include "Backends/OutToC/OutToC.h"
#include "Backends/RepriseXml/RepriseXml.h"
#include "Reprise/Lifetime.h"
#include <iostream>

using namespace std;
using namespace OPS;
using namespace OPS::Reprise;

static void DefaultParamValue();

void testMI2SB(void)
{
	DefaultParamValue();
}

void testContinue(void)
{
	cout << Coordinator::instance().getStatistics();
	{
		Frontend::Frontend frontend;
		const CompileResult& result = frontend.compileSingleFile("tests\\C2R\\continue1.c");
		if (result.errorCount() == 0)
		{
			cout << Coordinator::instance().getStatistics();
			cout << "Compiled successful.\n";
			OPS::Backends::OutToC writer(std::cout);
			writer.visit(frontend.getProgramUnit().getUnit(0));

            XmlBuilder xml(std::cout);
			Backends::RepriseXml::Options options;
			options.writeNCIDofParent = true;
			Backends::RepriseXml repXml(xml, options);
			repXml.visit(frontend.getProgramUnit().getUnit(0));
		}
	}
	cout << Coordinator::instance().getStatistics();

}


static void DefaultParamValue()
{
	Frontend::Frontend frontend;
	const CompileResult& result = frontend.compileSingleFile("tests\\C2R\\MI2SB\\DefaultParamValue.c");
	if (result.errorCount() == 0)
	{
		cout << "Compiled successful.\n";
		VariableDeclaration& varDecl = *frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr()->getDeclarations().getFirstVar();
		if (!varDecl.hasNonEmptyInitExpression())
		{
			varDecl.setInitExpression(*StrictLiteralExpression::createInt32(12));
		}
		else
			cout << "Unexpected sample program.";
		Frontend::C2R::convertVariablesInit(frontend.getProgramUnit());
		OPS::Backends::OutToC writer(std::cout);
		writer.visit(frontend.getProgramUnit().getUnit(0));
	}
}

