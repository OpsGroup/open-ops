#include "Analysis/DepGraph/DepGraphEx/Simplifier.h"
#include <memory>

#include "OPS_Core/msc_leakcheck.h"

namespace DepGraph
{
using namespace OPS;
//using namespace OPS::Reprise;

class SimplifyVisitor : public BaseVisitor,
						public Visitor<BasicCallExpression>,
						//public Visitor<Canto::HirCCallExpression>,
						public Visitor<ReferenceExpression>,
						public Visitor<SubroutineReferenceExpression>,
						public Visitor<StructAccessExpression>,
						public Visitor<EnumAccessExpression>,
						public Visitor<SubroutineCallExpression>,
						public Visitor<LiteralExpression>
{
	const SimplifiersList* m_pSimplifiers;
	ExpressionBase* simplify(ExpressionBase* pExpr, GetTypeVisitor::NodeKind objType);
public:
	SimplifyVisitor(const SimplifiersList* pSimplifiers)
		:m_pSimplifiers(pSimplifiers)
		,pResult(0)
	{
	}
	void visit(BasicCallExpression& rCallExpr);
	//void visit(Canto::HirCCallExpression& rCallExpr);
	void visit(ReferenceExpression& rDataExpr);
	void visit(SubroutineReferenceExpression& rArrayExpr);
	void visit(StructAccessExpression& rAssignExpr);
	void visit(EnumAccessExpression& rCallExpr);
	void visit(SubroutineCallExpression& rImmExpr);
	void visit(LiteralExpression& rImmExpr);
	ExpressionBase* pResult;
};

// Собственно функция, которая применяет преобразования
ExpressionBase* SimplifyVisitor::simplify(ExpressionBase* pExpr, GetTypeVisitor::NodeKind objType)
{
	ExpressionBase* pCurrExpr = pExpr;
	// Будем последовавательно брать преобразования из списка и
	// применять их к исходному выражению
	SimplifiersList::const_iterator iter = m_pSimplifiers->begin(),
				end = m_pSimplifiers->end();
	GetTypeVisitor visitior;
	while(iter != end)
	{
		if( objType == iter->second )
		{
			ExpressionBase* pSimpleExpr = iter->first(pCurrExpr);
			// Если удалось применить преобразование...
			if( pSimpleExpr )
			{
				// Устанавливаем упрощенное выражение текущим
				if( pCurrExpr != pExpr )
					delete pCurrExpr;
				pCurrExpr = pSimpleExpr;
				pCurrExpr->accept(visitior);
				objType = visitior.m_typeOfNode;
				// Начинаем цикл упрощений заново
				iter = m_pSimplifiers->begin();
			}
			else
			{
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}
	return pCurrExpr == pExpr ? 0 : pCurrExpr;
}

void SimplifyVisitor::visit(BasicCallExpression& rCallExpr)
{
	if (rCallExpr.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS || rCallExpr.getKind() == BasicCallExpression::BCK_ASSIGN)
	{
		pResult = 0;
	}
	else
	{
		for (int i=0; i<rCallExpr.getArgumentCount(); i++)
		{
			pResult = 0;
			rCallExpr.getArgument(i).accept(*this);
			// Если левое подвыражение преобразовалось
			if( pResult )
				rCallExpr.setArgument(i,pResult);
	
		}
		pResult = simplify(&rCallExpr, GetTypeVisitor::NK_BasicCallExpression);
	}
}

void SimplifyVisitor::visit(ReferenceExpression& /*rDataExpr*/)
{
	pResult = 0;
}

void SimplifyVisitor::visit(SubroutineReferenceExpression& /*rDataExpr*/)
{
	pResult = 0;
}

void SimplifyVisitor::visit(StructAccessExpression& /*rArrayExpr*/)
{
	pResult = 0;
}

void SimplifyVisitor::visit(EnumAccessExpression& /*rAssignExpr*/)
{
	pResult = 0;
}

void SimplifyVisitor::visit(SubroutineCallExpression& /*rCallExpr*/)
{
	pResult = 0;
}

void SimplifyVisitor::visit(LiteralExpression& /*rImmExpr*/)
{
	pResult = 0;
}

ExpressionBase* simplify(ExpressionBase* pExpr, const SimplifiersList* pSimplifiers /* = &g_lStdSimplifiers */)
{
	if( !pExpr ) return 0;

	SimplifyVisitor visitor(pSimplifiers);
	pExpr->accept(visitor);
	return visitor.pResult ? visitor.pResult : pExpr;
}




/* ================================================================================================= */
/* ================================================================================================= */
/* ================================================================================================= */


//ImmValue IMM_FALSE(ImmValue::IV_BOOL, false);
//ImmValue IMM_TRUE(ImmValue::IV_BOOL, true);

ExpressionBase* AndSimp(const BasicCallExpression* pExpr)
{
	if( pExpr->getKind() != BasicCallExpression::BCK_LOGICAL_AND )
		return 0;

	const BasicLiteralExpression* pLeftExpr = dynamic_cast<const BasicLiteralExpression*>(&pExpr->getArgument(0));
	const BasicLiteralExpression* pRightExpr = dynamic_cast<const BasicLiteralExpression*>(&pExpr->getArgument(1));

	if( pLeftExpr )
	{
		if( pLeftExpr->getBoolean() == true )
		{
			if( pRightExpr )
			{
				if( pRightExpr->getBoolean() == true )
				{
					return BasicLiteralExpression::createBoolean(true);
				}
				else if( pRightExpr->getBoolean() == false )
				{
					return BasicLiteralExpression::createBoolean(false);
				}
			}
			return pExpr->getArgument(1).clone();
		}
		else if( pLeftExpr->getBoolean() == false )
		{
			return BasicLiteralExpression::createBoolean(false);
		}
	}

	if( pRightExpr )
	{
		if( pRightExpr->getBoolean() == true )
		{
			return pExpr->getArgument(1).clone();
		}
		else if( pRightExpr->getBoolean() == false )
		{
			return BasicLiteralExpression::createBoolean(false);
		}
	}

	return 0;
}

ExpressionBase* UnMinBinPlusSimp(const BasicCallExpression* pExpr)
{
	if( pExpr->getKind() != BasicCallExpression::BCK_BINARY_PLUS )
		return 0;

	const BasicCallExpression* pLeftExpr = dynamic_cast<const BasicCallExpression*>(&pExpr->getArgument(0));
	const BasicCallExpression* pRightExpr = dynamic_cast<const BasicCallExpression*>(&pExpr->getArgument(1));

	if( pLeftExpr )
	{
		if( pLeftExpr->getKind() == BasicCallExpression::BCK_UNARY_MINUS )
		{
			/*return ExprBinary::create(ExprOper::OT_MINUS, pExpr->getRightArg().clone(),
				pLeftExpr->getArg().clone());*/
			BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
			temp->addArgument(pExpr->getArgument(1).clone());
			temp->addArgument(pLeftExpr->getArgument(0).clone());
			return temp;
		}
	}

	if( pRightExpr )
	{
		if( pRightExpr->getKind() == BasicCallExpression::BCK_UNARY_MINUS )
		{
			/*return ExprBinary::create(ExprOper::OT_MINUS, pExpr->getLeftArg().clone(),
				pRightExpr->getArg().clone());*/
			BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS);
			temp->addArgument(pExpr->getArgument(0).clone());
			temp->addArgument(pRightExpr->getArgument(0).clone());
			return temp;
		}
	}
	return 0;
}
//!!!!!!!!!!!!!!!NB!!!!!!!!!!!!!!!
ExpressionBase* VarMinVarSimp(const BasicCallExpression* pExpr)
{
	if( pExpr->getKind() != BasicCallExpression::BCK_BINARY_MINUS )
		return 0;
    
	const ReferenceExpression* pLeftExpr = dynamic_cast<const ReferenceExpression*>(&pExpr->getArgument(0));
	const ReferenceExpression* pRightExpr = dynamic_cast<const ReferenceExpression*>(&pExpr->getArgument(1));

	if( pLeftExpr && pRightExpr )
	{
		if( &pLeftExpr->getReference() == &pRightExpr->getReference() )
		{
			return BasicLiteralExpression::createInteger(0);
		}
	}

	return 0;
}

ExpressionBase* ImmOperImmSimp(const BasicCallExpression* pExpr)
{
	if (pExpr->getArgumentCount()==2)
	{
		const BasicLiteralExpression* pLeftExpr = dynamic_cast<const BasicLiteralExpression*>(&pExpr->getArgument(0));
		const BasicLiteralExpression* pRightExpr = dynamic_cast<const BasicLiteralExpression*>(&pExpr->getArgument(1));
	
	    if( !pLeftExpr || !pRightExpr)
			return 0;
	
		switch( pExpr->getKind() )
		{
		case BasicCallExpression::BCK_BINARY_PLUS:
			return BasicLiteralExpression::createInteger(pLeftExpr->getInteger() + pRightExpr->getInteger());
		case BasicCallExpression::BCK_BINARY_MINUS:
			return BasicLiteralExpression::createInteger(pLeftExpr->getInteger() - pRightExpr->getInteger());
		case BasicCallExpression::BCK_MULTIPLY:
			return BasicLiteralExpression::createInteger(pLeftExpr->getInteger() * pRightExpr->getInteger());
		case BasicCallExpression::BCK_DIVISION:
			return BasicLiteralExpression::createInteger(pLeftExpr->getInteger() / pRightExpr->getInteger());
		case BasicCallExpression::BCK_INTEGER_MOD:
			return BasicLiteralExpression::createInteger(pLeftExpr->getInteger() % pRightExpr->getInteger());
		case BasicCallExpression::BCK_LESS:
			return BasicLiteralExpression::createBoolean(pLeftExpr->getInteger() < pRightExpr->getInteger());
		case BasicCallExpression::BCK_GREATER:
			return BasicLiteralExpression::createBoolean(pLeftExpr->getInteger() > pRightExpr->getInteger());
		case BasicCallExpression::BCK_GREATER_EQUAL:
			return BasicLiteralExpression::createBoolean(pLeftExpr->getInteger() >= pRightExpr->getInteger());
		case BasicCallExpression::BCK_LESS_EQUAL:
			return BasicLiteralExpression::createBoolean(pLeftExpr->getInteger() <= pRightExpr->getInteger());
		case BasicCallExpression::BCK_EQUAL:
			return BasicLiteralExpression::createBoolean(pLeftExpr->getInteger() == pRightExpr->getInteger());
		case BasicCallExpression::BCK_NOT_EQUAL:
			return BasicLiteralExpression::createBoolean(pLeftExpr->getInteger() != pRightExpr->getInteger());
		case BasicCallExpression::BCK_LOGICAL_AND:
			return BasicLiteralExpression::createBoolean(pLeftExpr->getBoolean() && pRightExpr->getBoolean());
		case BasicCallExpression::BCK_LOGICAL_OR:
			return BasicLiteralExpression::createBoolean(pLeftExpr->getBoolean() || pRightExpr->getBoolean());
		default:
			return 0;
		}
	}

	if(pExpr->getArgumentCount()==1)
	{
		const BasicLiteralExpression* pArg = dynamic_cast<const BasicLiteralExpression*>(&pExpr->getArgument(0));

		if( !pArg )
			return 0;

		switch( pExpr->getKind() ) 
		{
		case BasicCallExpression::BCK_UNARY_PLUS:
			BasicLiteralExpression::createInteger(pArg->getInteger());
			//pArg->clone();
			break;
		case BasicCallExpression::BCK_UNARY_MINUS:
			return BasicLiteralExpression::createInteger( -pArg->getInteger());
			break;
		case BasicCallExpression::BCK_LOGICAL_NOT:
			return BasicLiteralExpression::createBoolean(!pArg->getBoolean());
		default:
			return 0;
		}
	}
	return 0;
}

//ExpressionBase* ImmOperImmSimp(const BasicCallExpression* pExpr)
//{
//	if(pExpr->getArgumentCount()!=1)
//		return 0;
//
//	const BasicLiteralExpression* pArg = dynamic_cast<const BasicLiteralExpression*>(&pExpr->getArgument(0));
//
//	if( !pArg )
//		return 0;
//
//	switch( pExpr->getKind() ) 
//	{
//	case BasicCallExpression::BCK_UNARY_PLUS:
//		BasicLiteralExpression::createInteger(pArg->getInteger());
//		//pArg->clone();
//		break;
//	case BasicCallExpression::BCK_UNARY_MINUS:
//		return BasicLiteralExpression::createInteger( -pArg->getInteger());
//		break;
//	case BasicCallExpression::BCK_LOGICAL_NOT:
//		return BasicLiteralExpression::createBoolean(!pArg->getBoolean());
//	default:
//		return 0;
//	}
//}

ExpressionBase* IdUsingSimp(BasicCallExpression* /*pExpr*/)
{
	return 0;
}

void SimplifiersList::push_back(const PSimpFuncBinary pSimp)
{
	m_cCont.push_back(std::make_pair(PSimpFuncGeneric(pSimp), GetTypeVisitor::NK_BasicCallExpression) );
}

//void SimplifiersList::push_back(const PSimpFuncUnary pSimp)
//{
//	m_cCont.push_back(std::make_pair(PSimpFuncGeneric(pSimp), OpsNewIr::OT_EXPR_UNARY));
//}

void SimplifiersList::push_back(const PSimpFuncData pSimp)
{
	m_cCont.push_back(std::make_pair(PSimpFuncGeneric(pSimp), GetTypeVisitor::NK_ReferenceExpression));
}

SimplifiersList::const_iterator SimplifiersList::begin() const
{
	return m_cCont.begin();
}

SimplifiersList::const_iterator SimplifiersList::end() const
{
	return m_cCont.end();
}

}
