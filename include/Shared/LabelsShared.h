#ifndef _LABELS_SHARED_H_INCLUDED_
#define _LABELS_SHARED_H_INCLUDED_

#include "Reprise/Statements.h"

namespace OPS
{
namespace Shared
{

using OPS::Reprise::ReprisePtr;

using OPS::Reprise::StatementBase;


// You can use this function after replaceStatement function
// if you need to translate label from sourceStmt to destinationStmt.
// EXAMPLE: updateLabel(replaceStatement(stmt1, ReprisePtr<StatementBase>(stmt2)), &stmt2);
ReprisePtr<StatementBase> updateLabel(ReprisePtr<StatementBase> sourceStmt, StatementBase& destinationStmt);


// This function checks whether you can apply a generateNewLabels function
// to a statement statementBase.
bool isPossibleToGenerateNewLabels(StatementBase& statementBase);

// If you clone some statement from the program and want to use it in this program,
// then you have to generate new labels in cloned statement by this function.
// WARNING: If some goto outside statementBase points label inside statementBase,
// then the function will not work!!!
void generateNewLabels(StatementBase& statementBase);

}	// Shared
}	// OPS

#endif	// _LABELS_SHARED_H_INCLUDED_
