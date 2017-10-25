#include "Analysis/Montego/DependenceGraph/SymbolicAnalysis.h"
#include "Analysis/Montego/DependenceGraph/IndexAnalysis.h"
#include "Analysis/Montego/OccurrenceCoeffs.h"
#include "Analysis/LatticeGraph/GaussianElimination.h"
#include "Analysis/SymbolPredicateFinder.h"
#include "Shared/LoopShared.h"
#include "Reprise/Service/DeepWalker.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{
namespace SymbolicAnalysis
{

typedef std::vector<int> MatrixRow;
typedef std::vector<MatrixRow> Matrix;

PredicateBuilder::PredicateBuilder(const Analysis::AbstractDependence &arc)
	:m_arc(&arc)
	,m_start(0)
	,m_end(0)
	,m_startCondition(0)
	,m_endCondition(0)
{
	m_start = arc.getBeginOccurrence();
	m_end = arc.getEndOccurrence();
	m_commonLoops = OPS::Shared::getCommonUpperLoopsAmount(m_start->getExpr(), m_end->getExpr());

	if (isApplicable())
	{
		SymbolPredicate* startSymPred = OPS::getPathPredicates(*m_start->getStatement(),
														   m_start->getStatement()->getRootBlock());
		SymbolPredicate* endSymPred = OPS::getPathPredicates(*m_end->getStatement(),
															 m_end->getStatement()->getRootBlock());
		m_startCondition = startSymPred->outToExpression();
		m_endCondition = endSymPred->outToExpression();

		delete startSymPred;
		delete endSymPred;
	}
}

PredicateBuilder::~PredicateBuilder()
{
	delete m_startCondition;
	delete m_endCondition;
}

bool PredicateBuilder::isApplicable() const
{
	if (m_start == 0 || m_end == 0)
		return false;

	if (m_start->getBracketCount() == 0)
		return false;

	if (m_start->getBracketCount() != m_end->getBracketCount()/* ||
		m_start->getName() != m_end->getName()*/)
		return false;

/*	if (m_start->getStatement() == m_end->getStatement())
		return false;*/

	if (OPS::Shared::getEmbracedLoopsCount(m_start->getExpr()) != m_commonLoops)
		return false;

	if (m_arc->getType() == Analysis::AbstractDependence::Input)
		return false;

	return true;
}

std::vector<PredicatePtr> PredicateBuilder::buildPredicates()
{
	if ((m_startCondition == 0 && m_endCondition == 0) || !isApplicable())
		return std::vector<PredicatePtr>();

	const int commonLoopNumber = OPS::Shared::getCommonUpperLoopsAmount(m_start->getExpr(),
																		m_end->getExpr());

	std::vector<PredicatePtr> result;

	for(int supp = 0; supp < commonLoopNumber; ++supp)
	{
		result.push_back(buildPredicate(supp));
	}

	return result;
}

static ExpressionBase* getSubtitutionExpression(const MatrixRow& row, int loops, std::vector<VariableDeclaration*>& vars)
{
	ExpressionBase* result = 0;

	if (row[0] != 0)
		result = StrictLiteralExpression::createInt32(row[0]);

	for(size_t i = 0; i < vars.size() - loops; ++i)
	{
		const int coef = -row[1 + loops + i];
		if (coef != 0)
		{
			ExpressionBase* e = new ReferenceExpression(*vars[loops+i]);
			if (coef != 1)
				e = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY,
											StrictLiteralExpression::createInt32(coef),
											e);
			if (result)
				result = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS,
												 result,
												 e);
			else
				result = e;
		}
	}

	if (result == 0)
		result = StrictLiteralExpression::createInt32(0);

	return result;
}

class VariableSubstitutionWalker : public OPS::Reprise::Service::DeepWalker
{
public:

	void addSubst(VariableDeclaration* varDecl, ExpressionBase* expr)
	{
		m_subsMap[varDecl] = expr;
	}

	void visit(ReferenceExpression& refExpr)
	{
		if (m_subsMap.find(&refExpr.getReference()) != m_subsMap.end())
		{
			ReprisePtr<ExpressionBase> dstExpr(m_subsMap[&refExpr.getReference()]->clone());
			OPS::Reprise::Editing::replaceExpression(refExpr, dstExpr);
		}
	}

private:
	std::map<VariableDeclaration*, ExpressionBase*> m_subsMap;
};

ReferenceExpression* findRefExpr(ExpressionBase* expr)
{
	ReferenceExpression* result = expr->cast_ptr<ReferenceExpression>();

	if (result)
		return result;

	if (BasicCallExpression* callExpr = expr->cast_ptr<BasicCallExpression>())
	{
		if (callExpr->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
		{
			return callExpr->getArgument(0).cast_ptr<ReferenceExpression>();
		}
	}

	return 0;
}

PredicatePtr PredicateBuilder::buildPredicate(int support)
{
	PredicatePtr startPred, endPred;
	if (m_startCondition != 0)
		startPred = buildPredicate(support, m_start, m_end, m_startCondition);
	if (m_endCondition != 0)
		endPred = buildPredicate(support, m_end, m_start, m_endCondition);

	if (startPred.get() != 0 && endPred.get() != 0)
	{
		return PredicatePtr(new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_OR, startPred.release(), endPred.release()));
	}
	else if(startPred.get() != 0)
	{
		return startPred;
	}
	else if (endPred.get() != 0)
	{
		return endPred;
	}
	else
		return PredicatePtr();
}

PredicatePtr PredicateBuilder::buildPredicate(int support, Analysis::AbstractOccurrence *start, Analysis::AbstractOccurrence *end, Reprise::ExpressionBase *condition)
{
	BlockStatement& fragment = start->getStatement()->getRootBlock();

	std::pair<std::vector<VariableDeclaration*>, std::vector<VariableDeclaration*> > allParamsInOrder =
			getAllParamsInOrder(start->getExpr(),	end->getExpr(), &fragment);

	//коэффициенты индексных выражений
    std::vector<OPS::LatticeGraph::SimpleLinearExpression> coeffs1, coeffs2;
	OPS::Montego::OccurrenceCoeffs coeffsStart(BasicOccurrence(*findRefExpr(start->getExpr()))),
									coeffsEnd(BasicOccurrence(*findRefExpr(end->getExpr())));

	coeffsStart.getAllVarsCoeffsInOrder(coeffs1, allParamsInOrder.first);
	coeffsEnd.getAllVarsCoeffsInOrder(coeffs2, allParamsInOrder.second);

	Matrix equationSystem(start->getBracketCount() + support);

	for (int i = 0; i < start->getBracketCount(); ++i)
	{
        OPS::LatticeGraph::SimpleLinearExpression& coeffs = coeffs1[i];
		coeffs.substract(coeffs2[i]);
		equationSystem[i].assign(coeffs.m_coefs, coeffs.m_coefs + coeffs.m_dim);
	}

	//добавляем support равенств индексов
	for (int i = 0; i < support; i++)
	{
		MatrixRow& row = equationSystem[start->getBracketCount() + i];
		row.resize(allParamsInOrder.first.size() + 1);
		row[1 + i] = 1;
		row[1 + m_commonLoops + i] = -1;
	}

	LatticeGraph::GaussianEliminator eliminator(equationSystem);
	if (eliminator.eliminate() != LatticeGraph::GaussianEliminator::Incompatible)
	{
		if (eliminator.getIntegerSolution(equationSystem))
		{
			// нужно выразить все переменные
			if ((int)equationSystem.size() >= m_commonLoops)
			{
				for(int i = 0; i < m_commonLoops; ++i)
				{
					if (equationSystem[i][i+1] != 1)
						return PredicatePtr();
				}

				VariableSubstitutionWalker vsw;
				for(int i = 0; i < m_commonLoops; ++i)
				{
					vsw.addSubst(allParamsInOrder.first[i], getSubtitutionExpression(equationSystem[i], m_commonLoops, allParamsInOrder.second));
				}

				PredicatePtr result(new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_NOT, condition->clone()));
				result->accept(vsw);

				return result;
			}
		}
	}

	return PredicatePtr();
}

const char* const SymbolicPredicatesMeta::UNIQUE_ID = "SymbolicPredicates";

SymbolicPredicatesMeta::PredicateValueList& SymbolicPredicatesMeta::getPredicates()
{
	return m_predicates;
}

void SymbolicPredicatesMeta::refineDependence(Analysis::AbstractDependence &dep)
{
	if (m_predicates.empty())
		return;

	PredicateBuilder builder(dep);
	if (builder.isApplicable())
	{
		std::list<ForStatement*> loops = OPS::Shared::getEmbracedLoopsNest(*dep.getBeginStatement());

		std::list<ForStatement*>::const_iterator itLoop = loops.begin();
		for(int i = 0; itLoop != loops.end(); ++itLoop, ++i)
		{
			if (dep.testSupp(**itLoop))
			{
				PredicatePtr pred = builder.buildPredicate(i);
				if (pred.get() != 0)
				{
					for(PredicateValueList::const_iterator it = m_predicates.begin(); it != m_predicates.end(); ++it)
					{
						if (it->first->isEqual(*pred) && it->second == true)
						{
							dep.eraseSupp(**itLoop);
						}
					}
				}
			}
		}
	}
}

const char* SymbolicPredicatesMeta::getUniqueId() const { return UNIQUE_ID; }

}
}
}
