#include "ForStmtTree.h"
#include "Shared/LinearExpressions.h"
#include "Reprise/Reprise.h"
#include "shared_helpers.h"
#include <cassert>

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace std;
using namespace OPS::Shared;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Transforms;
using namespace OPS::TransformationsHub;

//
//	LOCAL FUNCTIONS
//

// Возвращает объявление счетчика конкретного цикла
static VariableDeclaration* getCounterFromSpecificFor(ForStmtTree& tree, int index)
{			
	std::list<ForStmtTree::Node>::iterator it;

	int ind = -1;
	for (it = tree.nodes.begin(); it != tree.nodes.end(); ++it)
	{
		ind++;
		if (index == ind)
		{
			// TODO: Сделать проверки перед выполнением преобразования, что
			// циклы канонизированы.
			ForStatement* stmt = it->forStmt;
			VariableDeclaration* result = &stmt->getInitExpression().cast_to<BasicCallExpression>().getArgument(0).cast_to<ReferenceExpression>().getReference();
			return result;
		}
	}
	return 0;
}

static ForStmtTree::Node* getForStmtNode(ForStmtTree& tree, int index)
{
	std::list<ForStmtTree::Node>::iterator it;

	int ind = -1;
	for (it = tree.nodes.begin(); it != tree.nodes.end(); ++it)
	{
		ind++;
		if (index == ind)
			return &*it;
	}
	return 0;
}

OPS::Reprise::VariableDeclaration* ArrayOccurence::arrayDecl()
{
	ReferenceExpression* refExpr = expr->getArgument(0).cast_ptr<ReferenceExpression>();
	return &refExpr->getReference();
}

int getGCD(int a, int b)
{
	assert(a >= 0 && b >= 0);

	if (a == 0) return b;
	if (b == 0) return a;
	if (a == b) return a;
	if (a == 1 || b == 1) return 1;
	if (a % 2 == 0 && b % 2 == 0) return 2*getGCD(a/2, b/2);
	if (a % 2 == 0) return getGCD(a/2, b);
	if (b % 2 == 0) return getGCD(a, b/2);
	if (a > b) return getGCD((a-b)/2, b);

	assert(a % 2 != 0 && b % 2 != 0 && a < b);
	return  getGCD((b-a)/2, a);
}

ForStmtTree::Node::Node()
: m_commonBlockSizeVariable(0)
{
}

VariableDeclaration* ForStmtTree::Node::getCommonBlockSizeVariable(std::list<BDParameters>& pragmas)
{	
	//	NOTE размер блока = НОД(d1, d2, ..). Если все di равны, то возвращается размер одного
	//	из них. Иначе создается новая константная переменная.

	if (m_commonBlockSizeVariable != 0)
		return m_commonBlockSizeVariable;

	int blockSizesCount = occuranceTable.values.size();
	std::vector<int> blockSizes(blockSizesCount);

	int index = -1;

	// someVar - переменная, содержащее в себе размерность блока di.
	VariableDeclaration* someVar = 0;

	for (std::list<ArrayOccurence>::iterator it = occuranceTable.values.begin();
		it != occuranceTable.values.end(); ++it)
	{
		index++;
		for (std::list<BDParameters>::iterator it2 = pragmas.begin(); it2 != pragmas.end(); ++it2)
		{
			if (it2->newArrayDecl == it->arrayDecl())
			{
				someVar = it2->d[it->dim];
				blockSizes[index] = getValueOfConstIntegerVariable(someVar);
			}
		}
	}	

	assert(blockSizesCount >= 1);
	if (blockSizesCount == 1)
	{
		m_commonBlockSizeVariable = someVar;
		return m_commonBlockSizeVariable;
	}


	int gcd = 0;
	bool areEqual = true;
	for (int i = 0; i < blockSizesCount; i++)
	{
		int new_gcd = getGCD(gcd, blockSizes[i]);		
		areEqual = areEqual && (new_gcd == gcd || i == 0);
		gcd = new_gcd;
	}
	if (areEqual == true)
	{
		m_commonBlockSizeVariable = someVar;
		return m_commonBlockSizeVariable;
	}

	
	BlockStatement* parentBlock = &forStmt->getParentBlock();	
	BasicType* type = BasicType::int32Type();
	type->setConst(true);
	m_commonBlockSizeVariable = &OPS::Reprise::Editing::createNewVariable(*type, *parentBlock, "c");
	m_commonBlockSizeVariable->setInitExpression(*StrictLiteralExpression::createInt32(gcd));
	return m_commonBlockSizeVariable;
}

//
//		ForStmtTreeCreator implementation
//
ForStmtTreeCreator::ForStmtTreeCreator(list<BDParameters> pragmas)
{	
	ForStmtTree::Node root;
	root.parent = -1;
	root.forStmt = 0;

	tree.nodes.push_back(root);
	tree.pragmas = pragmas;
	m_forStmtStack.push_back(tree.nodes.size() - 1);
}

void ForStmtTreeCreator::visit(ForStatement& forStmt)
{

	ForStmtTree::Node nextNode;
	nextNode.forStmt = &forStmt;	
	nextNode.parent = m_forStmtStack.back();

	tree.nodes.push_back(nextNode);

	m_forStmtStack.push_back(tree.nodes.size() - 1);	
	DeepWalker::visit(forStmt);	

	m_forStmtStack.pop_back();
}

void ForStmtTreeCreator::visit(BasicCallExpression& bckCall)
{
	if (bckCall.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS && bckCall.getArgument(0).is_a<ReferenceExpression>())
	{
		ReferenceExpression* refExpr = bckCall.getArgument(0).cast_ptr<ReferenceExpression>();		
		if (isDistibutedArray(&refExpr->getReference(), tree.pragmas) == true)
		{
			//  делаем линеаризацию каждого индексного выражения
			for (int i = 1; i < bckCall.getArgumentCount(); i++)
			{
				ExpressionBase* dimExpression = &bckCall.getArgument(i);

				for (int j = m_forStmtStack.size() - 1; j >= 1; j--)
				{
					int nodeIndex = m_forStmtStack[j];

					VariableDeclaration* counterDecl = getCounterFromSpecificFor(tree, nodeIndex);
					if (counterDecl == 0)
						continue;
					
					ParametricLinearExpression::VariablesDeclarationsVector vectorDecl;
					vectorDecl.push_back(counterDecl);

					ParametricLinearExpression* LinExpr = ParametricLinearExpression::createByListOfVariables(dimExpression, vectorDecl);
					ParametricLinearExpression::Coefficient freeCoef = LinExpr->getFreeCoefficient();
					ParametricLinearExpression::Coefficient counterCoef = LinExpr->getCoefficient(counterDecl);
										
					if (counterCoef.get() != 0) //	TODO: добавить проверку, что counterCoef = 1
					{
						ForStmtTree::Node* node = getForStmtNode(tree, nodeIndex);

						ArrayOccurence occurance;
						occurance.exprPtr = &bckCall;
						occurance.expr.reset(bckCall.clone());
						occurance.counterDecl = counterDecl;
						occurance.freeCoef.reset(freeCoef->clone());
						occurance.dim = i-1;
						node->occuranceTable.values.push_back(occurance);
						break;
					}
				}
			}
		}
	}

	//		TODO добавить проверку, что все вхождения для каждого цикла имеют одинаковый параметр d
	//		Он определяет верхнюю границу одного из циклов при гнездовании.

	DeepWalker::visit(bckCall);
}

}
}
}
