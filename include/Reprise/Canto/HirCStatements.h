#ifndef OPS_IR_REPRISE_CANTO_HIRCSTATEMENTS_H_INCLUDED__
#define OPS_IR_REPRISE_CANTO_HIRCSTATEMENTS_H_INCLUDED__

#include "Reprise/Statements.h"
#include "Reprise/Expressions.h"

namespace OPS
{
namespace Reprise
{
class VariableDeclaration;

namespace Canto
{


///	break statement
class HirBreakStatement : public StatementBase
{
public:
    HirBreakStatement(void);

    virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
    virtual int getChildCount(void) const;
    virtual RepriseBase& getChild(int index);

    virtual std::string dumpState(void) const;

    OPS_DEFINE_VISITABLE()
    OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
    OPS_DEFINE_CLONABLE_INTERFACE(HirBreakStatement)
};

///	continue statement
class HirContinueStatement : public StatementBase
{
public:
    HirContinueStatement(void);

//		RepriseBase implementation
    virtual int getChildCount(void) const;
    virtual RepriseBase& getChild(int index);

    virtual std::string dumpState(void) const;

    virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

    OPS_DEFINE_VISITABLE()
    OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
    OPS_DEFINE_CLONABLE_INTERFACE(HirContinueStatement)
};

class HirCVariableInitStatement : public OPS::Reprise::StatementBase
{
public:
    explicit HirCVariableInitStatement(VariableDeclaration* const variable,
                                        ExpressionBase* const initExpression);

    VariableDeclaration& getVariableDeclaration();

    ExpressionBase& getInitExpression();

    bool isConnectedToForStmt() const;
    ForStatement& getConnectedForStmt();
    void connectToForStmt(ForStatement& forStmt);

    virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
    virtual int getChildCount(void) const;
    virtual RepriseBase& getChild(int index);

    virtual std::string dumpState(void) const;

    OPS_DEFINE_VISITABLE()
    OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
    OPS_DEFINE_CLONABLE_INTERFACE(HirCVariableInitStatement)

protected:
    HirCVariableInitStatement(const HirCVariableInitStatement& other);

private:
    RepriseWeakPtr<VariableDeclaration> m_variable;
    ReprisePtr<ExpressionBase> m_initExpression;
    RepriseWeakPtr<ForStatement> m_forStmt;
};

}
}
}

#endif // OPS_IR_REPRISE_CANTO_HIRCSTATEMENTS_H_INCLUDED__
