#include "Shared/LinearExpressionsUtils.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Shared
{

CanonicalLinearExpression& operator +=(CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient freeSummand)
{
	expr.add(freeSummand);
	return expr;
}

CanonicalLinearExpression& operator-=(CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient freeSummand)
{
	expr.add(-freeSummand);
	return expr;
}

CanonicalLinearExpression& operator+=(CanonicalLinearExpression& expr, const CanonicalLinearExpression& other)
{
	expr.add(other);
	return expr;
}

CanonicalLinearExpression& operator -=(CanonicalLinearExpression& expr, const CanonicalLinearExpression& other)
{
	expr.add(other.getOpposite());
	return expr;
}

CanonicalLinearExpression operator +(const CanonicalLinearExpression& expr, const CanonicalLinearExpression& other)
{
	CanonicalLinearExpression	res (expr);
	res += other;
	return res;
}

CanonicalLinearExpression operator-(const CanonicalLinearExpression& expr, const CanonicalLinearExpression& other)
{
	CanonicalLinearExpression res(expr);
	res -= other;
	return res;
}

CanonicalLinearExpression operator *(const CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient mult)
{
	CanonicalLinearExpression res(expr);
	res.multiply(mult);
	return res;
}

CanonicalLinearExpression operator +(const CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient freeSummand)
{
	CanonicalLinearExpression	res (expr);
	res += freeSummand;
	return res;
}

CanonicalLinearExpression operator -(const CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient freeSummand)
{
	CanonicalLinearExpression	res (expr);
	res -= freeSummand;
	return res;
}

bool operator==(const CanonicalLinearExpression& left, const CanonicalLinearExpression& right)
{
	return left.getMap() == right.getMap() && left.getFreeSummand() == right.getFreeSummand();
}

bool operator!=(const CanonicalLinearExpression& left, const CanonicalLinearExpression& right)
{
	return !(left == right);
}

typedef ReprisePtr<ExpressionBase> ExprPtr;

ExprPtr summandToReprise(const CanonicalLinearExpression::SummandsMap::const_iterator& it)
{
	ExprPtr	pResult;

	// Р”РµР»Р°РµРј СѓРїСЂРѕС‰РµРЅРёСЏ...
	if( it->second == 1 )	// 1*n - > n
	{
		return ExprPtr( new ReferenceExpression(*(it->first)) );
	}
	if( it->second == -1 ) // (-1)*n - > -n
	{
		/*return AutoExprNode( ExprUnary::create(
								ExprOper::OT_UNMINUS,
								ExprData::create(it->first.getNameRef())) );*/
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_UNARY_MINUS);
		temp->addArgument(new ReferenceExpression(*(it->first)));
		return ExprPtr(temp);
	}

	/*return AutoExprNode( ExprBinary::create( ExprOper::OT_MUL,
											 ExprImm::create(ImmValue::IV_INT, it->second),
											 ExprData::create(it->first.getNameRef())	)
						);*/
	BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
	temp->addArgument(BasicLiteralExpression::createInteger(it->second));
	temp->addArgument(new ReferenceExpression(*(it->first)));
	return ExprPtr(temp);
}

ExprPtr summandsMapToReprise(const CanonicalLinearExpression::SummandsMap &expr)
{
	if (expr.empty())
		return ExprPtr();

	ExprPtr	pNode( summandToReprise(expr.begin()) );

	CanonicalLinearExpression::SummandsMap::const_iterator it = ++expr.begin(),
							itEnd = expr.end();

	for(; it != itEnd; ++it)
	{
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
		temp->addArgument(pNode.release());
		temp->addArgument(summandToReprise(it).release());
		pNode.reset(temp);
	}

	return pNode;
}

ExprPtr canonicalLinearExpressionToReprise(const CanonicalLinearExpression &expr)
{
	ExprPtr pLinear = summandsMapToReprise(expr.getMap());

	if (pLinear.get() && expr.getFreeSummand() == 0)
		return pLinear;
	else if (!pLinear.get())
	{
		//return AutoExprNode( ExprImm::create(ImmValue::IV_INT, m_nFreeCoef) );
		return ExprPtr(BasicLiteralExpression::createInteger(expr.getFreeSummand()));
	}
	else
	{
		//return AutoExprNode( ExprBinary::create(ExprOper::OT_PLUS, pLinear.release(), ExprImm::create(ImmValue::IV_INT, m_nFreeCoef)) );
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
		temp->addArgument(pLinear.release());
		temp->addArgument(BasicLiteralExpression::createInteger(expr.getFreeSummand()));
		return ExprPtr(temp);
	}
}

std::string summandToString(const CanonicalLinearExpression::SummandsMap::const_iterator& itElem, bool bSign)
{
	std::string strText;
	if (itElem->second == 1)
	{
		if (bSign)
			return OPS::Strings::format("+%s", itElem->first->getName().c_str());
		else
			return OPS::Strings::format("%s", itElem->first->getName().c_str());
	}
	else if (itElem->second == -1)
	{
		return OPS::Strings::format("-%s", itElem->first->getName().c_str());
	}
	else
	{
		if (itElem->second < 0 || !bSign)
			return OPS::Strings::format("%d*%s", itElem->second, itElem->first->getName().c_str());
		else
			return OPS::Strings::format("+%d*%s", itElem->second, itElem->first->getName().c_str());
	}
}

std::string canonicalLinearExpressionToString(const CanonicalLinearExpression &expr)
{
	CanonicalLinearExpression::SummandsMap::const_iterator it = expr.getMap().begin(),
							itEnd = expr.getMap().end();

	std::string strText;
	if (it != itEnd)
		strText = summandToString(it++, false);

	for(; it != itEnd; ++it)
	{
		strText += summandToString(it, true);
	}

	if (strText.empty() || expr.getFreeSummand() < 0)
	{
		strText += OPS::Strings::format("%d", expr.getFreeSummand());
	}
	else
	{
		if (expr.getFreeSummand() > 0)
			strText += OPS::Strings::format("+%d", expr.getFreeSummand());
	}
	return strText;
}


}
}
