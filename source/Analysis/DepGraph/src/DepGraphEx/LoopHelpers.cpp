#include "Analysis/DepGraph/DepGraphEx/LoopHelpers.h"

#include "OPS_Core/msc_leakcheck.h"
#include "OPS_Core/Localization.h"
#include "Shared/LinearExpressionsUtils.h"

namespace DepGraph
{
	using namespace OPS::Reprise;
	using namespace Id;

	int getAsInt(const AutoExprNode& pNode)
	{
		// Является ли выражение непосредственным значением ?
		if(BasicLiteralExpression* pExprImm = dynamic_cast<BasicLiteralExpression*>(pNode.get()))
		{
			// Является ли он целым значением ?
			if( pExprImm->getLiteralType() == BasicLiteralExpression::LT_INTEGER )
			{
				return (int)pExprImm->getInteger();
			}
			else throw OPS::Exception( _TL("Expression is not integer value.",""));
		}
		else if(StrictLiteralExpression* pExpr = dynamic_cast<StrictLiteralExpression*>(pNode.get()))
		{
			// Является ли он целым значением ?
			switch(pExpr->getLiteralType())
			{
			case  BasicType::BT_INT8:
				return pExpr->getInt8();
				break;
			case  BasicType::BT_INT16:
				return pExpr->getInt16();
				break;
			case  BasicType::BT_INT32:
				return pExpr->getInt32();
			    break;
			case  BasicType::BT_INT64:
				return (int)pExpr->getInt64();
			    break;
			case  BasicType::BT_UINT8:
				return pExpr->getUInt8();
				break;
			case  BasicType::BT_UINT16:
				return pExpr->getUInt16();
				break;
			case  BasicType::BT_UINT32:
				return pExpr->getUInt32();
				break;
			case  BasicType::BT_UINT64:
				return (int)pExpr->getUInt64();
				break;
			default:
				throw OPS::Exception( _TL("Expression is not integer value.",""));
			    break;
			}
		}
		else throw OPS::Exception( _TL("Expression is not immediate value: ","") + pNode->dumpState() );
	}


class ExprParser : public OPS::BaseVisitor,
	public OPS::Visitor<BasicCallExpression>,
	public OPS::Visitor<SubroutineCallExpression>,
	public OPS::Visitor<ReferenceExpression>,
	public OPS::Visitor<StructAccessExpression>,
	public OPS::Visitor<EnumAccessExpression>
{
	typedef std::vector< VariableDeclaration*> NamesList;
	NamesList			m_lNames;
	bool				m_bValid;
public:
	void visit(BasicCallExpression& callExpr)
	{
		BasicCallExpression::BuiltinCallKind kindOfExpr = callExpr.getKind();
		if (kindOfExpr == BasicCallExpression::BCK_ARRAY_ACCESS || kindOfExpr == BasicCallExpression::BCK_ASSIGN ||
			kindOfExpr == BasicCallExpression::BCK_COMMA)
			m_bValid = false;
		else 
			for (int i=0; i<callExpr.getArgumentCount(); i++)
			{
				callExpr.getArgument(i).accept(*this);
			}
	}
	/*void visit(Canto::HirCCallExpression& callExpr)
	{
		Canto::HirCCallExpression::HirCOperatorKind kindOfExpr = callExpr.getKind();
		if (kindOfExpr == Canto::HirCCallExpression::HIRC_ARRAY_ACCESS || kindOfExpr == Canto::HirCCallExpression::HIRC_ASSIGN ||
			kindOfExpr == Canto::HirCCallExpression::HIRC_COMMA)
			m_bValid = false;
		else 
			for (int i=0; i<callExpr.getArgumentCount(); i++)
			{
				callExpr.getArgument(i).accept(*this);
			}
	}*/
	void visit(SubroutineCallExpression&)
	{
		m_bValid = false;
	}
	void visit(ReferenceExpression& rExprData)
	{
		if( std::find(m_lNames.begin(), m_lNames.end(), &rExprData.getReference()) == m_lNames.end() )
			m_lNames.push_back(&rExprData.getReference());		
	}
	/*void visit(SubroutineReferenceExpression& rExprData)
	{
		if( std::find(m_lNames.begin(), m_lNames.end(), &rExprData.getReference()) == m_lNames.end() )
			m_lNames.push_back(&rExprData.getReference());		
	}*/
	void visit(StructAccessExpression&)
	{
		m_bValid = false;
	}
	void visit(EnumAccessExpression&)
	{
		m_bValid = false;
	}
	
	ExprParser()
		:m_bValid(true)
	{
	}

	static bool Parse(const ExpressionBase* _pNode, LinearExpr& rLinExpr)
	{
		ExpressionBase* pNode = const_cast<ExpressionBase*>(_pNode);
		ExprParser	parser;
		pNode->accept(parser);
		if( !parser.m_bValid )
			return false;

		rLinExpr.clear();

		NamesList::iterator	it = parser.m_lNames.begin(), itEnd = parser.m_lNames.end();
		for(; it != itEnd; ++it)
		{
			AutoExprNode	pCoef( Id::getCoef(*pNode, **it) );
			if( pCoef.get() && pCoef->is_a<StrictLiteralExpression>() )
			{
				StrictLiteralExpression* pImm = dynamic_cast<StrictLiteralExpression*>( pCoef.get() );
				if( pImm->isInteger() )
				{
					rLinExpr += LinearExpr(*it, (int)pImm->getInt32());
				}
				else return false;
			}
			else return false;
		}

		// Получаем свободную константу
		AutoExprNode pFree( getFreeCoef(*pNode, parser.m_lNames) );
		if( pFree.get() && pFree->is_a<StrictLiteralExpression>() )
		{
			StrictLiteralExpression* pImm = dynamic_cast<StrictLiteralExpression*>( pFree.get() );
			if( pImm->isInteger() )
			{
				rLinExpr += (int)pImm->getInt32();
				return true;
			}
			else return false;
		}
		else return false;
	}
};

LoopDescr::LoopDescr()
	:m_bValidBounds(false)
	,counterIter(0)
	,m_pFor(0)
{
}

LoopDescr::LoopDescr(const LoopInfo& el)
	:m_bValidBounds(false)
	,counterIter(el.m_oDataObject)
	,m_pFor(el.pStmtFor)	
{
	if (Editing::forIsBasic(*m_pFor))
	{
		if (ExprParser::Parse(&Editing::getBasicForInitExpression(*m_pFor), m_cDwBound))
		{
			m_bValidBounds = true;
		}

		if (ExprParser::Parse(&Editing::getBasicForFinalExpression(*m_pFor), m_cUpBound))
		{
			m_bValidBounds = true;
		}

		if (ExprParser::Parse(&Editing::getBasicForStep(*m_pFor), m_cStep))
		{
			m_bValidBounds = true;
		}
	}
};

AutoExprNode makeLECondition(const LinearExpr& expr, int right)
{
	AutoExprNode pBase = OPS::Shared::summandsMapToReprise(expr.getMap());
	if( pBase.get() )
	{
		/*AutoExprNode pResult( 
			ExprBinary::create( ExprOper::OT_LESSEQ, pBase.release(), ExprImm::create( ImmValue::IV_INT, right - expr.getFreeCoef()) )
							);*/
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_LESS_EQUAL);
		temp->addArgument(pBase.release());
		temp->addArgument(BasicLiteralExpression::createInteger(right - expr.getFreeSummand()));
		AutoExprNode pResult(temp);
		return pResult;
	}
	else
	{
		return AutoExprNode( BasicLiteralExpression::createBoolean(expr.getFreeSummand() <= right ? true : false) );
	}
}

AutoExprNode makeMODCondition(const LinearExpr& expr, int divisor)
{
	// На 1 делится все!
	if( divisor == 1 )
		return AutoExprNode( BasicLiteralExpression::createBoolean(true) );

	AutoExprNode pBase = OPS::Shared::summandsMapToReprise(expr.getMap());
	if( pBase.get() )
	{
		/*AutoExprNode pResult( 
			ExprBinary::create( ExprOper::OT_MOD, 
			expr.getFreeCoef() == 0 ? pBase.release() : 
				ExprBinary::create( ExprOper::OT_PLUS, pBase.release(), ExprImm::create(ImmValue::IV_INT, expr.getFreeCoef()) ),
			ExprImm::create( ImmValue::IV_INT, divisor) 
			)
			);*/
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_INTEGER_MOD);
		ExpressionBase* pArg;
		if (expr.getFreeSummand() == 0)
		{
			pArg = pBase.release();
		}
		else
		{
			BasicCallExpression* tempArg = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
			tempArg->addArgument(pBase.release());
			tempArg->addArgument(BasicLiteralExpression::createInteger(expr.getFreeSummand()));
			pArg = tempArg;
		}
		temp->addArgument(pArg);
		temp->addArgument(BasicLiteralExpression::createInteger(divisor));
		AutoExprNode pResult(temp);
		return pResult;
	}
	else
	{
		return AutoExprNode( BasicLiteralExpression::createBoolean(expr.getFreeSummand() % divisor ? true : false) );
	}
}

AutoExprNode makeGECondition(const LinearExpr& expr, int right)
{
	AutoExprNode pBase = OPS::Shared::summandsMapToReprise(expr.getMap());
	if( pBase.get() )
	{
		/*AutoExprNode pResult( 
			ExprBinary::create( ExprOper::OT_GREATEREQ, pBase.release(), ExprImm::create( ImmValue::IV_INT, right - expr.getFreeCoef()) )
			);*/
		BasicCallExpression* temp = new BasicCallExpression(BasicCallExpression::BCK_GREATER_EQUAL);
		temp->addArgument(pBase.release());
		temp->addArgument( BasicLiteralExpression::createInteger(right - expr.getFreeSummand()) );
		AutoExprNode pResult(temp);
		return pResult;
	}
	else
	{
		return AutoExprNode( BasicLiteralExpression::createBoolean(expr.getFreeSummand() >= right ? true : false) );
	}
}

}
