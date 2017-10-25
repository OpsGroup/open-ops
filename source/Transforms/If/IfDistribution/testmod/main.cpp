#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Transforms/If/IfDistribution/IfDistribution.h"
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
	
	std::cout<<"Testmod for IfDistribution started...\n";

	try
	{
		Frontend frontend;
		const CompileResult& result = frontend.compileSingleFile("Samples/sample01.c");

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

			int K = 2;
			StatementBase* stmt = 0;
			BlockStatement::Iterator it = ifStmt->getThenBody().getFirst();
			for(int i = 0; it.isValid() && i < K; ++it, ++i)
			{
				stmt = &(*it);
			}

			cout<<"Parameter of distributing: \n";
			writer.visit(*(dynamic_cast<ExpressionStatement*>(stmt)));
			cout<<endl;


			IfDistribution ifDistribution;

			// Arguments
			ArgumentValues argumentValues;
			ArgumentValue firstArg(ArgumentValue::StmtIf);
			firstArg.setReprise(ifStmt);
			argumentValues.push_back(firstArg);
			
			ArgumentValue secondArg(ArgumentValue::StmtAny);
			secondArg.setReprise(stmt);
			argumentValues.push_back(secondArg);
			
			ArgumentValue thirdArg(ArgumentValue::Bool);
			thirdArg.setBool(false);
			argumentValues.push_back(thirdArg);

			ifDistribution.makeTransform(&frontend.getProgramUnit(), argumentValues);
			
			cout<<"Result of work:\n";
			writer.visit(frontend.getProgramUnit().getUnit(0));
		}
		
		cout << OPS::Reprise::Coordinator::instance().getStatistics();

	}
	catch(OPS::Reprise::RepriseError e)
	{
		std::cout<<"OPS Error! " + e.getMessage() + "\n";
	}
	catch(OPS::Transforms::IfDistribution::IfDistributionException e)
	{
		cout<<"Some problems:\n\t"<<e.getMessage();
	}

	std::cout<<"\nTestmod for IfDistribution finised...\n";

	system("pause");
	
	return 0;
}
