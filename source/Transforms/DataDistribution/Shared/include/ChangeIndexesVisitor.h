#ifndef CHANGE_INDEXES_VISITOR_H
#define CHANGE_INDEXES_VISITOR_H

#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stack>
#include <cassert>

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include "Transforms/DataDistribution/Shared/BDParameters.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::ForStatement;
using OPS::Reprise::Service::DeepWalker;

class ChangeIndexesVisitor : public DeepWalker
{		
public:
	using DeepWalker::visit;	
	
	ChangeIndexesVisitor(std::list<BDParameters> pragmas);
	void visit(BasicCallExpression& bckCall);
private:
	std::list<BDParameters> m_pragmas;

	void canonizeIndex(OPS::Reprise::ExpressionBase & expr, OPS::Reprise::VariableDeclaration* d);

	//OPS::Reprise::VariableDeclaration* get
};

}
}
}

#endif
