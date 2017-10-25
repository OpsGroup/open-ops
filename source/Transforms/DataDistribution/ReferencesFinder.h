#ifndef _REFERENCES_FINDER_H
#define _REFERENCES_FINDER_H

#include <list>
#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
namespace Transforms
{

using OPS::Reprise::ExpressionBase;
using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::ReferenceExpression;


class ArrayAccessExpressionsFinder : public OPS::Reprise::Service::DeepWalker
{
	std::string m_refName;
	int m_arrayDimension;
	bool leftOfAssign;
public:
	ArrayAccessExpressionsFinder(std::string refName, int arrayDimension) : m_refName(refName), m_arrayDimension(arrayDimension), leftOfAssign(false) {}
	std::list<BasicCallExpression*> refList;
	std::list<BasicCallExpression*> generatorsList;
	std::list<BasicCallExpression*> usingsList;

	void visit(BasicCallExpression& basic)
	{
		if(basic.getKind() == BasicCallExpression::BCK_ASSIGN)
		{
			bool previousLeftOfAssign = leftOfAssign;
			leftOfAssign = true;
			basic.getArgument(0).accept(*this);
			leftOfAssign = previousLeftOfAssign;
			basic.getArgument(1).accept(*this);
			return;
		}
		
		if (basic.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
		{
			if (basic.getArgumentCount() == 1 + m_arrayDimension &&
				basic.getArgument(0).is_a<ReferenceExpression>() &&
				basic.getArgument(0).cast_to<ReferenceExpression>().getReference().getName() == m_refName)
			{
				refList.push_back(&basic);
				if(leftOfAssign)
					generatorsList.push_back(&basic);
				else
					usingsList.push_back(&basic);
			}
		}	
		OPS::Reprise::Service::DeepWalker::visit(basic);
	}
};

}
}
#endif 
