#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Transforms/If/IfSplitting/IfSplitting.h"
#include "Backends/OutToC/OutToC.h"
#include "Reprise/Lifetime.h"
#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include <iostream>

int main()
{
	using namespace OPS::Transforms;
	using namespace OPS::TransformationsHub;
	using namespace OPS::Reprise;
	using namespace OPS::Frontend;
	using namespace std;
	
	cout<<"Testmod for IfSplitting started...\n";

	try
	{
		Frontend frontend;
		const CompileResult& result = frontend.compileSingleFile("Samples/sample07.c");

		if (result.errorCount() == 0)
		{
			cout << "Compiled successful.\n";
			
			OPS::Backends::OutToC writer(std::cout);
			writer.visit(frontend.getProgramUnit().getUnit(0));

			Declarations& decls = frontend.getProgramUnit().getUnit(0).getGlobals();
			SubroutineDeclaration& subroutine = *decls.getFirstSubr();//decls.getSubroutine(0);

			BlockStatement& bodyBlock = subroutine.getBodyBlock();

			cout<<"\nBody block: \n";

			writer.visit(bodyBlock);

			cout<<"\nTry to transform this operator: \n";

			IfStatement* ifStmt = dynamic_cast<IfStatement*>(&(*(bodyBlock.getFirst())));

			writer.visit(*ifStmt);

			IfSplitting ifSpitting;

			// Arguments
			ArgumentValues argumentValues;
			ArgumentValue firstArg(ArgumentValue::StmtIf);
			firstArg.setReprise(ifStmt);
			argumentValues.push_back(firstArg);
		
			ifSpitting.makeTransform(&frontend.getProgramUnit(), argumentValues);
			
			cout<<"Result of work:\n";
			writer.visit(frontend.getProgramUnit().getUnit(0));
		}
		
		cout << Coordinator::instance().getStatistics();

	}
	catch(RepriseError e)
	{
		cout<<"OPS Error! " + e.getMessage() + "\n";
	}
	catch(exception e)
	{
		cout<<"Some problems:\n\t"<<e.what();
	}

	cout<<"\nTestmod for IfSpitting finised...\n";

	system("pause");
	
	return 0;
}
