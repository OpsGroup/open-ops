#ifndef PREDICATES_H
#define PREDICATES_H

#include "DepGraphEx.h"
#include "LoopHelpers.h"
//#include <string>

namespace DepGraph
{

	// Предикат-константа. Может быть true или false
	class ConstPred : public IPredicate
					, public OPS::NonCopyableMix
	{
	public:
		const bool	m_bValue;

		AutoExprNode GetExpr() const;
		std::string GetText() const;
		IPredicate* Clone() const;
		static ConstPred* Create(bool bValue);
	protected:
		explicit ConstPred(bool bValue);
	};

	class BinaryPred : public IPredicate
	{
	public:
		BinaryPred(IPredicate* pLeft, IPredicate* pRight)
			:m_pLeft(pLeft)
			,m_pRight(pRight) {}

		inline const IPredicate* GetLeft() const
		{
			return m_pLeft;
		}
		inline const IPredicate* GetRight() const
		{
			return m_pRight;
		}

		void SetLeft(IPredicate* pPred);
		void SetRight(IPredicate* pPred);
		~BinaryPred();
	protected:
		IPredicate* m_pLeft;
		IPredicate* m_pRight;
	};

	// Предикат логическое И
	class AndPred : public BinaryPred
	{
	public:
		AutoExprNode GetExpr() const;
		std::string GetText() const;
		IPredicate* Clone() const;
		AndPred(IPredicate* pLeft, IPredicate* pRight);
	protected:
	};

	// Предикат логическое ИЛИ
	class OrPred : public BinaryPred
	{
	public:
		AutoExprNode GetExpr() const;
		std::string GetText() const;
		IPredicate* Clone() const;
		OrPred(IPredicate* pLeft, IPredicate* pRight);
	protected:

	};

	// Предикат 
	class NotPred : public IPredicate
	{
	public:
		AutoExprNode GetExpr() const;
		std::string GetText() const;
		IPredicate* Clone() const;
		inline const IPredicate* GetArg() const
		{
			return m_pArg;
		}
		void SetArg(IPredicate* pPred);
	protected:
		NotPred(IPredicate* pArg);
		IPredicate* m_pArg;
	};

	// Предикат-неравенство: "expr1 <= expr2"
	class InequalPred : public IPredicate
	{
	public:
		inline const LinearExpr& GetLeft() const
		{
			return m_cLeft;
		}
		inline const LinearExpr& GetRight() const
		{
			return m_cRight;
		}

		IPredicate* Clone() const;
		std::string GetText() const;
		AutoExprNode GetExpr() const;
		static IPredicate* Create(const LinearExpr& rLeft, const LinearExpr& rRight);

	protected:
		InequalPred(const LinearExpr& rLeft, const LinearExpr& rRight);
        LinearExpr	m_cLeft;
		LinearExpr	m_cRight;
	};

	// Предикат двойное неравенство: "expr1 <= exp2 <= expr3"
	class RangePred : public IPredicate
	{
	public:
		RangePred(const LinearExpr& rLeft,
					const LinearExpr& rMiddle,
					const LinearExpr& rRigth);
		inline const LinearExpr& GetLeft() const
		{
			return m_cLeft;
		}
		inline const LinearExpr& GetMiddle() const
		{
			return m_cMiddle;
		}
		inline const LinearExpr& GetRight() const
		{
			return m_cRight;
		}
		IPredicate* Clone() const;
		std::string GetText() const;
		AutoExprNode GetExpr() const;

		bool isIncompatibleWith(IPredicate& other);

	protected:
		LinearExpr	m_cLeft;
		LinearExpr	m_cMiddle;
		LinearExpr	m_cRight;
	};

	class MODPred : public IPredicate
	{
	public:
		IPredicate* Clone() const;
		std::string GetText() const;
		AutoExprNode GetExpr() const;
		static IPredicate* Create(const LinearExpr& rLeft, int GCD);
	protected:
		MODPred(const LinearExpr& rLeft, int GCD);
		LinearExpr	m_cLeft;
		int			m_nDivisor;
	};

	/// Проверяет на несовместность два предиката
	bool isIncompatible(IPredicate& left, IPredicate& right);
}
#endif
