#include "Analysis/DepGraph/DepGraphEx/Predicates.h"
#include "Shared/LinearExpressionsUtils.h"
#include "OPS_Core/msc_leakcheck.h"

namespace DepGraph
{
	using namespace OPS::Reprise;
	using namespace OPS::Shared;

	ConstPred::ConstPred(bool bValue)
		:m_bValue(bValue)
	{
	}

	IPredicate* ConstPred::Clone() const
	{
		return new ConstPred(m_bValue);
	}

	std::string ConstPred::GetText() const
	{
		return m_bValue ? "true" : "false";
	}

	AutoExprNode ConstPred::GetExpr() const
	{
		return AutoExprNode(BasicLiteralExpression::createBoolean(m_bValue));
	}

	ConstPred* ConstPred::Create(bool bValue)
	{
		return new ConstPred(bValue);
	}

	void BinaryPred::SetLeft(IPredicate* pPred)
	{
		delete m_pLeft;
		m_pLeft = pPred;
	}

	void BinaryPred::SetRight(IPredicate* pPred)
	{
		delete m_pRight;
		m_pRight = pPred;
	}
	void NotPred::SetArg(IPredicate* pPred)
	{
		delete m_pArg;
		m_pArg = pPred;
	}

	BinaryPred::~BinaryPred()
	{
		delete m_pLeft;
		delete m_pRight;
	}

	AndPred::AndPred(IPredicate* pLeft, IPredicate* pRight)
		:BinaryPred(pLeft,pRight)
	{
	}

	OrPred::OrPred(IPredicate* pLeft, IPredicate* pRight)
		:BinaryPred(pLeft,pRight)
	{
	}

	NotPred::NotPred(IPredicate* pArg)
		:m_pArg(pArg) {}


	IPredicate* AndPred::Clone() const
	{
		return new AndPred(m_pLeft->Clone(), m_pRight->Clone());
	}

	IPredicate* OrPred::Clone() const
	{
		return new OrPred(m_pLeft->Clone(), m_pRight->Clone());
	}

	IPredicate* NotPred::Clone() const
	{
		return new NotPred(m_pArg->Clone());
	}

	std::string AndPred::GetText() const
	{
		return "(" + m_pLeft->GetText() + ")&&(" + m_pRight->GetText() + ")";
	}

	std::string OrPred::GetText() const
	{
		return "(" + m_pLeft->GetText() + ")||(" + m_pRight->GetText() + ")";
	}

	std::string NotPred::GetText() const
	{
		return "!(" + m_pArg->GetText() + ")";
	}

	AutoExprNode AndPred::GetExpr() const
	{
		/*return AutoExprNode(ExprBinary::create(ExprOper::OT_LAND, 
								m_pLeft->GetExpr().release(), m_pRight->GetExpr().release()));*/
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_AND);
		temp->addArgument(m_pLeft->GetExpr().release());
		temp->addArgument(m_pRight->GetExpr().release());
		return AutoExprNode(temp);
	}

	AutoExprNode OrPred::GetExpr() const
	{
		/*return AutoExprNode(ExprBinary::create(ExprOper::OT_LOR, 
			m_pLeft->GetExpr().release(), m_pRight->GetExpr().release()));*/
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_OR);
		temp->addArgument(m_pLeft->GetExpr().release());
		temp->addArgument(m_pRight->GetExpr().release());
		return AutoExprNode(temp);
	}

	AutoExprNode NotPred::GetExpr() const
	{
		/*return AutoExprNode(ExprUnary::create(ExprOper::OT_LNOT, m_pArg->GetExpr().release()));*/
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_NOT);
		temp->addArgument(m_pArg->GetExpr().release());
		return AutoExprNode(temp);
	}

	InequalPred::InequalPred(const LinearExpr& rLeft, const LinearExpr& rRight)
		:m_cLeft(rLeft)
		,m_cRight(rRight) 
	{
		int sub = 0;
		
		if (m_cRight.isConstant()) {
			sub = m_cLeft.getFreeSummand();
		}
		else if (m_cLeft.isConstant()) {
			sub = m_cRight.getFreeSummand();
		}
		
		m_cLeft -= sub;
		m_cRight -= sub;
	}

	IPredicate* InequalPred::Create(const LinearExpr& rLeft, const LinearExpr& rRight)
	{
		if (rLeft.isConstant() && rRight.isConstant())
		{
			return ConstPred::Create( rLeft.getFreeSummand() <= rRight.getFreeSummand() );
		}

		bool makeSwap = (rLeft.isConstant() && rRight.getMap().begin()->second < 0) ||
						(rRight.isConstant() && rLeft.getMap().begin()->second < 0);

		if (makeSwap)
			return new InequalPred(rRight*-1, rLeft*-1);
		else
			return new InequalPred(rLeft, rRight);
	}

	IPredicate* InequalPred::Clone() const
	{
		return new InequalPred(m_cLeft, m_cRight);
	}

	std::string InequalPred::GetText() const
	{
		if (m_cLeft.isConstant())
			return canonicalLinearExpressionToString(m_cRight) + " >= " + canonicalLinearExpressionToString(m_cLeft);
		else
			return canonicalLinearExpressionToString(m_cLeft) + " <= " + canonicalLinearExpressionToString(m_cRight);
	}

	AutoExprNode InequalPred::GetExpr() const
	{
		throw OPS::RuntimeError("InequalPred::GetExpr() is not implemented");
	}

	RangePred::RangePred(const LinearExpr& rLeft, const LinearExpr& rMiddle, const LinearExpr& rRigth)
		:m_cLeft(rLeft)
		,m_cMiddle(rMiddle)
		,m_cRight(rRigth)	{}

	IPredicate* RangePred::Clone() const
	{
		return new RangePred(m_cLeft, m_cMiddle, m_cRight);
	}

	std::string RangePred::GetText() const
	{
		if (m_cRight == m_cLeft) {
			return canonicalLinearExpressionToString(m_cLeft) + " = " + canonicalLinearExpressionToString(m_cMiddle);
		}
		return canonicalLinearExpressionToString(m_cLeft) + " <= " + canonicalLinearExpressionToString(m_cMiddle)
				+ " <= " + canonicalLinearExpressionToString(m_cRight);
	}

	AutoExprNode RangePred::GetExpr() const
	{
		AutoExprNode pLeft = canonicalLinearExpressionToReprise(m_cLeft);
		AutoExprNode pMiddle = canonicalLinearExpressionToReprise(m_cMiddle);
		AutoExprNode pRight = canonicalLinearExpressionToReprise(m_cRight);
		
		BasicCallExpression* temp1 = new BasicCallExpression(BasicCallExpression::BCK_LESS_EQUAL);
		temp1->addArgument(pLeft.release());
		temp1->addArgument(pMiddle->clone());
		BasicCallExpression* temp2 = new BasicCallExpression(BasicCallExpression::BCK_LESS_EQUAL);
		temp2->addArgument(pMiddle.release());
		temp2->addArgument(pRight.release());
		AutoExprNode pPart1 (temp1);
		AutoExprNode pPart2 (temp2);
		temp1 = new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_AND);
		temp1->addArgument(pPart1.release());
		temp1->addArgument(pPart2.release());
		return AutoExprNode(temp1);
	}

	IPredicate* MODPred::Create(const LinearExpr& rLeft, int GCD)
	{
		// На 1 делится все!
		if (GCD == 1)
			return ConstPred::Create(true);

		if (rLeft.isConstant()) {
			return ConstPred::Create(rLeft.getFreeSummand() % GCD ? true : false);
		}
		else {
			return new MODPred(rLeft, GCD);
		}
	}

	MODPred::MODPred(const LinearExpr& rLeft, int GCD)
		:m_cLeft(rLeft)
		,m_nDivisor(GCD)	{}

	IPredicate*	MODPred::Clone() const
	{
		return new MODPred(m_cLeft, m_nDivisor);
	}

	std::string MODPred::GetText() const
	{
		return "(" + canonicalLinearExpressionToString(m_cLeft) + ")%" + OPS::Strings::format("%d", m_nDivisor) + "== 0";
	}

	AutoExprNode MODPred::GetExpr() const
	{
		throw OPS::RuntimeError("MODPred::GetExpr() is not implemented");
	}

	IPredicate* CreateAndPredicate(const IPredicate& leftPred, const IPredicate& rightPred)
	{
		const ConstPred* pLeftConst = dynamic_cast<const ConstPred*>(&leftPred);

		// Если левая часть - константа
		if (pLeftConst)
		{
			if (pLeftConst->m_bValue)
				return rightPred.Clone();
			else
				return pLeftConst->Clone();
		}

		const ConstPred* pRightConst = dynamic_cast<const ConstPred*>(&rightPred);
		// Если правая часть - константа
		if (pRightConst)
		{
			if (pRightConst->m_bValue)
				return leftPred.Clone();
			else
				return pRightConst->Clone();
		}

		const InequalPred* pRightIneqPred = dynamic_cast<const InequalPred*>(&rightPred);
		const InequalPred* pLeftIneqPred = dynamic_cast<const InequalPred*>(&leftPred);

		if (pLeftIneqPred && pRightIneqPred) {
			if (pLeftIneqPred->GetRight() == pRightIneqPred->GetLeft()) {
				return new RangePred(pLeftIneqPred->GetLeft(), 
									 pLeftIneqPred->GetRight(),
									 pRightIneqPred->GetRight());
			}
			if (pLeftIneqPred->GetLeft() == pRightIneqPred->GetRight()) {
				return new RangePred(pRightIneqPred->GetLeft(), 
									 pRightIneqPred->GetRight(),
									 pLeftIneqPred->GetRight());
			}
		}

		return new AndPred(leftPred.Clone(), rightPred.Clone());
	}

	IPredicate* CreateOrPredicate(const IPredicate& leftPred, const IPredicate& rightPred)
	{
		const ConstPred* pLeftConst = dynamic_cast<const ConstPred*>(&leftPred);

		// Если левая часть - константа
		if (pLeftConst)
		{
			if (pLeftConst->m_bValue)
				return pLeftConst->Clone();
			else
				return rightPred.Clone();
		}

		const ConstPred* pRightConst = dynamic_cast<const ConstPred*>(&rightPred);
		// Если правая часть - константа
		if (pRightConst)
		{
			if (pRightConst->m_bValue)
				return pRightConst->Clone();
			else
				return leftPred.Clone();
		}

		const RangePred* pRightRangePred = dynamic_cast<const RangePred*>(&rightPred);
		const RangePred* pLeftRangePred = dynamic_cast<const RangePred*>(&leftPred);

		if (pRightRangePred && pLeftRangePred &&
			pRightRangePred->GetMiddle() == pLeftRangePred->GetMiddle()) 
		{
			LinearExpr cb = pRightRangePred->GetLeft() - pLeftRangePred->GetRight();
			if (cb.isConstant() && cb.getFreeSummand() == 1)
				return new RangePred(pLeftRangePred->GetLeft(), pLeftRangePred->GetMiddle(), pRightRangePred->GetRight());

			LinearExpr ad = pLeftRangePred->GetLeft() - pRightRangePred->GetRight();
			if (ad.isConstant() && ad.getFreeSummand() == 1)
				return new RangePred(pRightRangePred->GetLeft(), pRightRangePred->GetMiddle(), pLeftRangePred->GetRight());
		}

		return new OrPred(leftPred.Clone(), rightPred.Clone());
	}

	bool RangePred::isIncompatibleWith(IPredicate& other)
	{
		RangePred* range = dynamic_cast<RangePred*>(&other);
		if (range)
		{
						
		}
		return false;
	}

	bool isIncompatible(IPredicate& left, IPredicate& right)
	{
		if (RangePred* leftRange = dynamic_cast<RangePred*>(&left))
		{
			if (RangePred* rightRange = dynamic_cast<RangePred*>(&right))
			{
				if (leftRange->GetMiddle() == rightRange->GetMiddle())
				{

				}
			}
		}

		return false;
	}

}
