#include <utility>
#include <algorithm>
#include "Analysis/DepGraph/DepGraphEx/DepGraphEx.h"
#include "Analysis/DepGraph/DepGraphEx/Simplifier.h"
#include "Analysis/DepGraph/DepGraphEx/LoopHelpers.h"
#include "Analysis/DepGraph/DepGraphEx/Predicates.h"
#include "Shared/LinearExpressionsUtils.h"
#include "OPS_Core/msc_leakcheck.h"

namespace DepGraph
{

using namespace std;
using namespace OPS;
using namespace Id;
using namespace OPS::Reprise;

ConditionTableMarker condTableMarker;

ConditionTableMarker::~ConditionTableMarker()
{
	TMarkedNodes::iterator it = m_marks.begin();
	for(; it != m_marks.end(); ++it) delete it->second;
}

void WriteLog(const std::string& message, OPS::Console::MessageLevel level)
{
	getOutputConsole("DepGraphEx").log(level, message);
}

GlobalConditionTable::ConstIterator GlobalConditionTable::Begin() const
{
	return m_mTable.begin();
}
GlobalConditionTable::ConstIterator GlobalConditionTable::End() const
{
	return m_mTable.end();
}
GlobalConditionTable::ConstIterator 
	GlobalConditionTable::Find(const std::pair<const ReferenceExpression*,const ReferenceExpression*>& pSrcDest) const
{
	OPS_ASSERT(pSrcDest.first);
	OPS_ASSERT(pSrcDest.second);
	return m_mTable.find(pSrcDest);
}

void GlobalConditionTable::Add(const std::pair<const ReferenceExpression*,const ReferenceExpression*>& pSrcDest, const TableRec& rec)
{
	OPS_ASSERT(pSrcDest.first);
	OPS_ASSERT(pSrcDest.second);
	m_mTable.insert(make_pair(pSrcDest,rec));
}

void GlobalConditionTable::Delete(const std::pair<const ReferenceExpression*, const ReferenceExpression*>& pSrcDest)
{
	Iterator it = m_mTable.find(pSrcDest);
	if (it != End())
	{
		m_mTable.erase(it);
	}
}

DepGraph::GlobalConditionTable* GlobalConditionTable::Get(ProgramUnit& rProg)
{
	/*ImmValue* pNote = rProg.getNote("GlobalConditionTable");
	if (pNote) {
		return dynamic_cast<DepGraph::GlobalConditionTable*>(pNote);
	}
	else {
		DepGraph::GlobalConditionTable* pTable = new DepGraph::GlobalConditionTable();
		rProg.setNote("GlobalConditionTable", pTable);
		return pTable;
	}*/
	if (condTableMarker.hasMarks(rProg))
	{
		return condTableMarker.getMark(rProg);
	}
	else
	{
		DepGraph::GlobalConditionTable* pTable = new DepGraph::GlobalConditionTable();
		condTableMarker.addMark(&rProg,pTable);
		return pTable;
	}
}

const int ALLSUPP = -4;

class DepGraphEx : public IDepGraphEx
{
	typedef std::map<int, OccurDesc*> OccurrenceMap;
	typedef std::map<int, IPredicate*> CondMap;
	typedef std::map<LampArrow*, CondMap> EdgesMap;

	EdgesMap		m_mEdges;
	OccurrenceMap	m_mOccurrenceMap;

	void buildIR(IndOccurContainer& rOccursContainer);
	void buildGraph(LamportGraph& rLamportGraph);

public:
	DepGraphEx(IndOccurContainer& rOccursContainer, LamportGraph& rLamportGraph);
	void build(IndOccurContainer& rOccursContainer, LamportGraph& rLamportGraph);

	PredicatePtr getDepCond(LampArrow* pArrow) 
	{ 
		EdgesMap::const_iterator pConds = m_mEdges.find(pArrow);
		if( pConds == m_mEdges.end() )
			return PredicatePtr(ConstPred::Create(true));
		else
		{
			return PredicatePtr(m_mEdges[pArrow][ALLSUPP]->Clone());
		}
	}
	//AutoExprNode getNoDepCond(LampArrow* pArrow) { return AutoExprNode(); }
	PredicatePtr getDepSuppCond(LampArrow* /*pArrow*/, int /*nSupp*/) { return PredicatePtr(); }				// Заглушки!
	bool hasDepCond(LampArrow* /*pArrow*/) {return true;}													// Заглушки!
	//AutoExprNode getNoDepSuppCond(LampArrow* pArrow, int nSupp) { return AutoExprNode(); }
	~DepGraphEx();
};

// НОД-тест на носитель supp
PredicatePtr gcdTestSymbolicKnownBounds( const OccurDesc& rOccur1, const OccurDesc& rOccur2, unsigned supp )
{
	int gcd = 0;

	for( unsigned i = 0; i < supp; i++ )
	{
        gcd = OPS::LatticeGraph::calculateGCD( gcd, rOccur1.d1[i] - rOccur2.d1[i] );
	}

	for( int i = supp; i < rOccur1.loopNumb; i++)
        gcd = OPS::LatticeGraph::calculateGCD( gcd, rOccur1.d1[i] );

	for( int i = supp; i < rOccur2.loopNumb; i++)
        gcd = OPS::LatticeGraph::calculateGCD( gcd, rOccur2.d1[i] );

	//	НОД тест...
	if( gcd )
	{
		LinearExpr	cExpr;
		
		cExpr.add(rOccur1.d1[rOccur1.loopNumb ] -
				  rOccur2.d1[rOccur2.loopNumb ]);
		cExpr.add(rOccur1.m_externalParamCoefsMapped.getOpposite());
		cExpr.add(rOccur2.m_externalParamCoefsMapped.getOpposite());

		return PredicatePtr( MODPred::Create(cExpr, gcd));
	}
	else 
		return PredicatePtr( ConstPred::Create(true) );
}

PredicatePtr banerjeeTestSymbolicMapped( const OccurDesc& rOccur1, const OccurDesc& rOccur2, unsigned supp, bool bLIDtest)
{
	LinearExpr sum_left, sum_right;
	int		sum_a = 0, sum_b = 0;

	for(unsigned  i=0; i < supp; i++)
	{
		sum_left -=  ( rOccur1.loops[i].loopBounds.getUpperFreeExpression() - 1 )*NEG( rOccur1.d1[i] - rOccur2.d1[i] );
		sum_right += ( rOccur1.loops[i].loopBounds.getUpperFreeExpression() - 1 )*POS( rOccur1.d1[i] - rOccur2.d1[i] );
	}

	for( int i = supp+1; i < rOccur1.loopNumb; i++)
	{
		sum_left -=  ( rOccur1.loops[i].loopBounds.getUpperFreeExpression() - 1 )*NEG( rOccur1.d1[i] );
		sum_right += ( rOccur1.loops[i].loopBounds.getUpperFreeExpression() - 1 )*POS( rOccur1.d1[i] );
	}

	for( int i = supp+1; i < rOccur2.loopNumb; i++)
	{
		sum_left -=  ( rOccur2.loops[i].loopBounds.getUpperFreeExpression() - 1 )*POS( rOccur2.d1[i] );
		sum_right += ( rOccur2.loops[i].loopBounds.getUpperFreeExpression() - 1 )*NEG( rOccur2.d1[i] );
	}


	for( int i = 0; i <= rOccur1.loopNumb; i++)
		sum_a += rOccur1.d1[i];

	for( int i = 0; i <= rOccur2.loopNumb; i++)
		sum_b += rOccur2.d1[i];


	LinearExpr		exprLeft = rOccur1.m_externalParamCoefsMapped - rOccur2.m_externalParamCoefsMapped;
	LinearExpr		exprRight = rOccur1.m_externalParamCoefsMapped - rOccur2.m_externalParamCoefsMapped;

	if( !bLIDtest )
	{
		// Если это не LID тест
		exprLeft += sum_left - rOccur2.d1[supp];
		exprLeft -= ( rOccur1.loops[supp].loopBounds.getUpperFreeExpression() - 2)*POS( NEG( rOccur1.d1[supp] )+ rOccur2.d1[supp] );

		exprRight += sum_right - rOccur2.d1[supp];
		exprRight += ( rOccur1.loops[supp].loopBounds.getUpperFreeExpression() - 2)*POS( POS( rOccur1.d1[supp] ) - rOccur2.d1[supp] );
	}
	else
	{
		// Если это LID тест
		exprLeft += sum_left;
		exprRight += sum_right;
	}
	
	PredicatePtr pGEPred( InequalPred::Create(exprLeft, LinearExpr(sum_b - sum_a)));
	PredicatePtr pLEPred( InequalPred::Create(LinearExpr(sum_b-sum_a), exprRight));
	return PredicatePtr( CreateAndPredicate(*pGEPred, *pLEPred));
}

PredicatePtr banerjeeTestSymbolicUnmapped( const OccurDesc& rOccur1, const OccurDesc& rOccur2, int carrier, int commonLoopNumb)
{
	if (rOccur1.dim != rOccur2.dim)
		return PredicatePtr(ConstPred::Create(true));

	LinearExpr sum_left, sum_right;
	int		sum_a = 0, sum_b = 0;

	PredicatePtr dependPred(ConstPred::Create(true));

	// Определяем какой тест выполнять.
	// lid - loop independent dependence
	const bool lidTest = carrier == commonLoopNumb;

	for (int j = 0; j < rOccur1.dim && isTrue(*dependPred); ++j)
	{
		for(unsigned  i = 0; i < unsigned(carrier); ++i)
		{
			sum_left -=  ( rOccur1.loops[i].loopBounds.getUpperFreeExpression() - 1 )*NEG( rOccur1.data[j][i] - rOccur2.data[j][i] );
			sum_right += ( rOccur1.loops[i].loopBounds.getUpperFreeExpression() - 1 )*POS( rOccur1.data[j][i] - rOccur2.data[j][i] );
		}

		for(int i = (lidTest ? carrier : carrier + 1); i < rOccur1.loopNumb; ++i)
		{
			sum_left -=  ( rOccur1.loops[i].loopBounds.getUpperFreeExpression() - 1 )*NEG( rOccur1.data[j][i] );
			sum_right += ( rOccur1.loops[i].loopBounds.getUpperFreeExpression() - 1 )*POS( rOccur1.data[j][i] );
		}

		for(int i = (lidTest ? carrier : carrier + 1); i < rOccur2.loopNumb; ++i)
		{
			sum_left -=  ( rOccur2.loops[i].loopBounds.getUpperFreeExpression() - 1 )*POS( rOccur2.data[j][i] );
			sum_right += ( rOccur2.loops[i].loopBounds.getUpperFreeExpression() - 1 )*NEG( rOccur2.data[j][i] );
		}

		for( int i = 0; i <= rOccur1.loopNumb; i++)
			sum_a += rOccur1.data[j][i];

		for( int i = 0; i <= rOccur2.loopNumb; i++)
			sum_b += rOccur2.data[j][i];

		LinearExpr		exprLeft = rOccur1.m_externalParamCoefs[j] - rOccur2.m_externalParamCoefs[j];
		LinearExpr		exprRight = rOccur1.m_externalParamCoefs[j] - rOccur2.m_externalParamCoefs[j];

		if (lidTest)
		{
			// Если это LID тест
			exprLeft += sum_left;
			exprRight += sum_right;
		}
		else
		{
			// Если это не LID тест
			exprLeft += sum_left - rOccur2.data[j][carrier];
			exprLeft -= ( rOccur1.loops[carrier].loopBounds.getUpperFreeExpression() - 2)*POS( NEG( rOccur1.data[j][carrier] )+ rOccur2.data[j][carrier] );

			exprRight += sum_right - rOccur2.data[j][carrier];
			exprRight += ( rOccur1.loops[carrier].loopBounds.getUpperFreeExpression() - 2)*POS( POS( rOccur1.data[j][carrier] ) - rOccur2.data[j][carrier] );
		}

		PredicatePtr pGEPred( InequalPred::Create(exprLeft, LinearExpr(sum_b - sum_a)));
		PredicatePtr pLEPred( InequalPred::Create(LinearExpr(sum_b-sum_a), exprRight));
		dependPred.reset( CreateAndPredicate(*pGEPred, *pLEPred) );
	}

	return dependPred;
}

DepGraphEx::DepGraphEx(IndOccurContainer& rOccursContainer, LamportGraph& rLamportGraph)
{
	build(rOccursContainer, rLamportGraph);
}

class FindByFor
{
	const ForStatement*	pFor;
public:
	FindByFor(const ForStatement* _pFor)
		:pFor(_pFor)	{}
	bool operator()(const LoopDesc& ld)
	{
		return pFor == ld.stmtFor;
	}
};

void DepGraphEx::buildIR(IndOccurContainer& rOccursContainer)
{
	OccurList& indGenList = rOccursContainer.GetIndGenList();
	OccurList& indUtilList = rOccursContainer.GetIndUtilList();

	for(OccurIter it = indGenList.Begin(); it != indGenList.End(); ++it)
	{
		m_mOccurrenceMap[(*it)->GetNumber()] = *it;
	}

	for(OccurIter it = indUtilList.Begin(); it != indUtilList.End(); ++it)
	{
		m_mOccurrenceMap[(*it)->GetNumber()] = *it;
	}
}

bool isLoopsHasParams(OccurDesc& occurr)
{
	bool bLoopsHasParams = false;
	for(int i = 0; i < occurr.loopNumb && !bLoopsHasParams; ++i)
	{
		bLoopsHasParams |= !occurr.loops[i].loopBounds.getUpperFreeExpression().isConstant();
	}

	return bLoopsHasParams;
}

void DepGraphEx::buildGraph(LamportGraph& rLamportGraph)
{
	list<LampArrow*>::iterator	iter = rLamportGraph.Begin(),
		end = rLamportGraph.End();
	for(; iter != end; ++iter)
	{
		OccurrenceMap::iterator itSrc = m_mOccurrenceMap.find((*iter)->srcOccurNumb),
								itDep = m_mOccurrenceMap.find((*iter)->depOccurNumb);
		if( itSrc == m_mOccurrenceMap.end() || itDep == m_mOccurrenceMap.end() )
		{
			//throw OPS::Exception( "Invalid Occurrence number found." );
			continue;
		}

		OccurDesc	*pDepOccur = itDep->second,
					*pSrcOccur = itSrc->second;
		if( pDepOccur->d1 == 0 || pSrcOccur->d1 == 0 )
			continue;

		if( !pDepOccur->m_externalParamCoefsMapped.isConstant() || !pSrcOccur->m_externalParamCoefsMapped.isConstant() ||
			isLoopsHasParams(*pDepOccur) || isLoopsHasParams(*pSrcOccur) )
		{
			m_mEdges.insert(make_pair(*iter, CondMap()));

			// Тесты на зависимость с носителем i
			for( unsigned i = 0; i < unsigned((*iter)->commonLoopNumb); i++)
			{
				PredicatePtr pSuppCondGcd( gcdTestSymbolicKnownBounds(*pSrcOccur, *pDepOccur, i) );
				PredicatePtr pSuppCondBan( banerjeeTestSymbolicMapped(*pSrcOccur, *pDepOccur, i));

				m_mEdges[*iter][i] = CreateAndPredicate(*pSuppCondGcd, *pSuppCondBan);
			}

			// Loop independent dependence тест
			if( pDepOccur != pSrcOccur &&
				pDepOccur->GetNumber() > pSrcOccur->GetNumber())
			{
				if( (*iter)->commonLoopNumb )
				{ 
					PredicatePtr pSuppCondGcd( gcdTestSymbolicKnownBounds(*pSrcOccur, *pDepOccur, (*iter)->commonLoopNumb) );
					PredicatePtr pSuppCondBan( banerjeeTestSymbolicMapped(*pSrcOccur, *pDepOccur, (*iter)->commonLoopNumb, true));

					m_mEdges[*iter][LIDEP] = CreateAndPredicate(*pSuppCondGcd, *pSuppCondBan);
				}
				else
				{
					if( (*iter)->TestSupp(LIDEP) )
						m_mEdges[*iter][LIDEP] = ConstPred::Create(true);
					else
						m_mEdges[*iter][LIDEP] = ConstPred::Create(false);
				}
			}


			// Теперь собираем условия по всем носителям в одно
			PredicatePtr	pCond;
			for( CondMap::iterator it = m_mEdges[*iter].begin(), end1 = m_mEdges[*iter].end();
				it != end1; ++it)
			{
				ConstPred* pConstPred = dynamic_cast<ConstPred*>(it->second);
				if( pConstPred && pConstPred->m_bValue )
				{
					// Если по одному носителю true - то дальше нет смысла продолжать
					pCond.reset( pConstPred->Clone() );
					break;
				}

				if( pCond.get() )
				{
					pCond.reset( CreateOrPredicate(*pCond, *it->second) );
				}
				else
				{
					pCond.reset( it->second->Clone() );
				}
			}

			m_mEdges[*iter][ALLSUPP] = pCond.get() ? pCond.release() : ConstPred::Create(true);
		}
	}
}

void DepGraphEx::build(IndOccurContainer& rOccursContainer, LamportGraph& rLamportGraph)
{
	buildIR(rOccursContainer);
	buildGraph(rLamportGraph);
}

DepGraphEx::~DepGraphEx()
{
	EdgesMap::iterator it = m_mEdges.begin(),
			end = m_mEdges.end();
	for( ; it != end; ++it )
	{
		CondMap::iterator it1 = it->second.begin(),
			end1 = it->second.end();
		for( ;it1 != end1; ++it1 )
		{
			delete it1->second;
		}
	}
}

bool testArrowOnFreeParams(LampArrow& rLampArrow)
{
	return  rLampArrow.isStatus(LAS_CR_EXTERNAL_VAR_IN_SUBSCRIPT) ||
			rLampArrow.isStatus(LAS_CR_EXTERNAL_NONLINEAR_EXPR_IN_SUBSCRIPT) ||
			rLampArrow.isStatus(LAS_CR_SOME_COMMON_LOOPS_WASNT_PARSERED_CAREFULLY);

	/*if(OccurDesc* pOccurDesc =  rOccurs[rLampArrow.srcOccurNumb])
	{
		if(pOccurDesc->GetStatus(FREE_IS_PARAMETER))
			return true;
    }
	else throw OPS::Exception("Occurrences are not indexed or Occurrence number is invalid");

	if(OccurDesc* pOccurDesc =  rOccurs[rLampArrow.depOccurNumb])
	{
		if(pOccurDesc->GetStatus(FREE_IS_PARAMETER))
			return true;
	}
	else throw OPS::Exception("Occurrences are not indexed or Occurrence number is invalid");*/
}

void getArrowsWithFreeParams(LamportGraph& rLamp, 
							 std::vector<LampArrow*>& rResult )
{
	for(LampArrowIterator curr = rLamp.Begin(), End = rLamp.End(); curr != End; ++curr)
   	{
		if( testArrowOnFreeParams(**curr ) )
			rResult.push_back( *curr );
	}
}

bool testGraphOnFreeParams(IndOccurContainer& /*rOccurs*/, LamportGraph& rLamportGraph)
{
	LampArrowIterator curr = rLamportGraph.Begin(), End = rLamportGraph.End();
	for( ; curr != End; ++curr )
	{
		try 
		{
			if( testArrowOnFreeParams( **curr ) )
				return true;
		}
		catch(const OPS::Exception& err)
		{
			WriteLog( err.getMessage(), Console::LEVEL_ERROR );
			throw;
		}
	}
	return false;
}

std::unique_ptr<IDepGraphEx> getDepGraphEx(IndOccurContainer& rOccursContainer, LamportGraph& rLamportGraph )
{
	std::unique_ptr<IDepGraphEx> pManager;
	try
	{
		// Пытаемся создать менеджера
		pManager.reset( new DepGraphEx(rOccursContainer, rLamportGraph) );
	}
	catch(const OPS::Exception& e)
	{
		WriteLog( e.getMessage(), Console::LEVEL_ERROR);
	}
	catch(...)
	{
		// Если было выброшено исключение, то ничего не делаем.
		// Автоматически будет возвращен 0.
	}
	return pManager;
}
												// ============ NB! ================
bool isEqualImm(ExpressionBase* pNode, const BasicLiteralExpression& imm)
{
	OPS_ASSERT(pNode);
	if (pNode->is_a<BasicLiteralExpression>())
	{
		BasicLiteralExpression pImmExpr = pNode->cast_to<BasicLiteralExpression>();
		return pImmExpr.getLiteralValueAsString() == imm.getLiteralValueAsString();
	}
	return false;
}

bool isFalse(const IPredicate& pred)
{
	const ConstPred* pConstPred = dynamic_cast<const ConstPred*>(&pred);
	return (pConstPred && false == pConstPred->m_bValue);
}

bool isTrue(const IPredicate& pred)
{
	const ConstPred* pConstPred = dynamic_cast<const ConstPred*>(&pred);
	return (pConstPred && true == pConstPred->m_bValue);
}

class RefMapper : public OPS::NonCopyableMix
{
	typedef std::map<RepriseBase*,RepriseBase*> ObjectsMap;
	ObjectsMap&	m_map;

	inline void addToMap(RepriseBase& first, RepriseBase& second)
	{
		ObjectsMap::iterator	it = m_map.find(&first);
		if (it != m_map.end()) {
			it->second = &second;
		}
	}
public:

	RefMapper(ObjectsMap& om):m_map(om) {}

	void doMapping(ExpressionBase& first, ExpressionBase& second)
	{
		addToMap(first, second);

		CallExpressionBase* firstOper = dynamic_cast<CallExpressionBase*>(&first);
		CallExpressionBase* secondOper = dynamic_cast<CallExpressionBase*>(&second);
		if (firstOper) {
			for(int i = 0; i < firstOper->getArgumentCount(); ++i) {
				doMapping(firstOper->getArgument(i), secondOper->getArgument(i));
			}
		}
	}

	void doMapping(StatementBase& first, StatementBase& second)
	{
		addToMap(first, second);
		GetTypeVisitor visitor;
		first.accept(visitor);
		switch(visitor.m_typeOfNode)
		{
			case GetTypeVisitor::NK_BlockStatement: 
			{
				BlockStatement	&firstBlock = first.cast_to<BlockStatement>(),
								&secondBlock = second.cast_to<BlockStatement>();
				doMapping(firstBlock.getDeclarations(), secondBlock.getDeclarations());
		
				BlockStatement::Iterator fit = firstBlock.getFirst(),
										 sit = secondBlock.getFirst();
				for(; fit.isValid() && sit.isValid(); ++fit, ++sit) {
					doMapping(*fit, *sit);
				}
				
			}
			break;
			case GetTypeVisitor::NK_IfStatement:
			{
				IfStatement  &firstIf  = first.cast_to<IfStatement>(),
							 &secondIf = second.cast_to<IfStatement>();

				doMapping(firstIf.getThenBody(), secondIf.getThenBody());
				doMapping(firstIf.getElseBody(), secondIf.getElseBody());
				doMapping(firstIf.getCondition(), secondIf.getCondition());
			}
			break;
			case GetTypeVisitor::NK_ForStatement:
			{
				ForStatement &firstFor  = first.cast_to<ForStatement>(),
							 &secondFor = second.cast_to<ForStatement>();
				doMapping(firstFor.getBody(), secondFor.getBody());
				doMapping(firstFor.getFinalExpression(), secondFor.getFinalExpression());
				doMapping(firstFor.getInitExpression(), secondFor.getInitExpression());
				doMapping(firstFor.getStepExpression(), secondFor.getStepExpression());
			}
			break;
			case GetTypeVisitor::NK_WhileStatement:
			{
				WhileStatement  &firstWhile  = first.cast_to<WhileStatement>(),
								&secondWhile = second.cast_to<WhileStatement>();
				doMapping(firstWhile.getBody(), secondWhile.getBody());
				doMapping(firstWhile.getCondition(), secondWhile.getCondition());
			}
			break;
			case GetTypeVisitor::NK_ExpressionStatement:
			{
				ExpressionStatement &firstExpr = first.cast_to<ExpressionStatement>(),
									&secondExpr = second.cast_to<ExpressionStatement>();
				doMapping(firstExpr.get(), secondExpr.get());
			}
			break;
			case GetTypeVisitor::NK_ReturnStatement:
				doMapping(first.cast_to<ReturnStatement>().getReturnExpression(), second.cast_to<ReturnStatement>().getReturnExpression());
			break;

			case GetTypeVisitor::NK_GotoStatement:
			case GetTypeVisitor::NK_EmptyStatement:
			break;
			OPS_DEFAULT_CASE_LABEL
		}
	}

	void doMapping(Declarations& first, Declarations& second)
	{
		/*DeclarationBase	*fit = first.getNamesFirst(),
						*sit = second.getNamesFirst();

		for(; fit.isValid() && sit.isValid(); ++fit, ++sit)
		{

			addToMap(*fit, *sit);

			switch(fit->getObjType())
			{
				case OT_NAME_VAR: break;
				case OT_NAME_TYPE: break;
				case OT_NAME_LABEL: break;
				case OT_NAME_SUBPROC:
				{
					Block& fBlock = fit.getNameRef().getAsSubProc().getBody();
					Block& sBlock = sit.getNameRef().getAsSubProc().getBody();
					doMapping(fBlock, sBlock);
				}
				break;
				default:
					OPS::OPSAssert(false, "RefMapper: unexpected name type");
				break;
			}
		}*/
		{
			Declarations::VarIterator firstIter = first.getFirstVar();
			Declarations::VarIterator secondIter = second.getFirstVar();
			while (firstIter.isValid() && secondIter.isValid())
			{
				addToMap(*firstIter, *secondIter);
				++firstIter;
				++secondIter;
			}
		}
		{
			Declarations::TypeIterator firstIter = first.getFirstType();
			Declarations::TypeIterator secondIter = second.getFirstType();
			while (firstIter.isValid() && secondIter.isValid())
			{
				addToMap(*firstIter, *secondIter);
				++firstIter;
				++secondIter;
			}
		}
		{
			Declarations::SubrIterator firstIter = first.getFirstSubr();
			Declarations::SubrIterator secondIter = second.getFirstSubr();
			while (firstIter.isValid() && secondIter.isValid())
			{
				addToMap(*firstIter, *secondIter);
				BlockStatement& fBlock = firstIter->getBodyBlock();
				BlockStatement& sBlock = secondIter->getBodyBlock();
				doMapping(fBlock, sBlock);
				++firstIter;
				++secondIter;
			}
		}
	}
};


IfStatement* makeDynamicTransform(StatementBase& statement, std::vector<LampArrow*>& vArrows, IDepGraphEx& depGraphEx)
{
	if (!statement.hasParentBlock())
		throw OPS::Exception("Statement has no parent");
	
	std::map<RepriseBase*,RepriseBase*>	refMap;
	PredicatePtr	predicate;

	for(size_t i = 0; i < vArrows.size(); ++i)
	{
		if (predicate.get())
			predicate.reset( CreateOrPredicate(*predicate, *depGraphEx.getDepCond(vArrows[i])) );
		else
			predicate.reset( depGraphEx.getDepCond(vArrows[i]).release());
		refMap[vArrows[i]->pSrcOccur] = 0;
		refMap[vArrows[i]->pDepOccur] = 0;
	}
	
	OPS_ASSERT(predicate->GetExpr().release() != 0)
		IfStatement* res = new IfStatement(predicate->GetExpr().release());
	
	res->getThenBody().addLast(statement.clone());
	res->getElseBody().addLast(statement.clone());

	//OpsNewIr::mapReferences(statement, *res->getElseBody().getFirst(), refMap);
	RefMapper mapper(refMap);
	mapper.doMapping(statement, *res->getElseBody().getFirst());
	ProgramUnit* pProg = statement.findProgramUnit();
	if (pProg == 0)
	{
		throw OPS::RuntimeError("Can't find a ProgramUnit");
	}
	GlobalConditionTable* gct = GlobalConditionTable::Get(*pProg);

	for(size_t i = 0; i < vArrows.size(); ++i)
	{
		gct->Add(make_pair((ReferenceExpression*)refMap[vArrows[i]->pSrcOccur], (ReferenceExpression*)refMap[vArrows[i]->pDepOccur]), 
			GlobalConditionTable::TableRec(&res->getElseBody(), false));
	}


	StatementBase* parent = &statement.getParent()->cast_to<StatementBase>();
	if (dynamic_cast<WhileStatement*>(parent) || dynamic_cast<ForStatement*>(parent) ||
        dynamic_cast<IfStatement*>(parent))
	{
		// это одна из веток или тело цикла
		BlockStatement* stmtBlock = &statement.cast_to<BlockStatement>();
		//stmtBlock->clear();
		while (!stmtBlock->isEmpty())
        {
            BlockStatement::Iterator firstStatement = stmtBlock->getFirst();
            stmtBlock->erase(firstStatement);
        }
		stmtBlock->addLast(res);
	}
	else if (BlockStatement* parentBlock = dynamic_cast<BlockStatement*>(parent))
	{
		BlockStatement::Iterator it = parentBlock->convertToIterator(&statement);
		parentBlock->addAfter(it, res);
		parentBlock->erase(it);
	}
	else
	{
		throw OPS::RuntimeError("Unexpected statement type");
	}

	return res;
}

}
