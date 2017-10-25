#include <iostream>
#include <fstream>

#include <QtWidgets/QApplication>
#include <QStringList>

#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Backends/OutToC/OutToC.h"
#include "OPS_Core/Exceptions.h"

#include "Transforms/Subroutines/FragmentToSubroutine.h"
#include "Shared/SubroutinesShared.h"
#include "Shared/NodesCollector.h"


using namespace OPS::Shared;
using namespace OPS::Transforms;
using namespace OPS::Reprise;
using namespace std;

enum errorType {
	errorNone = 0,
	errorParsing = 1,
	errorStatementNotFound = 2,
	errorStatementsNotInOneBlock = 3,
	errorParsingArguments = 4,
	errorInternal = 5
};


using namespace OPS;
using namespace Reprise;


class StatementsCollector: public Shared::NodesCollector<StatementBase> 
{
	void visit(IfStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
	void visit(WhileStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
	void visit(ForStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
	void visit(PlainSwitchStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
    void visit(Canto::HirBreakStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
	void visit(GotoStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
    void visit(Canto::HirContinueStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
	void visit(ReturnStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
	void visit(ExpressionStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
	void visit(BlockStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
	void visit(EmptyStatement& node)
	{
		getCollection().push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}
};

void printUsage()
{
	std::cout << "usage: FragmentToSubroutine sourcefile.c destinationfile.c startRow finishRow" << std::endl;
}

StatementBase* getStatementByRow(StatementsCollector::CollectionType& statements, int row)
{
	StatementBase* result = NULL;

	StatementsCollector::CollectionType::iterator pStatement = statements.begin();
	for(; pStatement != statements.end(); ++pStatement)
	{
		Reprise::SourceCodeManager& codeManager = RepriseContext::defaultContext().getSourceCodeManager();
		Reprise::SourceCodeLocation location = codeManager.getLocation(*(*pStatement));
		if (row == location.Row.first)
		{
			result = *pStatement;
			break;
		}
	}
	return result;
}

int main(int argc, char* argv[])
{
	std::string sourceFileName = "";
	std::string destinationFileName = "";
	int startRow = 0;
	int finishRow = 0;
	// reading command-line arguments
	 try {
	    using namespace Qt;
        QApplication the_app(argc, argv);

        //trying to get the arguments into a list    
        QStringList cmdline_args = QCoreApplication::arguments();

		if(cmdline_args.length() != 5)
		{
			printUsage();
			return errorParsingArguments;
		}

		cmdline_args.pop_front();

		sourceFileName = cmdline_args.first().toStdString(); cmdline_args.pop_front();
		destinationFileName = cmdline_args.first().toStdString(); cmdline_args.pop_front();

		bool transformed = false;
		startRow = cmdline_args.first().toInt(&transformed); cmdline_args.pop_front();
		if(!transformed)
		{
			printUsage();
			return errorParsingArguments;
		}

		transformed = false;
		finishRow = cmdline_args.first().toInt(&transformed);
		if(!transformed)
		{
			printUsage();
			return errorParsingArguments;
		}
    }
	catch (...) 
	{ 
		printUsage();
		return errorParsingArguments;
	}

	try
	{
		Frontend::Frontend frontend;
		const CompileResult& result = frontend.compileSingleFile(sourceFileName);
		
		if (result.errorCount() == 0)
		{

			Declarations& decls = frontend.getProgramUnit().getUnit(0).getGlobals();
			SubroutineDeclaration* subroutine = &(*(decls.getFirstSubr()));		
			BlockStatement::Iterator firstStmt = subroutine->getBodyBlock().getFirst();
			BlockStatement::Iterator lastStmt = firstStmt;
	
			// найдем операторы, находящиеся на указанных строках		
			StatementsCollector statementsCollector;
			frontend.getProgramUnit().getUnit(0).getGlobals().accept(statementsCollector);
			StatementBase* pFirstStatement = getStatementByRow(statementsCollector.getCollection(), startRow);
			StatementBase* pLastStatement = getStatementByRow(statementsCollector.getCollection(), finishRow);
			
			if(pFirstStatement == NULL || pLastStatement == NULL)
			{
				return errorStatementNotFound;
			}

			BlockStatement* firstParentBlock = &(pFirstStatement->getParentBlock());
			BlockStatement* secondParentBlock = &(pLastStatement->getParentBlock());

			if(firstParentBlock != secondParentBlock)
			{
				return errorStatementsNotInOneBlock;
			}
	
			firstStmt = firstParentBlock->convertToIterator(pFirstStatement);
			lastStmt = firstParentBlock->convertToIterator(pLastStatement);

			bool result = Transforms::Subroutines::fragmentToSubroutine(firstStmt, lastStmt); 
			OPS_UNUSED(result);

			ofstream fOut(destinationFileName.c_str());
			OPS::Backends::OutToC writer(fOut);
			frontend.getProgramUnit().getUnit(0).accept(writer);
		} else {
			return errorParsing;
		}
	}
	catch(RepriseError e)
	{
		cout<<"OPS Error! " + e.getMessage() + "\n";
	}
	catch(exception e)
	{
		cout<<"Some problems:\n\t"<<e.what();
	}
	return errorNone;
}
