#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Analysis/AliasAnalysis/AliasAnalyzer.h"

#include <conio.h>
#include <string>
#include <iostream>

int main()
{
	using namespace OPS::Reprise;
	using namespace OPS::Reprise::Canto;
	using namespace OPS::Frontend;
	using namespace OPS::AliasAnalysis;
	using namespace std;

	OPS::Frontend::Frontend frontend;	
	const CompileResult& result = frontend.compileSingleFile("tests\\test.c");
	AliasAnalyzer aliasAnalyzer(&frontend.getProgramUnit());

	/*
	BlockStatement::Iterator iterFirst = frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(0).getBodyBlock().getFirst();
	BlockStatement::Iterator iterLast = frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(1).getBodyBlock().getLast();


	//firstPointer
	ExpressionStatement& esFirst = iterFirst->cast_to<ExpressionStatement>();
	BasicCallExpression& basicCallExprFirst = esFirst.get().cast_to<BasicCallExpression>();
	ReferenceExpression& firstPointer = basicCallExprFirst.getArgument(0).cast_to<ReferenceExpression>();
	std::string sFirst = firstPointer.dumpState();

	//secondPointer
	iterFirst++;
	ExpressionStatement& esSecond = iterFirst->cast_to<ExpressionStatement>();
	BasicCallExpression& basicCallExprSecond = esSecond.get().cast_to<BasicCallExpression>();
	ReferenceExpression& secondPointer = basicCallExprSecond.getArgument(0).cast_to<ReferenceExpression>();
	std::string sSecond = secondPointer.dumpState();

	//exprCall
	std::string sExpr = iterLast->dumpState();	
	StatementBase& sb = iterLast->cast_to<StatementBase>();	
	ExpressionStatement& es = sb.cast_to<ExpressionStatement>();
	SubroutineCallExpression& sce =  es.get().cast_to<SubroutineCallExpression>(); //static_cast<SubroutineCallExpression*>()

	int iii = aliasAnalyzer.IsAlias(&firstPointer, &secondPointer, &sce);

	cout << iii << endl;
	*/

	getch();
}
