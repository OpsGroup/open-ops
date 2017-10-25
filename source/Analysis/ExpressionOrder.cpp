#include "Analysis/ExpressionOrder.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Statements.h"
#include "Shared/StatementsShared.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Analysis
{

class ExpressionOrderWalker : public Service::WalkerBase
{
public:
	ExpressionOrderWalker(RepriseBase& firstParent, RepriseBase& secondParent)
		:m_firstParent(&firstParent)
		,m_secondParent(&secondParent)
		,m_result(0)
	{
	}

	int getResult() const { return m_result; }

	void visit(CompoundLiteralExpression&)
	{
		m_result = 0;
	}

	void visit(BasicCallExpression& basic)
	{
		switch(basic.getKind())
		{
		case BasicCallExpression::BCK_UNARY_PLUS:
		case BasicCallExpression::BCK_UNARY_MINUS:
		case BasicCallExpression::BCK_SIZE_OF:
		case BasicCallExpression::BCK_TAKE_ADDRESS:
		case BasicCallExpression::BCK_DE_REFERENCE:
		case BasicCallExpression::BCK_LOGICAL_NOT:
			// Унарные операции вообще не должны сюда приводить
			throw OPS::StateError("Unexpected BasicCallExpression kind in ExpressionOrderWalker::visit");

		//	Binary
		case BasicCallExpression::BCK_BINARY_PLUS:
		case BasicCallExpression::BCK_BINARY_MINUS:
		case BasicCallExpression::BCK_MULTIPLY:
		case BasicCallExpression::BCK_DIVISION:
		case BasicCallExpression::BCK_INTEGER_DIVISION:
		case BasicCallExpression::BCK_INTEGER_MOD:
		//	Assign
		case BasicCallExpression::BCK_ASSIGN:
		//	Equality
		case BasicCallExpression::BCK_LESS:
		case BasicCallExpression::BCK_GREATER:
		case BasicCallExpression::BCK_LESS_EQUAL:
		case BasicCallExpression::BCK_GREATER_EQUAL:
		case BasicCallExpression::BCK_EQUAL:
		case BasicCallExpression::BCK_NOT_EQUAL:
		//	Shifts
		case BasicCallExpression::BCK_LEFT_SHIFT:
		case BasicCallExpression::BCK_RIGHT_SHIFT:
		//	Bitwise
		case BasicCallExpression::BCK_BITWISE_NOT:
		case BasicCallExpression::BCK_BITWISE_AND:
		case BasicCallExpression::BCK_BITWISE_OR:
		case BasicCallExpression::BCK_BITWISE_XOR:
		case BasicCallExpression::BCK_ARRAY_ACCESS:
			// порядок не определен
			m_result = 0; break;
		//	Logical
		case BasicCallExpression::BCK_LOGICAL_AND:
		case BasicCallExpression::BCK_LOGICAL_OR:
		case BasicCallExpression::BCK_COMMA:
			// левая часть всегда выполняется перед правой
			if (m_firstParent == &basic.getArgument(0))
			{
				OPS_ASSERT(m_secondParent == &basic.getArgument(1));
				m_result = 1;
			}
			else
			{
				OPS_ASSERT(m_secondParent == &basic.getArgument(0));
				OPS_ASSERT(m_firstParent == &basic.getArgument(1));
				m_result = -1;
			}
			break;

		//	Special
		case BasicCallExpression::BCK_CONDITIONAL:
			if (m_firstParent == &basic.getArgument(0))
			{
				m_result = 1;
			}
			else if (m_secondParent == &basic.getArgument(0))
			{
				m_result = -1;
			}
			else
			{
				m_result = 0;
			}
			break;
		OPS_DEFAULT_CASE_LABEL
		}
	}

	void visit(SubroutineCallExpression&)
	{
		// порядок вычисления аргументов не определен
		m_result = 0;
	}

	void visit(ForStatement& forStmt)
	{
		if (m_firstParent == &forStmt.getInitExpression())
		{
			m_result = 1;
		}
		else if (m_secondParent == &forStmt.getInitExpression())
		{
			m_result = -1;
		}
		else
		{
			m_result = 0;
		}
	}

	void visit(Canto::HirFImpliedDoExpression& implDo)
	{
		if (m_firstParent == &implDo.getInitExpression())
		{
			m_result = 1;
		}
		else if (m_secondParent == &implDo.getInitExpression())
		{
			m_result = -1;
		}
		else
		{
			m_result = 0;
		}
	}


	void visit(Canto::HirFIntrinsicCallExpression&)
	{
		// порядок вычисления аргументов не определен
		m_result = 0;
	}

private:

	RepriseBase* m_firstParent, *m_secondParent;
	int m_result;
};

int getExpressionOrder(Reprise::ExpressionBase &first, Reprise::ExpressionBase &second)
{
	if (&first == &second)
		return 0;

	RepriseBase* firstParent = 0, *secondParent = 0;
	// Получить первого общего предка
	RepriseBase* parent = Shared::getFirstCommonParentEx(first, second, firstParent, secondParent);

	if (parent == 0)
	{
		throw OPS::RuntimeError("Expressions have no any common parent");
	}
	else if (parent == &first)
	{
		return -1;
	}
	else if (parent == &second)
	{
		return 1;
	}
	else
	{
		ExpressionOrderWalker w(*firstParent, *secondParent);
		parent->accept(w);
		return w.getResult();
	}
}

}
}
