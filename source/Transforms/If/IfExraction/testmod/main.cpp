#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Transforms/If/IfExtraction/IfExtraction.h"
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
	
	std::cout<<"Testmod for IfExtraction started...\n";

	try
	{
		Frontend frontend;
		const CompileResult& result = frontend.compileSingleFile("Samples/sample01.c");

		if (result.errorCount() == 0)
		{
			cout << "Compiled successful.\n";
			
			OPS::Backends::OutToC writer(std::cout);
//			writer.visit(frontend.getProgramUnit().getUnit(0));

			Declarations& decls = frontend.getProgramUnit().getUnit(0).getGlobals();
			SubroutineDeclaration& subroutine = *decls.getFirstSubr();//decls.getSubroutine(0);

			BlockStatement& bodyBlock = subroutine.getBodyBlock();
/*
			cout<<"\nBody block: \n";

			writer.visit(bodyBlock);
*/
			cout<<"\nTry to transform this statement: \n";

			ForStatement* forStmt = dynamic_cast<ForStatement*>(&(*(bodyBlock.getFirst())));

			writer.visit(*forStmt);

			int K = 1;
			StatementBase* stmt = 0;
			BlockStatement::Iterator it = forStmt->getBody().getFirst();
			for(int i = 0; it.isValid() && i < K; ++it, ++i)
			{
				stmt = &(*it);
			}

			cout<<"By this conditional statement: \n";
			writer.visit(*(dynamic_cast<IfStatement*>(stmt)));
			cout<<endl;

			IfExtraction ifExtraction;

			// Arguments
			ArgumentValues argumentValues;
			
			ArgumentValue firstArg(ArgumentValue::StmtIf);
			firstArg.setReprise(stmt);
			argumentValues.push_back(firstArg);

			ArgumentValue secondArg(ArgumentValue::StmtFor);
			secondArg.setReprise(forStmt);
			argumentValues.push_back(secondArg);

			try
			{
				ifExtraction.makeTransform(&frontend.getProgramUnit(), argumentValues);

				cout<<"Result of work:\n";
				writer.visit(frontend.getProgramUnit().getUnit(0));
			}
			catch(OPS::Transforms::IfExtraction::IfExtractionException e)
			{
				cout<<"IfExtraction exception: "<<e.getMessage()<<endl;
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

	std::cout<<"\nTestmod for IfDistribution finised...\n";

	system("pause");
	
	return 0;
}
