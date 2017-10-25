#include "Shared/LoopShared.h"
#include "LoopSharedDeepWalkers.h"
#include "Shared/StatementsShared.h"

using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace std;

#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

namespace OPS
{
namespace Shared
{

/// подсчет охватывающих циклов for, принадлежащих заданному фрагменту программы
/// возвращает -1, если e не принадлежит code
int getEmbracedLoopsCount(const OPS::Reprise::RepriseBase* e, const OPS::Reprise::RepriseBase* code)
{
    if (code != 0) code = code->getParent(); //если code - for, то его тоже возвращаем
    const RepriseBase *p = e;
    const RepriseBase *lastP = p;
    int i = 0;
    while ((p != 0) && (p != code))
    {
        const ForStatement* fstmt = p->cast_ptr<ForStatement>();
        if (fstmt != 0)
        {
            if (&fstmt->getInitExpression() != lastP) //счтается что инициализирующее выражение не входит в цикл for
                i++;
        }
        lastP = p;
        p = p->getParent();
    }
	if (p == 0 && code != 0) return -1; //e не принадлежит code
    else return i;

}

vector<VariableDeclaration*> getIndexVariables(RepriseBase* StartNode, RepriseBase* code)
{
	std::list<ForStatement*> loops = getEmbracedLoopsNest(*StartNode, code);
	vector<VariableDeclaration*> res;

	std::list<ForStatement*>::const_iterator it = loops.begin();

	for(; it != loops.end(); ++it)
	{
		ForStatement* forStmt = *it;
		if (HirCCallExpression* stepExpr = forStmt->getStepExpression().cast_ptr<HirCCallExpression>())
		{
			switch (stepExpr->getKind())
			{
			case HirCCallExpression::HIRC_ASSIGN:
			case HirCCallExpression::HIRC_PLUS_ASSIGN:
			case HirCCallExpression::HIRC_MINUS_ASSIGN:
			case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS:
			case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS:
			case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS:
			case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS:
				{
					VariableDeclaration* vd = &(stepExpr->getArgument(0).cast_to<ReferenceExpression>().getReference());
					res.push_back(vd);
					break;
				}
			default:
				;// TODO ?
			}
		}
		if (BasicCallExpression* stepExpr = forStmt->getStepExpression().cast_ptr<BasicCallExpression>())
		{
			switch (stepExpr->getKind())
			{
			case BasicCallExpression::BCK_ASSIGN:
				{
					if (stepExpr->getArgument(0).is_a<ReferenceExpression>())
					{
						VariableDeclaration* vd = &(stepExpr->getArgument(0).cast_to<ReferenceExpression>().getReference());
						res.push_back(vd);
					}
					break;
				}
			default:
				;// TODO ?
			}
		}
	}

	return res;
}

//map<VariableDeclaration*,int>& getIndexVariables(StatementBase* Stmt)
//{
//	int IndexCount = 0;
//	RepriseBase* parent = Stmt->getParent();
//	map<VariableDeclaration*,int>* list = new map<VariableDeclaration*,int>();
//	while (! parent->is_a<SubroutineDeclaration>())
//	{
//		if (parent->is_a<ForStatement>())
//		{
//			ForStatement& forStMt = parent->cast_to<ForStatement>();
//			if (forStMt.getFinalExpression().is_a<HirCCallExpression>())
//			{
//				HirCCallExpression& finalExpr = forStMt.getStepExpression().cast_to<HirCCallExpression>();
//				switch (finalExpr.getKind())
//				{
//				case HirCCallExpression::HIRC_PLUS_ASSIGN:
//				case HirCCallExpression::HIRC_MINUS_ASSIGN:
//				case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS:
//				case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS:
//				case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS:
//				case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS:
//						(*list)[&(finalExpr.getArgument(0).cast_to<ReferenceExpression>().getReference())] = ++IndexCount;
//						break;
//				default:
//					;// TODO ?
//				}
//			}
//		
//		}
//		parent = parent->getParent();
//	}
//	return *list;
//}

int getCommonUpperLoopsAmount(StatementBase* Stmt1, StatementBase* Stmt2)
{
	map<ForStatement*,bool> hash4Stmt1;

	RepriseBase* parent = Stmt1->getParent();
	while (! parent->is_a<SubroutineDeclaration>())
	{
		if (parent->is_a<ForStatement>())
		{
			ForStatement& forStmt = parent->cast_to<ForStatement>();
			hash4Stmt1[&forStmt]=true;
		}
		parent = parent->getParent();
	}

	int count = 0;
	parent = Stmt2->getParent();
	while (! parent->is_a<SubroutineDeclaration>())
	{
		if (parent->is_a<ForStatement>())
		{
			if (hash4Stmt1[&parent->cast_to<ForStatement>()])
				count++;
		}
		parent = parent->getParent();
	}

	return count; // не нашли общих циклов
}

int getCommonUpperLoopsAmount(OPS::Reprise::ExpressionBase* Occurance1, OPS::Reprise::ExpressionBase* Occurance2)
{
	StatementBase* Stmt1 = 0, *Stmt2 = 0;
	RepriseBase* parent = Occurance1->getParent();
	while (! parent->is_a<StatementBase>())
		parent = parent->getParent();
	if (parent->is_a<StatementBase>())
		Stmt1 = &parent->cast_to<StatementBase>();

	parent = Occurance2->getParent();
	while (! parent->is_a<StatementBase>())
		parent = parent->getParent();
	if (parent->is_a<StatementBase>())
		Stmt2 = &parent->cast_to<StatementBase>();
	//else // TODO : выход
	//	throw new Exception

	return getCommonUpperLoopsAmount(Stmt1, Stmt2);
}

bool isPerfectLoopNest(ForStatement* Loop)
{
	const ForStatement *pInnerFor = 0, *pFor = Loop;
	bool esc = false;

	// Найдем самый глубоковложенный цикл for
	while (!esc)
	{
		const BlockStatement& LoopBody = pFor->getBody();
		int forLoopsCounter = 0; // считает циклы for на текущем уровне гнезда циклов
		BlockStatement::ConstIterator itStmtIter = LoopBody.getFirst();
		while(itStmtIter.isValid() == 1)
		{
			if ((*itStmtIter).is_a<ForStatement>())
			{
				if (forLoopsCounter == 1)
					// Внутри цикла for найдено более 1 цикла for
					return false;

				pInnerFor = (*itStmtIter).cast_ptr<ForStatement>();
				forLoopsCounter++;
			}
			else
			{
				if ((!(*itStmtIter).is_a<EmptyStatement>()) && (forLoopsCounter == 1))
					// Внутри цикла for найден другой цикл for и непустой оператор
					return false;

				if ((!(*itStmtIter).is_a<EmptyStatement>()) && (forLoopsCounter == 0))
				{
					// В текущем цикле for есть операторы отличные от пустого. Предполагаем, что он является самым глубоковложенным for
					break;
				}
			}
			itStmtIter++;
		}

		if (forLoopsCounter == 0)
			esc = true; // найден кандидат на самый вложенный цикл
		else
			pFor = pInnerFor;
	}

	// Необходимо проверить нет ли циклов внутри предполагаемого самого глубоковложенного pFor 
	if (isPresentInBlock<ForStatement>(pFor->getBody()))
		return false; // внутри найден еще один цикл for, а перед этим был найден не пустой оператор

	return true;
}

std::vector<ForStatement*> getLoopsOfPerfectNest(ForStatement &upperLoop)
{
	RepriseBase* stmt = &upperLoop;
	vector<ForStatement*> loopNest;
	//находим все тесно вложенные друг в друга циклы
	while (true)
	{
		ForStatement* loop = stmt->cast_ptr<ForStatement>();
		if (loop)
			loopNest.push_back(loop);
		else
			break;

		BlockStatement* b = &loop->getBody();
		while (b->getChildCount() == 1 && b->getChild(0).is_a<BlockStatement>())
			b = b->getChild(0).cast_ptr<BlockStatement>();

		if (b->getChildCount() != 1)
			break;

		stmt = &b->getChild(0);
	}
	return loopNest;
}

bool isAllLoopsInNestAreCanonised(OPS::Reprise::ForStatement* pTopLoopInNest)
{
	OPS_ASSERT(pTopLoopInNest != NULL);

	AllLoopsInNestAreCanonisedDeepWalker deepWalker;
	deepWalker.visit(*pTopLoopInNest);

	return deepWalker.isAllLoopsInNestAreCanonised();
}

std::list<ForStatement*> getEmbracedLoopsNest(RepriseBase& smth, OPS::Reprise::RepriseBase* code)
{
	std::list<ForStatement*> result;
    if (code != 0) code = code->getParent();
	RepriseBase* parent = &smth;
    RepriseBase* lastParent = &smth;
	while ( (parent != NULL) && (parent != code) )
	{
        ForStatement* forStmt = parent->cast_ptr<ForStatement>();
		if (forStmt != 0)
		{
            if (lastParent != &forStmt->getInitExpression()) //считаем, что InitExpr находится вне for
			    result.push_back(forStmt);
		}
        lastParent = parent;
        parent = parent->getParent();
	}

	result.reverse();

	return result;
}

std::list<const OPS::Reprise::ForStatement*> getEmbracedLoopsNest(const OPS::Reprise::RepriseBase& stmt, const OPS::Reprise::RepriseBase* code)
{
    std::list<OPS::Reprise::ForStatement*> nonConstRes = getEmbracedLoopsNest(const_cast<RepriseBase&>(stmt), const_cast<RepriseBase*>(code));
    std::list<const OPS::Reprise::ForStatement*> res(nonConstRes.begin(), nonConstRes.end());
    return res;
}

int getEmbracedLoopsIndex(OPS::Reprise::RepriseBase &stmt, OPS::Reprise::ForStatement &loop, OPS::Reprise::RepriseBase *code)
{
	std::list<ForStatement*> nest = getEmbracedLoopsNest(stmt, code);

	std::list<ForStatement*>::const_iterator it = nest.begin();
	for(int i = 0; it != nest.end(); ++it, ++i)
	{
		if (*it == &loop)
			return i;
	}
	return -1;
}

int getCommonUpperLoopsAmount(RepriseBase* e1, RepriseBase* e2, RepriseBase* code)
{
    std::list<ForStatement*> loopNest1 = getEmbracedLoopsNest(*e1, code);
    std::list<ForStatement*> loopNest2 = getEmbracedLoopsNest(*e2, code);
    std::list<ForStatement*>::iterator it1, it2;
    it1 = loopNest1.begin();
    it2 = loopNest2.begin();
    int count = 0;
    while ( (it1 != loopNest1.end()) && (it2 != loopNest2.end()) && (*it1 == *it2) )
    {
        ++count;
        ++it1;
        ++it2;
    }
    return count;
}

} // end namespace Shared
} // end namespace OPS
