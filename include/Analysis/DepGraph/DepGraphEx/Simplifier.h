#ifndef _SIMPLIFIER_H_
#define _SIMPLIFIER_H_

#include "Reprise/Reprise.h"
#include <list>
#include <utility>
#include "Analysis/DepGraph/DepGraph.h"
namespace DepGraph
{
using namespace OPS::Reprise;
// Функция-упроститель 
typedef ExpressionBase*(*PSimpFuncGeneric)(const ExpressionBase*);
typedef ExpressionBase*(*PSimpFuncBinary)(const BasicCallExpression*);
//typedef ExpressionBase*(*PSimpFuncUnary)(const BasicCallExpression*);
typedef ExpressionBase*(*PSimpFuncData)(const LiteralExpression*);

class SimplifiersList
{
public:
	typedef std::list<std::pair<PSimpFuncGeneric, GetTypeVisitor::NodeKind> > Cont;
	typedef Cont::const_iterator const_iterator;

protected:
	Cont m_cCont;

public:
	void push_back(const PSimpFuncBinary pSimp);
	//void push_back(const PSimpFuncUnary pSimp);
	void push_back(const PSimpFuncData pSimp);

	const_iterator begin() const;
	const_iterator end() const;
};

extern SimplifiersList	g_lStdSimplifiers;
extern BasicLiteralExpression IMM_TRUE;
extern BasicLiteralExpression IMM_FALSE;

ExpressionBase* AndSimp(const BasicCallExpression* pExpr);
ExpressionBase* UnMinBinPlusSimp(const BasicCallExpression* pExpr);
ExpressionBase* VarMinVarSimp(const BasicCallExpression* pExpr);
ExpressionBase* ImmOperImmSimp(const BasicCallExpression* pExpr);
//ExpressionBase* ImmOperImmSimp(const BasicCallExpression* pExpr);
ExpressionBase* IdUsingSimp(const BasicCallExpression* pExpr);

ExpressionBase* simplify(ExpressionBase* pExpr, const SimplifiersList* pSimplifiers = &g_lStdSimplifiers);

}

#endif	//_SIMPLIFIER_H_
