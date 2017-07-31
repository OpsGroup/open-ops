#include "Reprise/Reprise.h"
#include "Reprise/Canto.h"
#include "Frontend/Frontend.h"
#include "FrontTransforms/CantoToReprise.h"
#include "Backends/OutToC/OutToC.h"
#include "Reprise/Lifetime.h"

#include "Tests.h"
#include "c2r.h"

#include <iostream>

#include <stdio.h>



void test_HirCdecodeLiteral()
{
	using namespace std;
	using namespace OPS::Reprise;
	using namespace OPS::Reprise::Canto;

	ReprisePtr<StrictLiteralExpression> result1 = HirCdecodeLiteral("0x123456789");
	cout << result1->dumpState();

}

int* f()
{
	static int i = 1;
	i++;
	return &i;
}

class Listen1 : public OPS::BaseListener, public OPS::Listener<OPS::Reprise::PtrType>
{
public:
	Listen1()
	{
		add(*this);
	}

	virtual ~Listen1()
	{
		remove(*this);
	}

	inline virtual void preChange(OPS::Reprise::PtrType& ptr, OPS::BaseListener::ChangeType ct)
	{
		std::cout << ptr.dumpState() << " modify" << std::endl;
	}

};

int main()
{
	using namespace OPS::Reprise;
	using namespace OPS::Frontend;
	using namespace std;

	testContinue();
//	testCollections();

	{
		Listen1 l;
		PtrType* ptr = new PtrType(BasicType::float32Type());
		ptr->setRestrict(true);
		;
	}


//	test_HirCdecodeLiteral();

//	testMI2SB();
//	testCollections();


	{
		Frontend frontend;
		//frontend.options().convertGeneralExpressions = false;
		//frontend.options().convertOtherExpressions = false;
//		frontend.options().convertBreakContinue = false;

//		const CompileResult& result = frontend.compileSingleFile("tests\\fullhouse.c");
//		const CompileResult& result = frontend.compileSingleFile("tests\\lamport2.c");
		const CompileResult& result = frontend.compileSingleFile("tests\\oper--.c");

//		ExpressionStatement& exprStmt = frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(1).getBodyBlock().getFirst()->cast_to<OPS::Reprise::ExpressionStatement>();
		if (result.errorCount() == 0)
		{
//			frontend.getProgramUnit().getUnit(0).getGlobals().getVariable(0).declarators().set(VariableDeclarators::DECL_VOLATILE);
			cout << "Compiled successful.\n";
			OPS::Backends::OutToC writer(std::cout);
			writer.visit(frontend.getProgramUnit().getUnit(0));
			cout << "=========================== dumpState =========================" << endl;
			cout << frontend.getProgramUnit().dumpState();
			cout << "=========================== convertTest ======================="  << endl;

//			NotesMap<> notesMap;
//			ReprisePtr<RepriseBase> ptr(&frontend.getProgramUnit());
//			notesMap.set("Hello", Note::newReprise(ptr));
//			C2R::convertExpressions(frontend.getProgramUnit(), true, true, true);
			Declarations::SubrIterator subrDeclIter = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr();
			Declarations::ConstVarIterator varDeclIter = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstVar();
			if (varDeclIter.isValid())
				cout << varDeclIter->getName();
			BlockStatement& blockStmt = subrDeclIter->getBodyBlock();
			BlockStatement::Iterator iterBeforeLast = blockStmt.getLast();
			iterBeforeLast--;
			//ExpressionStatement& esFirst = iterBeforeLast->cast_to<ExpressionStatement>();
//			C2R::convertExpressions(frontend.getProgramUnit(), true, true, true);
			writer.visit(frontend.getProgramUnit().getUnit(0));
/*			ExpressionStatement& exprStmt = frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(1).getBodyBlock().getFirst()->cast_to<ExpressionStatement>();
			exprStmt.get().cast_to<Canto::HirCCallExpression>().getArgument(1).setNote("Blah", "sdd");
			ReprisePtr<ExpressionBase> clonedExpr(exprStmt.get().clone());
			ReprisePtr<ExpressionBase> newExpr(new ReferenceExpression(frontend.getProgramUnit().getUnit(0).getGlobals().getVariable(0)));
			Editing::substituteExpression(clonedExpr, "Blah", newExpr);
			Editing::replaceExpression(exprStmt.get(), clonedExpr);
			writer.visit(frontend.getProgramUnit().getUnit(0));
	*/
//			writer.visit(*clonedExpr);

		/*	ForStatement& firstFor = frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(0).getBodyBlock().getFirst()->cast_to<ForStatement>();
			BlockStatement& blockStmt = frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(0).getBodyBlock();
			ReprisePtr<StatementBase> returnStmt(new ReturnStatement());
			Editing::replaceStatement(firstFor, returnStmt);
			writer.visit(frontend.getProgramUnit().getUnit(0));
			ReprisePtr<StatementBase> newBlockStmt(new BlockStatement());
			Editing::replaceStatement(blockStmt, newBlockStmt);
			writer.visit(frontend.getProgramUnit().getUnit(0));
			*/
//			fullUnrollFor(firstFor);
		}
		cout << Coordinator::instance().getStatistics();

		//ReprisePtr<ExpressionBase> replaced = 
			//Editing::replaceExpression(firstFor.getFinalExpression(), ReprisePtr<ExpressionBase>(BasicLiteralExpression::createBoolean(true)));
	}
	cout << OPS::Reprise::Coordinator::instance().getStatistics();

	return 0;
}


