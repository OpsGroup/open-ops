#ifndef OPS_IR_REPRISE_SERVICEFUNCTIONS_H_INCLUDED__
#define OPS_IR_REPRISE_SERVICEFUNCTIONS_H_INCLUDED__

#include "Reprise/Common.h"
#include "Reprise/Expressions.h"
#include "Reprise/Statements.h"
#include "Reprise/Units.h"

#include <list>

namespace OPS
{
namespace Reprise
{
class ProgramFragment;

namespace Editing
{

//	Declarations manipulation functions
/**
	\brief Creates new variable.

	If you want to create unique variable in given block with given type this function is what you need.

	\arg \c type - type of the creation variable. \sa ExpressionBase::getResultType()
	\arg \c block - block to create variable in
	\arg \c partName [optional] - adds to unique generated variable name if you want it

	\return Declaration of variable object reference.
*/
VariableDeclaration& createNewVariable(const TypeBase& type, BlockStatement& block, const std::string& partName = "");

/**
	\brief Creates new global variable.

	If you want to create unique global variable with given type this function is what you need.

	\arg \c type - type of the creation variable. \sa ExpressionBase::getResultType()
	\arg \c unit - translation unit to create variable in
	\arg \c partName [optional] - adds to unique generated variable name if you want it

	\return Declaration of variable object reference.
*/
VariableDeclaration& createNewGlobalVariable(const TypeBase& type, TranslationUnit& unit, const std::string& partName = "");


//	Expressions manipulation functions
ReprisePtr<ExpressionBase> replaceExpression(ExpressionBase& sourceExpr, ReprisePtr<ExpressionBase> destinationExpr);

void substituteExpression(ReprisePtr<ExpressionBase>& baseExpr, const std::string& note, 
	ReprisePtr<ExpressionBase> destinationExpr);

bool isExpressionsEqual(const ExpressionBase& expr1, const ExpressionBase& expr2);

bool hasSideEffects(ExpressionBase& expr);

ReprisePtr<TypeBase> getExpressionType(const ExpressionBase& expression);

ReprisePtr<TypeBase> getExpressionPreciseType(const ExpressionBase& expression);

TypeBase& desugarType(TypeBase& type);
const TypeBase& desugarType(const TypeBase& type);

ReprisePtr<TypeBase> replaceType(TypeBase& sourceType, ReprisePtr<TypeBase> destinationType);

//	Statements manipulation functions
std::list<GotoStatement*> findAllGotos(BlockStatement& rootBlock);

ReprisePtr<StatementBase> replaceStatement(StatementBase& sourceStmt, ReprisePtr<StatementBase> destinationStmt); 
ReprisePtr<StatementBase> replaceProgramFragment(ProgramFragment& sourceFragment, ReprisePtr<StatementBase> destinationStmt);



// Functions to manipulate with FOR loops
bool forIsBasic(const ForStatement& forStmt);

bool forHeaderIsCanonized(ForStatement& forStmt);

ReferenceExpression& getBasicForCounter(ForStatement& forStmt);

ExpressionBase& getBasicForInitExpression(ForStatement& forStmt);

ExpressionBase& getBasicForFinalExpression(ForStatement& forStmt);

ExpressionBase& getBasicForStep(ForStatement& forStmt);

bool checkParentChildRelations(RepriseBase& node, bool shouldThrow = true);


void moveLabels(StatementBase& fromStmt, StatementBase& toStmt);
}
}
}

#endif	// OPS_IR_REPRISE_SERVICEFUNCTIONS_H_INCLUDED__
