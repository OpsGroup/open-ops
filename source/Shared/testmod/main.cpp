#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Shared/LinearExpressions.h"
#include "Shared/LoopShared.h"
#include "Shared/RepriseClone.h"
#include "Shared/ExpressionHelpers.h"

#include "Backends/RepriseXml/RepriseXml.h"
#include "Backends/OutToC/OutToC.h"

//#include <conio.h>
#include <string>
#include <iostream>

//using namespace OPS;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Frontend;
using namespace OPS::Shared;
using namespace std;

void check_LinearExpressions()
{
	cout << "!!!Check Linear Expressions parsing!!!" << endl;
 	Frontend frontend;
    const CompileResult& result = frontend.compileSingleFile("tests//testLinearExpressions.c"); // testLinearExpressions.c

	if (result.errorCount() == 0)
	{
		ForStatement& ForStmt = frontend.getProgramUnit().getUnit(0).getGlobals().getFirstSubr()->getBodyBlock().getFirst()->cast_to<ForStatement>();
		ForStatement& ForStmt2 = ForStmt.getBody().getFirst()->cast_to<ForStatement>();
		ExpressionStatement& AssignmentStmt = ForStmt2.getBody().getFirst()->cast_to<ExpressionStatement>();

		BasicCallExpression& OccurenceExpression = AssignmentStmt.get().cast_to<BasicCallExpression>().getArgument(1).cast_to<BasicCallExpression>();
		BasicCallExpression& LessExpression = ForStmt2.getFinalExpression().cast_to<BasicCallExpression>();
		ExpressionBase& SumExpression = LessExpression.getArgument(1);
		
		OPS::Backends::OutToC writer(cout);
		writer.visit(AssignmentStmt);
		cout << endl << "-------------" << endl;
        cout << AssignmentStmt.dumpState() << endl;

		vector<VariableDeclaration*> LoopCounters = getIndexVariables(&OccurenceExpression);
		
//		if (lp.isLinear(LoopCounters, &OccurenceExpression.getArgument(1).cast_to<BasicCallExpression>()))
//		{
		
			LinearExpressionMatrix* LEM = new LinearExpressionMatrix(&OccurenceExpression);
            cout << (*LEM)[0].convert2RepriseExpression()->dumpState() << endl;
			
			cout << endl << "------- Linear Expression Coefficients --- A[4*i+3*j+2][j-3] -------" << endl;
			int ii = LEM->getArrayDimensionsCount();
			OPS_UNUSED(ii);
			for (int i=0; i<LEM->getArrayDimensionsCount(); ++i)
			{
				for (int j=0; j<LEM->getLoopNestDepth(); ++j)
				{
					if ((*LEM)[i].isEvaluatable())
						cout << (*LEM)[i].getCoefficientAsInteger(LoopCounters[j]) << " ";
					else
  						cout << (*LEM)[i][j]->dumpState() << " ";
				}

				if ((*LEM)[i].isEvaluatable())
					cout << (*LEM)[i].getFreeCoefficientAsInteger() << " ";
				else
					cout << (*LEM)[i][LEM->getLoopNestDepth()]->dumpState() << " ";
				cout << endl;
			}

		//}
		//else
		//	cout << endl << "------- Non-linear expression -------" << endl;

		cout << endl << "------- Linear Expression Coefficients --- i+2*j+10 -------" << endl;
        //ExpressionBase* pp = & SumExpression.cast_to<BasicCallExpression>().getArgument(1);
        LinearExpressionMatrix* LEM2 = new LinearExpressionMatrix(&SumExpression);
		int count = LEM2->getArrayDimensionsCount();
		OPS_UNUSED(count);
		for (int i=0; i<((LEM2->getArrayDimensionsCount()==0)?1:LEM2->getArrayDimensionsCount()); ++i)
		{
			for (int j=0; j<LEM2->getLoopNestDepth(); ++j)
			{
				if ((*LEM2)[i].isEvaluatable())
					cout << (*LEM2)[i].getCoefficientAsInteger(LoopCounters[j]) << " ";
				else
  					cout << (*LEM2)[i][j]->dumpState() << " ";
			}

			if ((*LEM2)[i].isEvaluatable())
				cout << (*LEM2)[i].getFreeCoefficientAsInteger() << " ";
			else
				cout << (*LEM2)[i][LEM2->getLoopNestDepth()]->dumpState() << " ";
			cout << endl;
		}

        cout << endl << "------ Simplify -------" << endl << endl;

        using namespace OPS::Shared::ExpressionHelpers;
        IntegerHelper ih(BasicType::BT_INT32);
		ReprisePtr<ExpressionBase> simple = ParametricLinearExpression::simplify(&(op(OccurenceExpression.getArgument(1)) - ih(4)*op(SumExpression)));
        cout << simple->dumpState() << endl;
        simple = ParametricLinearExpression::simplify(&(op(SumExpression)-op(SumExpression)));
        cout << simple->dumpState() << endl;
        cout << endl << "-------------" << endl << endl;

		delete LEM2;
        delete LEM;

	}
	else
        cout << "There are " << result.errorCount() << " error(s)" << endl;

}

int main()
{
	// Проверим методы работающие с линейными индексными выражениями
	check_LinearExpressions();

	//getch();
	return 0;
}
