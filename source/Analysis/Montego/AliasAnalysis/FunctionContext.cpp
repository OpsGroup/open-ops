#include "FunctionContext.h"

namespace OPS
{
namespace Montego
{
FunctionContext::FunctionContext(const FunctionContext* parent,
					OPS::Reprise::SubroutineCallExpression* call,
					OPS::Reprise::SubroutineDeclaration& subDecl)
	:m_parent(parent)
	,m_call(call)
	,m_decl(&subDecl)
    ,m_depth(0)
    ,m_recursion(-1)
{
}

FunctionContext::~FunctionContext()
{
	for(size_t i = 0; i < m_childs.size(); ++i)
		delete m_childs.at(i);
}

std::string FunctionContext::toString() const
{
	std::string res;

	if (m_parent == 0)
		res	= "Context: ";
	else
		res = m_parent->toString() + "->";

	res += m_decl->getName();
    return res;
}

int FunctionContext::recursion(RecursionType rType) const
{
    if (m_recursion == -1)
    {
        m_recursion = 0;
        const FunctionContext* con = m_parent;

        while(con)
        {
            if (rType == RecursionByCall &&
                con->m_call == m_call) ++m_recursion;
            if (rType == RecursionByDecl &&
                con->m_decl == m_decl) ++m_recursion;
            con = con->m_parent;
        }
    }
    return m_recursion;
}

size_t FunctionContext::getDepth() const
{
    if (m_depth == 0)
    {
        m_depth = 1;
        const FunctionContext* con = m_parent;
        while(con)
        {
            con = con->m_parent;
            m_depth++;
        }
    }
    return m_depth;
}

OPS::Reprise::SubroutineDeclaration& FunctionContext::getLastSubroutine() const
{
	return *m_decl;
}

OPS::Reprise::SubroutineCallExpression& FunctionContext::getLastCall() const
{
	return *m_call;
}

const FunctionContext* FunctionContext::createRootContext(OPS::Reprise::SubroutineDeclaration& mainDecl)
{
	return new FunctionContext(0, 0, mainDecl);
}

void FunctionContext::freeRootContext(const FunctionContext *context)
{
	delete const_cast<FunctionContext*>(context);
}

const FunctionContext* FunctionContext::getChildContext(OPS::Reprise::SubroutineCallExpression* call,
										   OPS::Reprise::SubroutineDeclaration& subDecl) const
{
	for(size_t i = 0; i < m_childs.size(); ++i)
	{
		if (m_childs[i]->m_call == call &&
			m_childs[i]->m_decl == &subDecl)
			return m_childs[i];
	}

	FunctionContext* child = new FunctionContext(this, call, subDecl);
	m_childs.push_back(child);
	return child;
}

}//end of namespace
}//end of namespace
