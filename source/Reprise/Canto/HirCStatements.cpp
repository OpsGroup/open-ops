#include "Reprise/Canto/HirCStatements.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{

//	BreakStatement class implementation
HirBreakStatement::HirBreakStatement(void)
{
}

void HirBreakStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
    OPS_UNUSED(destination)
    throw RepriseError(Strings::format("Unexpected replace expression (%p) in BreakStatement.", &source));
}

//		BreakStatement - RepriseBase implementation
int HirBreakStatement::getChildCount(void) const
{
    return 0;
}

RepriseBase& HirBreakStatement::getChild(const int index)
{
    OPS_UNUSED(index)
    throw UnexpectedChildError("BreakStatement::getChild");
}

std::string HirBreakStatement::dumpState(void) const
{
    std::string state = StatementBase::dumpState();
    state += "break;\n";
    return state;
}


//	ContinueStatement class implementation
HirContinueStatement::HirContinueStatement(void)
{
}

void HirContinueStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
    OPS_UNUSED(destination)
    throw RepriseError(Strings::format("Unexpected replace expression (%p) in ContinueStatement.", &source));
}

//		ContinueStatement - RepriseBase implementation
int HirContinueStatement::getChildCount(void) const
{
    return 0;
}

RepriseBase& HirContinueStatement::getChild(const int index)
{
    OPS_UNUSED(index)
    throw UnexpectedChildError("ContinueStatement::getChild");
}

std::string HirContinueStatement::dumpState(void) const
{
    std::string state = StatementBase::dumpState();
    state += "continue;\n";
    return state;
}

HirCVariableInitStatement::HirCVariableInitStatement(VariableDeclaration *const variable, ExpressionBase *const initExpression)
    :m_variable(variable)
    ,m_initExpression(initExpression)
{
    m_initExpression->setParent(this);
}

VariableDeclaration& HirCVariableInitStatement::getVariableDeclaration()
{
    return *m_variable;
}

ExpressionBase& HirCVariableInitStatement::getInitExpression()
{
    return *m_initExpression;
}

void HirCVariableInitStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
    if (&source == m_initExpression.get())
    {
        m_initExpression = destination;
        m_initExpression->setParent(this);
    }
    else
        throw RepriseError(Strings::format("Unexpected replace expression (%p) in HirCVariableInitStatement.", &source));
}

int HirCVariableInitStatement::getChildCount() const
{
    return 1;
}

RepriseBase& HirCVariableInitStatement::getChild(int index)
{
    switch(index)
    {
    case 0: return *m_initExpression;
    OPS_DEFAULT_CASE_LABEL
    }
    throw UnexpectedChildError("ForStatement::getChild");
}

std::string HirCVariableInitStatement::dumpState(void) const
{
    std::string state = StatementBase::dumpState();
    state += Strings::format("%s = %s",
             m_variable->dumpState().c_str(), m_initExpression->dumpState().c_str());
    return state;
}

HirCVariableInitStatement::HirCVariableInitStatement(const HirCVariableInitStatement &other)
    :m_variable(other.m_variable)
    ,m_initExpression(other.m_initExpression->clone())
    ,m_forStmt(other.m_forStmt)
{
    m_initExpression->setParent(this);
}

bool HirCVariableInitStatement::isConnectedToForStmt() const
{
    return m_forStmt.get() != 0;
}

ForStatement& HirCVariableInitStatement::getConnectedForStmt()
{
    return *m_forStmt;
}

void HirCVariableInitStatement::connectToForStmt(ForStatement &forStmt)
{
    m_forStmt.reset(&forStmt);
}

}
}
}
