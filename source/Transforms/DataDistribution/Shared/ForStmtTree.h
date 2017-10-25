#ifndef FOR_STMT_TREE_H
#define FOR_STMT_TREE_H

//		STL headers
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <deque>
#include <cassert>

//		OPS headers
#include "Transforms/DataDistribution/Shared/DataDistributionForSharedMemory.h"
#include "Transforms/DataDistribution/Shared/BDParameters.h"

#include "Reprise/Service/DeepWalker.h"

#include "Reprise/Canto/HirFTypes.h"
#include "Shared/SubroutinesShared.h"
#include "Shared/DataShared.h"
#include "Shared/LinearExpressions.h"
#include "../ReferencesFinder.h"


namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::ForStatement;
using OPS::Reprise::Service::DeepWalker;

class OccurencesFinder : public DeepWalker
{
public:
	using DeepWalker::visit;
	typedef std::list<BasicCallExpression*> NodeList;
	NodeList m_nodeList;

	std::list<BDParameters> m_pragmas;	
	void visit(BasicCallExpression& bckCall)
	{
		if (bckCall.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS && bckCall.getArgument(0).is_a<ReferenceExpression>())
		{
			m_nodeList.push_back(&bckCall);
		}

		DeepWalker::visit(bckCall);	
	};
};


// вхождение размещаемого массива
struct ArrayOccurence
{	
public:	

	BasicCallExpression* exprPtr;

	//	рассматривается выражение в размерности с номером dim
	//	обращение к массиву	A[..] .. [i + freeCoef]..[..].	
	OPS::Reprise::ReprisePtr<BasicCallExpression> expr;
	OPS::Reprise::VariableDeclaration* arrayDecl();

	//	counterDecl = i
	VariableDeclaration* counterDecl;
	OPS::Reprise::ReprisePtr<ExpressionBase> freeCoef;
	
	//		freeCoef = aCoeft*d + bCoef
	//		0 <= bCoef < d
	OPS::Reprise::ReprisePtr<ExpressionBase> aCoef;
	OPS::Reprise::ReprisePtr<ExpressionBase> bCoef;
	

	//	number of dimension
	int dim;
	
};

struct ArrayOccurenceTable
{
	std::list<ArrayOccurence> values;
};

struct ForStmtTree
{
public:
	struct Node
	{
	public:
		Node();
		//	Может быть равным null в случае, когда это корень дерева
		ForStatement* forStmt;

		//	Массив непосредственных потомков
		std::list<int> childs;

		//	Непосредственный родитель
		int parent;

		//	массив обращений для данного листа
		ArrayOccurenceTable occuranceTable;

		//	
		VariableDeclaration* sortedFreeCoefsArray;
		ExpressionBase* getSortedFreeCoefByIndex(int index);

		OPS::Reprise::VariableDeclaration* getCommonBlockSizeVariable(std::list<BDParameters>& pragmas);
	private:
		VariableDeclaration* m_commonBlockSizeVariable;
	};	

	std::list<Node> nodes;
	std::list<BDParameters> pragmas;
};

class ForStmtTreeCreator : public DeepWalker
{		
public:
	using DeepWalker::visit;	
	
	//		out: результирующее дерево
	ForStmtTree tree;	

	ForStmtTreeCreator(std::list<BDParameters> pragmas);
	void visit(ForStatement& forStmt);
	void visit(BasicCallExpression& bckCall);

private:
	std::deque<int> m_forStmtStack;
};

}
}
}

#endif
