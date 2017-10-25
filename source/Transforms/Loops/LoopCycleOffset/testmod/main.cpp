#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Transforms/Loops/LoopCycleOffset/LoopCycleOffset.h"
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
	
	cout << "Testmod for LoopCycleOffset started..."  << endl;

	try
	{
		Frontend frontend;
		const CompileResult& result = frontend.compileSingleFile("D:/programing/diploma/ops/bin/samples/Transforms/Loops/LoopCycleOffset/sample05.c");

		if (result.errorCount() == 0)
		{
 			cout << "Compiled successful." << endl;
			
			OPS::Backends::OutToC writer(std::cout);
			writer.visit(frontend.getProgramUnit().getUnit(0));

			Declarations& decls = frontend.getProgramUnit().getUnit(0).getGlobals();
			SubroutineDeclaration& subroutine = *decls.getFirstSubr();//decls.getSubroutine(0);

			BlockStatement& bodyBlock = subroutine.getBodyBlock();

			cout << endl << endl << "Body block: " << endl;

			writer.visit(bodyBlock);

			cout << endl << endl << "Try to transform this operator: " << endl;

			ForStatement* forStmt = (*(bodyBlock.getFirst())).cast_ptr<ForStatement>();
			writer.visit(*forStmt);

			LoopCycleOffset loop_transform;

			// Arguments
			ArgumentValues argumentValues;
			ArgumentValue firstArg(ArgumentValue::StmtFor);
			//ArgumentValue secondArg(ArgumentValue::Bool);
			firstArg.setReprise(forStmt);
			//secondArg.setBool(false);
			argumentValues.push_back(firstArg);
			//argumentValues.push_back(secondArg);
		
			loop_transform.makeTransform(&frontend.getProgramUnit(), argumentValues);
			
			cout << endl << endl << "Result of work: " << endl;
			writer.visit(frontend.getProgramUnit().getUnit(0));			
		}
		
		//cout << Coordinator::instance().getStatistics();

	}
	catch(RepriseError e)
	{
		cout<<"OPS Error! " + e.getMessage() + "\n";
	}
	catch(exception e)
	{
		cout<<"Some problems:\n\t"<<e.what();
	}

	cout << endl << "Testmod for LoopCycleOffset finised..." << endl;

	system("pause");
	
	return 0;
}
