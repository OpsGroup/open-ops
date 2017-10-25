#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Reprise/Service/DeepWalker.h"
#include "Backends/OutToC/OutToC.h"
#include "Shared/ExpressionHelpers.h"

using namespace OPS::Reprise;
using namespace OPS::Frontend;
using namespace OPS::Reprise::Service;

using namespace OPS::Shared::ExpressionHelpers;

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
		OPS::Backends::OutToC writer(std::cout);

		writer.visit( *refExpr.clone() R_AS *refExpr.clone() + +-c(10) / R_AD()*refExpr.clone() - (*refExpr.clone()R_BK(c(6))) );

		std::cout << std::endl;
	}
};

int main()
{
	Frontend frontend;
	const CompileResult& result = frontend.compileSingleFile("tests/test1.c");
	if (result.errorCount() == 0)
	{
// 		OutToC writer(std::cout);
// 		writer.visit(frontend.getProgramUnit().getUnit(0));

		TestWalker testWalker;
		testWalker.visit(frontend.getProgramUnit().getUnit(0));
	}
	else
	{
		std::cout << result.errorText() << std::endl;
	}
}
