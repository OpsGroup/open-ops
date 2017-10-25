#ifndef SHARED_HELPERS_H
#define SHARED_HELPERS_H

#include <string>
#include <memory>
#include <list>
#include <deque>

#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Reprise.h"
#include "Reprise/Utils.h"

#include "ArrayDistributionInfo.h"
#include "DistributedArrayReferenceInfo.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::ReferenceExpression;
using OPS::Reprise::VariableDeclaration;
using OPS::Reprise::VariableDeclaration;
using OPS::Reprise::ExpressionBase;
using OPS::Reprise::ForStatement;


bool isBlockArrayReference(ReferenceExpression& ref, std::list<std::shared_ptr<ArrayDistributionInfo> >& infoList,
                           std::shared_ptr<ArrayDistributionInfo>& info);
int getValueOfConstIntegerVariable(OPS::Reprise::VariableDeclaration* decl);

// loop helpers
OPS::Reprise::VariableDeclaration* getLoopCounter(OPS::Reprise::ForStatement* stmt);
bool check_loop(const OPS::Reprise::ForStatement& forStmt);

std::vector<ForStatement*> findAllLoops(OPS::Reprise::SubroutineDeclaration* func);
OPS::Reprise::VariableDeclaration* findVariableByName(OPS::Reprise::BlockStatement& innerBlock, std::string name);

bool expression_values_are_equal(ExpressionBase& e1, ExpressionBase& e2);


//      e1 = k*d
bool expression_is_devided_on(ExpressionBase& e1, ReferenceExpression& d);

//      e == 1
bool expression_value_is(ExpressionBase& e, int value);

//  если e1 = a*d, то возвращается a
ExpressionBase* get_quotient_expression(ExpressionBase& e1, ReferenceExpression& d);



}
}
}

#endif
