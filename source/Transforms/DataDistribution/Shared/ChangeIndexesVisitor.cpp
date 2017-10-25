#include "ChangeIndexesVisitor.h"
#include "Shared/LinearExpressions.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/DataDistribution/Shared/BDParameters.h"
#include "FrontTransforms/ExpressionSimplifier.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace std;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Transforms;
using namespace OPS::Shared::ExpressionHelpers;

class BlockIndexCananizer
{		
public:
	BDParameters bdParams;
	int index;	

	//		Result:	e1, e2
	//		index = e1*d + e2
	ExpressionBase* e1;
	ExpressionBase* e2;

	BlockIndexCananizer() : e1(0), e2(0) {}
	void canonizeExpression(ExpressionBase* expr, int rootSign);

	//	Просматривает множетели, если находит среди них d, то возвращает true,
	//	а в blockExpr - остальные множители. 
	//	Иначе возвращает false
	bool canonizeExpression2(ExpressionBase* expr, int rootSign, ExpressionBase*& blockExpr);

private:
	void addBlockCoef(ExpressionBase* expr);
	void addFreeCoef(ExpressionBase* expr, int rootSign);
};

void BlockIndexCananizer::canonizeExpression(ExpressionBase* expr, int rootSign)
{
	if (expr->cast_ptr<BasicCallExpression>() != 0)
	{
		BasicCallExpression* bckExpr = expr->cast_ptr<BasicCallExpression>();
		if (bckExpr->getKind() == BasicCallExpression::BCK_UNARY_PLUS)
		{
			canonizeExpression(&bckExpr->getArgument(0), rootSign);
			return;
		}
		if (bckExpr->getKind() == BasicCallExpression::BCK_UNARY_MINUS)
		{
			canonizeExpression(&bckExpr->getArgument(0), rootSign+1);
			return;
		}
		if (bckExpr->getKind() == BasicCallExpression::BCK_BINARY_PLUS)
		{
			canonizeExpression(&bckExpr->getArgument(0), rootSign);
			canonizeExpression(&bckExpr->getArgument(1), rootSign);
			return;
		}
		if (bckExpr->getKind() == BasicCallExpression::BCK_BINARY_MINUS)
		{
			canonizeExpression(&bckExpr->getArgument(0), rootSign);
			canonizeExpression(&bckExpr->getArgument(1), rootSign+1);
			return;
		}
	}

	ExpressionBase* blockExpr = 0;
	if (canonizeExpression2(expr, rootSign, blockExpr) == true)
	{
		addBlockCoef(blockExpr);
	}
	else
	{
		addFreeCoef(expr, rootSign);
	}
}

bool BlockIndexCananizer::canonizeExpression2(ExpressionBase* expr, int rootSign, ExpressionBase*& blockExpr)
{	
	if (expr->cast_ptr<StrictLiteralExpression>() != 0 || expr->cast_ptr<ReferenceExpression>() != 0)
	{
		ExpressionBase* simplExpr = OPS::ExpressionSimplifier::Simplifier().simplify(expr);
		if (simplExpr->cast_ptr<StrictLiteralExpression>() != 0)
		{
			StrictLiteralExpression* se = simplExpr->cast_ptr<StrictLiteralExpression>();
			if (se->isInteger() == true)
			{
				int v1 = simplExpr->cast_ptr<StrictLiteralExpression>()->getInt32();
				int v2 = bdParams.d_value(index);
				if (v1 % v2 == 0)
				{
					int blockPart = v1 / v2;
					if (rootSign % 2 == 1)
						blockPart = -blockPart;
					blockExpr = StrictLiteralExpression::createInt32(blockPart);
					return true;
				}
			}			
		}
	}

	if (expr->cast_ptr<BasicCallExpression>() != 0 && expr->cast_ptr<BasicCallExpression>()->getKind() == BasicCallExpression::BCK_MULTIPLY)
	{
		BasicCallExpression* bckExpr = expr->cast_ptr<BasicCallExpression>();
		if (canonizeExpression2(&bckExpr->getArgument(0), rootSign, blockExpr) == true)
		{
			if (blockExpr->cast_ptr<StrictLiteralExpression>() != 0
				&& blockExpr->cast_ptr<StrictLiteralExpression>()->isInteger() == true
				&& blockExpr->cast_ptr<StrictLiteralExpression>()->getInt32() == 1)
				blockExpr = &(op(&bckExpr->getArgument(1)));
			else
				blockExpr = &(op(blockExpr->clone()) * op(&bckExpr->getArgument(1)));
			return true;
		}
		if (canonizeExpression2(&bckExpr->getArgument(1), rootSign, blockExpr) == true)
		{
			if (blockExpr->cast_ptr<StrictLiteralExpression>() != 0
				&& blockExpr->cast_ptr<StrictLiteralExpression>()->isInteger() == true
				&& blockExpr->cast_ptr<StrictLiteralExpression>()->getInt32() == 1)
				blockExpr = &(op(&bckExpr->getArgument(0)));
			else
				blockExpr = &(op(blockExpr->clone()) * op(&bckExpr->getArgument(0)));
			return true;
		}
	}

	return false;
}

void BlockIndexCananizer::addBlockCoef(ExpressionBase* expr)
{
	ExpressionBase* buf = expr->clone();
	if (e1 == 0)
		e1 = &(op(buf));
	else
		e1 = &(op(e1) + op(buf));
}
void BlockIndexCananizer::addFreeCoef(ExpressionBase* expr, int rootSign)
{
	ExpressionBase* buf = expr->clone();
	if (e2 == 0)
	{
		if (rootSign % 2 == 0)
			e2 = &(op(buf));
		else
			e2 = &( - op(buf));
	}
	else
	{
		if (rootSign % 2 == 0)
			e2 = &(op(e2) + op(buf));
		else
			e2 = &(op(e2) - op(buf));
	}
}


ChangeIndexesVisitor::ChangeIndexesVisitor(std::list<BDParameters> pragmas)
{
	m_pragmas = pragmas;
}

void ChangeIndexesVisitor::visit(BasicCallExpression& bckCall)
{
	bool paramsFound = false;
	BDParameters usedParams;
	if (bckCall.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS && bckCall.getArgument(0).is_a<ReferenceExpression>())
	{		
		ReferenceExpression* refExpr = bckCall.getArgument(0).cast_ptr<ReferenceExpression>();
		for (std::list<BDParameters>::iterator it = m_pragmas.begin(); it != m_pragmas.end(); ++it)
		{
			if (it->newArrayDecl == &refExpr->getReference())
			{
				paramsFound = true;
				usedParams = *it;
				break;
			}
		}
	}

	IntegerHelper c(OPS::Reprise::BasicType::BT_INT32);
	if (paramsFound == true)
	{
		vector<ExpressionBase*> blockIndexes(usedParams.size());
		vector<ExpressionBase*> freeIndexes(usedParams.size());

		for (int i = 1; i < bckCall.getArgumentCount(); i++)
		{
			BlockIndexCananizer* canonizer = new BlockIndexCananizer();
			canonizer->bdParams = usedParams;
			canonizer->index = i - 1;
			canonizer->canonizeExpression(&bckCall.getArgument(i), 0);

			blockIndexes[i-1] = canonizer->e1;
			freeIndexes[i-1] = canonizer->e2;			

			delete canonizer;
		}

		//	A[d1*i1+j1][d2*i2+j2]...[dk*ik+jk] =>
		//	AA[i1*d1*d2*d3*dc2*dc3 + i2*d1*d2*d3*dc3 + i3*d1*d2*d3 + j1*d2*d3+j2*d3+j3]
		int buf1 = 1;
		int buf2 = 1;
		for (int i = 0; i < usedParams.size(); i++)
		{
			buf1 *= usedParams.d_value(i);
			buf2 *= usedParams.blocks_count(i);
		}

		ExpressionBase* newBlockIndex = 0;
		for (int i = 0; i < usedParams.size(); i++)
		{
			buf2 /= usedParams.blocks_count(i);
			if (newBlockIndex == 0)
				newBlockIndex = &(op(blockIndexes[i]) * c(buf1*buf2));
			else
				newBlockIndex = &(op(newBlockIndex) + op(blockIndexes[i]) * c(buf1*buf2));
		}
		for (int i = 0; i < usedParams.size(); i++)
		{
			buf1 /= usedParams.d_value(i);
			newBlockIndex = &(op(newBlockIndex) + op(freeIndexes[i]) * c(buf1));
		}

		ExpressionBase* arrayRef = bckCall.getArgument(0).clone();
		bckCall.removeArguments();
		bckCall.addArgument(arrayRef);
		bckCall.addArgument(newBlockIndex);
	}

	DeepWalker::visit(bckCall);
}

}
}
}
