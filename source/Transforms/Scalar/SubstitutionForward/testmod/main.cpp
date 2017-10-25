#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Reprise/Service/DeepWalker.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Backends/OutToC/OutToC.h"

using namespace OPS::Reprise;
using namespace OPS::Frontend;
using namespace OPS::Reprise::Service;

using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Transforms::Scalar;

class GetFirstReferenceWalker: public DeepWalker
{
public:
	GetFirstReferenceWalker()
	: m_refExpr(NULL)
	{
	}

	void visit(TranslationUnit& unit)
	{
		DeepWalker::visit(unit);
	}

	void visit(ReferenceExpression& refExpr)
	{
		if (m_refExpr == NULL)
		{
			OPS::Backends::OutToC writer(std::cout);
			m_refExpr = &refExpr;
			writer.visit(*m_refExpr);
			std::cout << std::endl << std::endl;
		}
	}

	ReferenceExpression& getReferenceExpression() const
	{
		OPS_ASSERT(m_refExpr != NULL);
		return *m_refExpr;
	}

private:
	ReferenceExpression* m_refExpr;
};

class GetFirstForWalker: public DeepWalker
{
public:
	GetFirstForWalker()
	: m_forStmt(NULL)
	{
	}

	void visit(TranslationUnit& unit)
	{
		DeepWalker::visit(unit);
	}

	void visit(ForStatement& forStatement)
	{
		if (m_forStmt == NULL)
		{
			OPS::Backends::OutToC writer(std::cout);
			m_forStmt = &forStatement;
			writer.visit(*m_forStmt);
			std::cout << std::endl << std::endl;
		}
	}

	ForStatement& getForStatement() const
	{
		OPS_ASSERT(m_forStmt != NULL);
		return *m_forStmt;
	}

private:
	ForStatement* m_forStmt;
};

int main()
{
	Frontend frontend;
	const CompileResult& result = frontend.compileSingleFile("tests/test1.c");
	if (result.errorCount() == 0)
	{
		OPS::Backends::OutToC writer(std::cout);
// 		writer.visit(frontend.getProgramUnit().getUnit(0));

		GetFirstReferenceWalker getFirstReferenceWalker;
		getFirstReferenceWalker.visit(frontend.getProgramUnit().getUnit(0));
		ReferenceExpression& refExpr = getFirstReferenceWalker.getReferenceExpression();
		
		GetFirstForWalker getFirstForWalker;
		getFirstForWalker.visit(frontend.getProgramUnit().getUnit(0));
		BlockStatement& blockStmt = getFirstForWalker.getForStatement().getBody();

        IntegerHelper c(refExpr.getResultType()->cast_to<BasicType>());
// 		std::cout << c(1).is_a<StrictLiteralExpression>() << std::endl << std::endl;
		makeSubstitutionForward(blockStmt, *refExpr.clone() + *refExpr.clone(), ReprisePtr<ExpressionBase>(&(c(2) * *refExpr.clone())));
		writer.visit(blockStmt);
		std::cout << std::endl << std::endl;
	}
	else
	{
		std::cout << result.errorText() << std::endl;
	}
}
