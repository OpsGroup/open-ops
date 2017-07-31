#ifndef OPS_FRONTEND_CANTOTOREPRISE_H_INCLUDED__
#define OPS_FRONTEND_CANTOTOREPRISE_H_INCLUDED__

//	OPS includes

//	Local includes

namespace OPS
{
namespace Reprise
{
	class RepriseBase;
	class VariableDeclaration;
	class StatementBase;
	class ExpressionStatement;
    class ExpressionBase;
}

namespace Frontend
{
namespace C2R
{


void convertTypes(Reprise::RepriseBase& repriseNode);
void convertExpressions(Reprise::RepriseBase& repriseNode, bool common, bool general, bool others);
void convertLiterals(Reprise::RepriseBase& repriseNode);

void convertVariablesInit(Reprise::RepriseBase& repriseNode);
void convertBreakContinue(Reprise::RepriseBase& repriseNode);

Reprise::StatementBase* createVariableInitStatement(Reprise::VariableDeclaration& variable,
                                                          Reprise::ExpressionBase& initExpression);
Reprise::StatementBase* convertVariableInit(Reprise::VariableDeclaration& variable,
												  Reprise::StatementBase* firstStatement);

// Must be deprecated soon... 
//void convertStrictToBasic(RepriseBase& repriseNode);

/*!		
	\brief Changes multi-dimensional FORTRAN arrays to Reprise arrays.
	
	FORTRAN arrays must have the following characteristic:		
		1) lower bounds equals to 0;	
		2) upper bounds are declared  
	Must be deprecated in future.		
*/
void convertHirFArrays(Reprise::RepriseBase& repriseNode);

}
}
}

#endif
