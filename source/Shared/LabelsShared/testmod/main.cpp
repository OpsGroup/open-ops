#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Reprise/Service/DeepWalker.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/LabelsShared.h"
#include "Backends/OutToC/OutToC.h"

using namespace OPS::Reprise;
using namespace OPS::Frontend;
using namespace OPS::Reprise::Service;
using namespace OPS::Reprise::Editing;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Shared;

class TestWalker: public DeepWalker
{
public:
	TestWalker()
	{
	}
	void visit(TranslationUnit& unit)
	{
		DeepWalker::visit(unit);
	}
	void visit(ReferenceExpression& refExpr)
	{
        IntegerHelper c(refExpr.getResultType()->cast_to<BasicType>());
		m_newExprStmt = new ExpressionStatement(&(*refExpr.clone() R_AS c(13)));
	}
	void visit(ExpressionStatement& exprStmt)
	{
		m_oldExprStmt = &exprStmt;
		DeepWalker::visit(exprStmt);
	}

	ExpressionStatement* newExprStmt()
	{
		return m_newExprStmt;
	}

	ExpressionStatement* oldExprStmt()
	{
		return m_oldExprStmt;
	}

private:
	ExpressionStatement* m_newExprStmt;
	ExpressionStatement* m_oldExprStmt;
};

int main()
{
	Frontend frontend;
	const CompileResult& result = frontend.compileSingleFile("tests/test1.c");
	if (result.errorCount() == 0)
	{
		OPS::Backends::OutToC writer1(std::cout);
		writer1.visit(frontend.getProgramUnit().getUnit(0));
		std::cout << "\n\n";

		TestWalker testWalker;
		testWalker.visit(frontend.getProgramUnit().getUnit(0));

		ExpressionStatement& oldExpr(*testWalker.oldExprStmt());
		ExpressionStatement* newExpr(testWalker.newExprStmt());
		updateLabel(replaceStatement(oldExpr, ReprisePtr<StatementBase>(newExpr)), *newExpr);

		OPS::Backends::OutToC writer2(std::cout);
		writer2.visit(frontend.getProgramUnit().getUnit(0));
		std::cout << "\n\n";

		BlockStatement& rootBlock = newExpr->getRootBlock();
		generateNewLabels(rootBlock);

		OPS::Backends::OutToC writer3(std::cout);
		writer3.visit(frontend.getProgramUnit().getUnit(0));
		std::cout << "\n\n";
	}
	else
	{
		std::cout << result.errorText() << std::endl;
	}
}
