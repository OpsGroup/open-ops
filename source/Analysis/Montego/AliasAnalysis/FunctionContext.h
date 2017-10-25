#pragma once
/*
место, где была вызвана данная функция и какая функция была вызвана
*/

#include <string>
#include <list>
#include "Reprise/Declarations.h"
#include "Reprise/Expressions.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"

namespace OPS
{
namespace Montego
{

class FunctionContext
{
public:

	static const FunctionContext* createRootContext(OPS::Reprise::SubroutineDeclaration& mainDecl);
	static void freeRootContext(const FunctionContext* context);

	const FunctionContext* getChildContext(OPS::Reprise::SubroutineCallExpression* call,
										   OPS::Reprise::SubroutineDeclaration& subDecl) const;
    
    std::string toString() const;
	size_t getDepth() const;
    int recursion(RecursionType rType) const;

	bool isRoot() const { return m_parent == 0; }

	OPS::Reprise::SubroutineDeclaration& getLastSubroutine() const;
	OPS::Reprise::SubroutineCallExpression& getLastCall() const;

private:
	FunctionContext(const FunctionContext* parent,
					OPS::Reprise::SubroutineCallExpression* call,
					OPS::Reprise::SubroutineDeclaration& subDecl);
    ~FunctionContext();

	const FunctionContext* m_parent;
	OPS::Reprise::SubroutineCallExpression* m_call;
	OPS::Reprise::SubroutineDeclaration* m_decl;

	mutable std::vector<FunctionContext*> m_childs;
    mutable size_t m_depth;
    mutable int m_recursion;
};

}//end of namespace
}//end of namespace
