#include "Analysis/LoopsInterchange/LoopsInterchangeAnalysis.h"
#include "Analysis/LatticeGraph/LatticeGraph.h"
#include "Analysis/LatticeGraph/ExtendedQuast.h"
#include "Analysis/LatticeGraph/ParamPoint.h"
#include "Analysis/LatticeGraph/ComplexCondition.h"

using namespace OPS::Reprise;
using namespace OPS::Montego;
using namespace OPS::LatticeGraph;

namespace OPS
{
namespace Analysis
{
namespace LoopsInterchange
{


void trasformExtendedQuastForLoopInterchangeCheckHelper(ExtendedQuast* eq, DependenceGraphAbstractArc::DependenceType depType)
{
    if (eq->getTypeOfNod() == ExtendedQuast::INNER)
    {
        trasformExtendedQuastForLoopInterchangeCheckHelper(eq->getTrueBranch(), depType);
        trasformExtendedQuastForLoopInterchangeCheckHelper(eq->getFalseBranch(), depType);
    }
    if (eq->getTypeOfNod() == ExtendedQuast::LEAF)
    {
        int inDim = eq->getInDim();
        int outDim = eq->getOutDim();
        OPS_ASSERT(outDim >= 2);
        ParamPoint* linExpr = eq->getLinExpr();
        //формируем структуру для замены листа
        ExtendedQuast* NULL1 = new ExtendedQuast(inDim, outDim, ExtendedQuast::EMPTY);
        ExtendedQuast* NULL2 = NULL1->clone();

        ExtendedQuast* return1 = new ExtendedQuast(inDim, outDim, ExtendedQuast::LEAF);
        ParamPoint* p = new ParamPoint(outDim);
        for (int i = 0; i < outDim; ++i)  (*p)[i] = new SimpleLinearExpression(inDim+1);
        return1->setLinExpr(p);
        ExtendedQuast* return2 = return1->clone();

        SimpleLinearExpression* ineq1 = new SimpleLinearExpression();
        *ineq1 = *((*linExpr)[1]);
        (*ineq1)[2] -= 1; //0-й - это свободный член
        SimpleLinearExpression* ineq2 = new SimpleLinearExpression();
        *ineq2 = *ineq1;
        (*ineq1)[0] += 1;
        ineq1->multiply(-1);
        (*ineq2)[0] -= 1;
        if (depType != DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)
        {
            //меняем местами ineq1 и ineq2
            SimpleLinearExpression* rab = ineq1;
            ineq1 = ineq2;
            ineq2 = rab;
        }

        SimpleLinearExpression* ineq3 = new SimpleLinearExpression();
        *ineq3 = *((*linExpr)[0]);
        (*ineq3)[1] -= 1;
        if (depType == DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)
        {
            (*ineq3)[0] += 1;
            ineq3->multiply(-1);
        }
        else
        {
            (*ineq3)[0] -= 1;
        }

        ComplexCondition* cond1 = new ComplexCondition((Inequality*)ineq1);
        ComplexCondition* cond2 = new ComplexCondition((Inequality*)ineq2);
        ComplexCondition* cond3 = new ComplexCondition((Inequality*)ineq3);

        ExtendedQuast* inner3 = new ExtendedQuast(inDim, outDim, cond3, return2, NULL2, NULL);
        ExtendedQuast* inner2 = new ExtendedQuast(inDim, outDim, cond2, NULL1, inner3, NULL);
        ExtendedQuast* inner1 = new ExtendedQuast(inDim, outDim, cond1, return1, inner2, NULL);

        eq->insertIntoTreeHere(inner1);
        delete inner1;
    }
}

ExtendedQuast* trasformExtendedQuastForLoopInterchangeCheck(ExtendedQuast& eq, DependenceGraphAbstractArc::DependenceType depType)
{
    ExtendedQuast* res;
    res = eq.clone();
    trasformExtendedQuastForLoopInterchangeCheckHelper(res, depType);
    return res;
}

//на вход подается внешний цикл тесного гнезда
bool isInterchangable(ForStatement& forStatement)
{
    if (forStatement.getBody().getFirst()->cast_ptr<ForStatement>() == 0) return false;
    bool flagCanApply = true;
    DependenceGraph depgraph(forStatement);
	depgraph.removeCounterArcs();
    DependenceGraph::ArcList arclist = depgraph.getAllArcs();
    DependenceGraph::ArcList::iterator it = arclist.begin();
    for ( ; it != arclist.end() && flagCanApply; ++it)
    {
        std::tr1::shared_ptr<DependenceGraphAbstractArc> arc = *it;
        if ((arc->getDependenceType() != DependenceGraphAbstractArc::DT_ANTIDEPENDENCE) &&
            (arc->getDependenceType() != DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE) &&
            (arc->getDependenceType() != DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE))
            continue;
        OccurrencePtr oo1 = arc->getStartVertex().getSourceOccurrence();
        OccurrencePtr oo2 = arc->getEndVertex().getSourceOccurrence();
        BasicOccurrence* o1 = oo1->cast_ptr<BasicOccurrence>();
        BasicOccurrence* o2 = oo2->cast_ptr<BasicOccurrence>();
        if ((o1 == 0) || (o2 == 0)) {flagCanApply = false; break;}
        std::tr1::shared_ptr<AliasInterface> ai = depgraph.getAliasInterface();
        StatementBase* program = const_cast<StatementBase*>(&depgraph.getSourceStatement());
        ElemLatticeGraph elg(*program, *ai, *o1, *o2, arc->getDependenceType(), true);
        if (elg.isStatus(LG_EMPTY))  continue;
        if (elg.isStatus(LG_ERROR_INIT))   {flagCanApply = false; break;}
        ExtendedQuast* elgq = elg.getFeautrierSolution();
        ExtendedQuast* transformedEQ = trasformExtendedQuastForLoopInterchangeCheck(*elgq, arc->getDependenceType());
        transformedEQ->removeExclusiveInequalities(GenArea());
        transformedEQ->simplifyEmptyLeaves();
        if (transformedEQ->getTypeOfNod() != ExtendedQuast::EMPTY)
        {
            flagCanApply = false;
        }
        delete transformedEQ;
    }

    return flagCanApply;
}

}	// LoopsInterchange
}	// Analysis
}	// OPS




