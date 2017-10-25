#include "Analysis/Statistics.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/ServiceFunctions.h"
#include "Reprise/Canto.h"
#include "Shared/LinearExpressions.h"
#include "Reprise/Service/Traversal.h"
#include "Shared/LoopShared.h"
#include "Shared/StatementsShared.h"
#include <iostream>

using namespace OPS::Analysis::Statistics;
using namespace OPS::Reprise;

typedef std::list<StatisticsAnalyserBase*> AnalysersList;

class StatisticsWalker
{
public:
    StatisticsWalker(const std::list<StatisticsAnalyserBase*>& analysers);
    void printStatistics(std::ostream& os) const;
    void operator()(RepriseBase& node);
private:
    const std::list<StatisticsAnalyserBase*>& m_analysers;
};

class NodeCounterAnalyser : public StatisticsAnalyserBase
                          , public OPS::Visitor<SubroutineDeclaration>
                          , public OPS::Visitor<VariableDeclaration>
                          , public OPS::Visitor<TypeDeclaration>
                          , public OPS::Visitor<ForStatement>
                          , public OPS::Visitor<WhileStatement>
                          , public OPS::Visitor<IfStatement>
                          , public OPS::Visitor<PlainSwitchStatement>
                          , public OPS::Visitor<GotoStatement>
                          , public OPS::Visitor<ReturnStatement>
                          , public OPS::Visitor<ExpressionStatement>
                          , public OPS::Visitor<EmptyStatement>
                          , public OPS::Visitor<BasicLiteralExpression>
                          , public OPS::Visitor<StrictLiteralExpression>
                          , public OPS::Visitor<CompoundLiteralExpression>
                          , public OPS::Visitor<ReferenceExpression>
                          , public OPS::Visitor<SubroutineReferenceExpression>
                          , public OPS::Visitor<StructAccessExpression>
                          , public OPS::Visitor<EnumAccessExpression>
                          , public OPS::Visitor<TypeCastExpression>
                          , public OPS::Visitor<BasicCallExpression>
                          , public OPS::Visitor<SubroutineCallExpression>
                          , public OPS::Visitor<EmptyExpression>
                          , public OPS::Visitor<Canto::HirBreakStatement>
                          , public OPS::Visitor<Canto::HirContinueStatement>
{
public:
    NodeCounterAnalyser()
        :m_subroutineDecls(0)
        ,m_subroutinesWithBody(0)
        ,m_uniqueSubroutines(0)
        ,m_variableDecls(0)
        ,m_globalVariables(0)
        ,m_typeDecls(0)
        ,m_forLoops(0)
        ,m_whileLoops(0)
        ,m_ifStmts(0)
        ,m_switchStmts(0)
        ,m_gotoStmts(0)
        ,m_returnStmts(0)
        ,m_expressionStmts(0)
        ,m_emptyStmts(0)
        ,m_breakStmts(0)
        ,m_continueStmts(0)
        ,m_basicLiterals(0)
        ,m_strictLiterals(0)
        ,m_compoundLiterals(0)
        ,m_refExprs(0)
        ,m_subroutineRefExprs(0)
        ,m_structAccessExprs(0)
        ,m_enumAccessExprs(0)
        ,m_typeCastExprs(0)
        ,m_basicCallExprs(0)
        ,m_subroutineCallExprs(0)
        ,m_emptyExprs(0)
    {}
    void visit(SubroutineDeclaration& sub)
    {
        m_subroutineDecls++;
        if (sub.hasImplementation()) m_subroutinesWithBody++;
        if (sub.hasDefinition() && &sub.getDefinition() == &sub) m_uniqueSubroutines++;
    }
    void visit(VariableDeclaration& var)
    {
        m_variableDecls++;
        if (!var.hasDefinedBlock() &&
             var.hasDefinition() &&
             &var.getDefinition() == &var) m_globalVariables++;
    }
    void visit(TypeDeclaration&) { m_typeDecls++; }
    void visit(ForStatement&) { m_forLoops++; }
    void visit(WhileStatement&) { m_whileLoops++; }
    void visit(IfStatement&) { m_ifStmts++; }
    void visit(PlainSwitchStatement&) { m_switchStmts++; }
    void visit(GotoStatement&) { m_gotoStmts++; }
    void visit(ReturnStatement&) { m_returnStmts++; }
    void visit(ExpressionStatement&) { m_expressionStmts++; }
    void visit(EmptyStatement&) { m_emptyStmts++; }
    void visit(Canto::HirBreakStatement&) { m_breakStmts++; }
    void visit(Canto::HirContinueStatement&) { m_continueStmts++; }
    void visit(BasicLiteralExpression&) { m_basicLiterals++; }
    void visit(StrictLiteralExpression&) { m_strictLiterals++; }
    void visit(CompoundLiteralExpression&) { m_compoundLiterals++; }
    void visit(ReferenceExpression&) { m_refExprs++; }
    void visit(SubroutineReferenceExpression&) { m_subroutineRefExprs++; }
    void visit(StructAccessExpression&) { m_structAccessExprs++; }
    void visit(EnumAccessExpression&) { m_enumAccessExprs++; }
    void visit(TypeCastExpression&) { m_typeCastExprs++; }
    void visit(BasicCallExpression&) { m_basicCallExprs++; }
    void visit(SubroutineCallExpression&) { m_subroutineCallExprs++; }
    void visit(EmptyExpression&) { m_emptyExprs++; }

    void printStatistics(std::ostream &os) const
    {
        os << "Subroutine decls: " << m_subroutineDecls << std::endl;
        os << "    with body: " << m_subroutinesWithBody << std::endl;
        os << "    unique: " << m_uniqueSubroutines << std::endl;
        os << "Variable decls: " << m_variableDecls << std::endl;
        os << "    globals: " << m_globalVariables << std::endl;
        os << "Type decls: " << m_typeDecls << std::endl;
        os << "For statements: " << m_forLoops << std::endl;
        os << "While statements: " << m_whileLoops << std::endl;
        os << "If statements: " << m_ifStmts << std::endl;
        os << "Switch statements: " << m_switchStmts << std::endl;
        os << "Goto statements: " << m_gotoStmts << std::endl;
        os << "Return statements: " << m_returnStmts << std::endl;
        os << "Expression statements: " << m_expressionStmts << std::endl;
        os << "Empty statements: " << m_emptyStmts << std::endl;
        os << "Break statements: " << m_breakStmts << std::endl;
        os << "Continue statements: " << m_continueStmts << std::endl;
        os << "Basic literals: " << m_basicLiterals << std::endl;
        os << "Strict literals: " << m_strictLiterals << std::endl;
        os << "Compound : " << m_compoundLiterals << std::endl;
        os << "Variable references: " << m_refExprs << std::endl;
        os << "Subroutine references: " << m_subroutineRefExprs << std::endl;
        os << "Struct memeber accesses: " << m_structAccessExprs << std::endl;
        os << "Enum accesses: " << m_enumAccessExprs << std::endl;
        os << "Type casts: " << m_typeCastExprs << std::endl;
        os << "Basic calls: " << m_basicCallExprs << std::endl;
        os << "Subroutine calls: " << m_subroutineCallExprs << std::endl;
        os << "Empty expressions: " << m_emptyExprs << std::endl;
        //os << ":" << m_ << std::endl;
        //os << ":" << m_ << std::endl;
    }
private:
    int m_subroutineDecls;
    int m_subroutinesWithBody;
    int m_uniqueSubroutines;
    int m_variableDecls;
    int m_globalVariables;
    int m_typeDecls;
    int m_forLoops;
    int m_whileLoops;
    int m_ifStmts;
    int m_switchStmts;
    int m_gotoStmts;
    int m_returnStmts;
    int m_expressionStmts;
    int m_emptyStmts;
    int m_breakStmts;
    int m_continueStmts;
    int m_basicLiterals;
    int m_strictLiterals;
    int m_compoundLiterals;
    int m_refExprs;
    int m_subroutineRefExprs;
    int m_structAccessExprs;
    int m_enumAccessExprs;
    int m_typeCastExprs;
    int m_basicCallExprs;
    int m_subroutineCallExprs;
    int m_emptyExprs;
};

std::ostream& operator<<(std::ostream& os, const std::map<std::string,int>& stats)
{
    std::map<std::string,int>::const_iterator it = stats.begin();
    for(; it != stats.end(); ++it)
    {
        os << it->first << " : " << it->second << std::endl;
    }
    return os;
}

class VariableTypeAnalyser : public StatisticsAnalyserBase
                           , public OPS::Visitor<VariableDeclaration>
{
    class TypeToTextWalker : public OPS::Reprise::Service::DeepWalker
    {
    public:
        std::string getText() const { return m_text; }
        void visit(BasicType&)
        {
            m_text = "BasicType";
        }
        void visit(Canto::HirCBasicType&)
        {
            m_text = "CBasicType";
        }
        void visit(PtrType& ptr)
        {
            ptr.getPointedType().accept(*this);
            m_text += "*";
        }
        void visit(ArrayType& array)
        {
            array.getBaseType().accept(*this);
            m_text += "[]";
        }
        void visit(StructType& str)
        {
            m_text = "struct";
        }
        void visit(EnumType& enm)
        {
            m_text = "enum";
        }
        void visit(SubroutineType&)
        {
            m_text = "sub";
        }
        void visit(DeclaredType& decl)
        {
            decl.getDeclaration().getType().accept(*this);
        }
        void visit(VectorType& vec)
        {
            vec.getBaseType().accept(*this);
            m_text += "v";
        }

    private:
        std::string m_text;
    };

public:
    void visit(VariableDeclaration& var)
    {
        if (var.hasDefinedBlock() || var.hasDefinition() && &var.getDefinition() == &var)
        {
            TypeToTextWalker walker;
            var.getType().accept(walker);
            m_stats[walker.getText()]++;
        }
    }

    void printStatistics(std::ostream &os) const
    {
        os << "---- Variable types -----" << std::endl;
        os << m_stats;
    }
private:
    std::map<std::string,int> m_stats;
};

class ForLoopHeaderAnalyser : public StatisticsAnalyserBase
                            , public OPS::Visitor<ForStatement>
                            , public OPS::Visitor<WhileStatement>
{
    class ExprToTextWalker : public OPS::Reprise::Service::DeepWalker
    {
    public:
        std::string getText(ExpressionBase& expr)
        {
            expr.accept(*this);
            return m_stack.top();
        }
        void visit(ReferenceExpression& ref)
        {
            std::map<VariableDeclaration*,char>::iterator it = m_decls.find(&ref.getReference());
            if (it != m_decls.end())
                m_stack.push(std::string(1, it->second));
            else
            {
                char name = 'A' + m_decls.size();
                m_decls[&ref.getReference()] = name;
                m_stack.push(std::string(1, name));
            }
        }
        void visit(StrictLiteralExpression&)
        {
            m_stack.push("c");
        }
        void visit(SubroutineCallExpression&)
        {
            m_stack.push("()");
        }
        void visit(StructAccessExpression&)
        {
            //m_stack.top() = m_stack.top() + ".m";
            m_stack.push(".");
        }
        void visit(EnumAccessExpression&)
        {
            m_stack.push("e");
        }
        void visit(TypeCastExpression& e)
        {
            DeepWalker::visit(e);
            m_stack.top() = m_stack.top() + "<t>";
        }
        void visit(BasicCallExpression& call)
        {
            DeepWalker::visit(call);
            if (call.getArgumentCount() == 1)
            {
                m_stack.top() = BasicCallExpression::builtinCallKindToString(call.getKind()) + m_stack.top();
            }
            else if (call.getArgumentCount() == 2)
            {
                std::string s = m_stack.top();
                m_stack.pop();
                m_stack.top() = m_stack.top() + BasicCallExpression::builtinCallKindToString(call.getKind()) + s;
            }
            else
                m_stack.top() = "3";
        }
        void visit(EmptyExpression&)
        {
            m_stack.push("");
        }

    private:
        std::stack<std::string> m_stack;
        std::map<VariableDeclaration*, char> m_decls;
    };

public:
    ForLoopHeaderAnalyser():m_total(0),m_basic(0),m_constBoundsCount(0) {}
    void printStatistics(std::ostream &os) const
    {
        os << "---- For loop types -----" << std::endl;
        os << "Total for loops: " << m_total << std::endl;
        os << "Basic for loops: " << m_basic << std::endl;
        os << "Basic loops with const bounds: " << m_constBoundsCount << std::endl;
        os << m_headerStats;
        if (!m_constBounds.empty())
        {
            os << std::endl << "Constant bounds: " << std::endl;
            os << m_constBounds;
        }
        os << std::endl << "Basic for increments: " << std::endl;
        os << m_incrementStats;
        //os << std::endl << "While loops: " << std::endl;
        //os << m_whileStats;
    }

    std::string getInitKind(ExpressionBase& expr)
    {
        if (BasicCallExpression* bce = expr.cast_ptr<BasicCallExpression>())
            return BasicCallExpression::builtinCallKindToString(bce->getKind());
        else if (expr.is_a<EmptyExpression>())
            return "emp";
        return "oth";
    }

    std::string getFinalKind(ExpressionBase& expr)
    {
        if (BasicCallExpression* bce = expr.cast_ptr<BasicCallExpression>())
            return BasicCallExpression::builtinCallKindToString(bce->getKind());
        else
            return "oth";
    }

    std::string getStepKind(ExpressionBase& expr)
    {
        if (BasicCallExpression* bce = expr.cast_ptr<BasicCallExpression>())
        {
            if (bce->getKind() == BasicCallExpression::BCK_ASSIGN)
            {
                if (BasicCallExpression* child = bce->getArgument(1).cast_ptr<BasicCallExpression>())
                {
                    return "=" + BasicCallExpression::builtinCallKindToString(child->getKind());
                }
            }
            return BasicCallExpression::builtinCallKindToString(bce->getKind());
        }
        else
            return "oth";
    }

    void visit(ForStatement& forStmt)
    {
        m_total++;
        ExprToTextWalker walker;

        std::string kind = walker.getText(forStmt.getInitExpression()) + " ; "
                + walker.getText(forStmt.getFinalExpression()) + " ; "
                + walker.getText(forStmt.getStepExpression()) + "   ";

        m_headerStats[kind]++;

        if (OPS::Reprise::Editing::forIsBasic(forStmt))
        {
            m_basic++;
            StrictLiteralExpression* initBound = OPS::Reprise::Editing::getBasicForInitExpression(forStmt).cast_ptr<StrictLiteralExpression>();
            StrictLiteralExpression* finalBound = OPS::Reprise::Editing::getBasicForFinalExpression(forStmt).cast_ptr<StrictLiteralExpression>();
            if (initBound != 0 && finalBound != 0)
            {
                m_constBoundsCount++;
                m_constBounds[initBound->getLiteralValueAsString(false) + ":" + finalBound->getLiteralValueAsString(false)]++;
            }

            m_incrementStats[OPS::Reprise::Editing::getBasicForStep(forStmt).dumpState()]++;
        }
    }

    void visit(WhileStatement& whileStmt)
    {
        ExprToTextWalker walker;
        m_whileStats[walker.getText(whileStmt.getCondition())]++;
    }

private:
    int m_total;
    int m_basic;
    int m_constBoundsCount;
    std::map<std::string,int> m_headerStats;
    std::map<std::string,int> m_constBounds;
    std::map<std::string,int> m_incrementStats;
    std::map<std::string,int> m_whileStats;
};

class ForStructureAnalyser : public StatisticsAnalyserBase
                           , public OPS::Visitor<ForStatement>
                           , public OPS::Visitor<Canto::HirBreakStatement>
                           , public OPS::Visitor<Canto::HirContinueStatement>
                           , public OPS::Visitor<ReturnStatement>
                           , public OPS::Visitor<GotoStatement>
                           , public OPS::Visitor<SubroutineCallExpression>
                           , public OPS::Visitor<IfStatement>
                           , public OPS::Visitor<PlainSwitchStatement>
{
public:
    ForStructureAnalyser()
        :m_perfect(0)
        ,m_emptyBody(0)
    {}

    void visit(ForStatement& forLoop)
    {
        if (OPS::Shared::isPerfectLoopNest(&forLoop))
            m_perfect++;
        if (forLoop.getBody().isEmpty())
            m_emptyBody++;
    }

    void visit(Canto::HirBreakStatement& brk)
    {
    }

    void visit(Canto::HirContinueStatement& cont)
    {
        std::list<ForStatement*> nest = OPS::Shared::getEmbracedLoopsNest(cont);
        m_badCtrl.insert(nest.begin(), nest.end());
    }

    void visit(ReturnStatement& rtn)
    {
        std::list<ForStatement*> nest = OPS::Shared::getEmbracedLoopsNest(rtn);
        m_badCtrl.insert(nest.begin(), nest.end());
    }

    void visit(GotoStatement& gto)
    {
        std::list<ForStatement*> nest = OPS::Shared::getEmbracedLoopsNest(gto);
        for(std::list<ForStatement*>::iterator it = nest.begin(); it != nest.end(); ++it)
        {
            if (!OPS::Shared::contain(*it, gto.getPointedStatement()))
            {
                m_badCtrl.insert(*it);
            }
        }
    }

    void visit(SubroutineCallExpression& call)
    {
        std::list<ForStatement*> nest = OPS::Shared::getEmbracedLoopsNest(call);
        m_calls.insert(nest.begin(), nest.end());
    }

    void visit(IfStatement& ifStmt)
    {
        std::list<ForStatement*> nest = OPS::Shared::getEmbracedLoopsNest(ifStmt);
        if (!nest.empty())
            m_cond.insert(nest.back());
    }

    void visit(PlainSwitchStatement& switchStmt)
    {
        std::list<ForStatement*> nest = OPS::Shared::getEmbracedLoopsNest(switchStmt);
        if (!nest.empty())
            m_cond.insert(nest.back());
    }

    void printStatistics(std::ostream &os) const
    {
        os << "-- For loop structure --" << std::endl;
        os << "Perfect nests: " << m_perfect << std::endl;
        os << "Bad control flow loops: " << m_badCtrl.size() << std::endl;
        os << "Loops with subroutine calls: " << m_calls.size() << std::endl;
        os << "Loops with if or switch: " << m_cond.size() << std::endl;
        os << "Loops with empty body block: " << m_emptyBody << std::endl;
    }

private:
    int m_perfect;
    int m_emptyBody;
    std::set<ForStatement*> m_badCtrl;
    std::set<ForStatement*> m_calls;
    std::set<ForStatement*> m_cond;
};

class SubscriptAnalyser : public StatisticsAnalyserBase
                        , public OPS::Visitor<BasicCallExpression>
                        , public OPS::Visitor<Canto::HirCCallExpression>
{
public:
    SubscriptAnalyser()
        :m_subscripts(0)
        ,m_evaluatable(0)
        ,m_independent(0)
        ,m_notLinear(0)
        ,m_pleErrors(0)
    {}
    void visit(BasicCallExpression& bce)
    {
        if (bce.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
        {
            m_dims[OPS::Strings::format("%dD", bce.getArgumentCount()-1)]++;
            for(int i = 1; i < bce.getArgumentCount(); ++i)
            {
                m_subscripts++;
                try
                {
                    OPS::Shared::ParametricLinearExpression* ple =
                            OPS::Shared::ParametricLinearExpression::createByIndexes(&bce.getArgument(i));
                    if (ple)
                    {
                        if (ple->isEvaluatable())
                            m_evaluatable++;
                        if (ple->isIndependent())
                            m_independent++;
                        delete ple;
                    }
                    else
                    {
                        m_notLinear++;
                        m_notLinearText += bce.getArgument(i).dumpState() + "\n";
                    }
                }
                catch(OPS::RuntimeError& err)
                {
                    m_pleErrors++;
                    m_errorsText += err.getMessage() + " : " + bce.getArgument(i).dumpState() + "\n";
                }
            }
        }
    }

    void visit(Canto::HirCCallExpression& bce)
    {
        if (bce.getKind() == Canto::HirCCallExpression::HIRC_ARRAY_ACCESS)
        {
            m_dims[OPS::Strings::format("%dD", bce.getArgumentCount()-1)]++;
            m_subscripts += bce.getArgumentCount();
        }
    }

    void printStatistics(std::ostream &os) const
    {
        os << "-- Subscripts --" << std::endl;
        os << "Number of subscripts: " << m_subscripts << std::endl;
        os << "Linear: " << m_subscripts - m_notLinear - m_pleErrors << std::endl;
        os << "    Evaluatable: " << m_evaluatable << std::endl;
        os << "    Independent: " << m_independent << std::endl;
        os << "Not Linear: " << m_notLinear << std::endl;
        os << "PLE errors: " << m_pleErrors << std::endl;
        os << "Dimentions: " << std::endl;
        os << m_dims << std::endl;
        //os << "Errors: " << std::endl;
        //os << m_errorsText << std::endl;
        //os << "Not linear: " << std::endl;
        //os << m_notLinearText;
    }
private:
    int m_subscripts;
    int m_evaluatable;
    int m_independent;
    int m_notLinear;
    int m_pleErrors;
    std::map<std::string, int> m_dims;
    std::string m_errorsText;
    std::string m_notLinearText;
};

class MemorySizeAnalyser : public StatisticsAnalyserBase
                         , public OPS::Visitor<ProgramUnit>
{
public:
    MemorySizeAnalyser()
        :m_size(0) {}

    void visit(ProgramUnit& program)
    {
        OPS::Reprise::Service::makePostOrderTraversal(program, *this);
    }

    void operator()(RepriseBase& node)
    {
        m_size += node.getObjectSize();
    }

    void printStatistics(std::ostream &os) const
    {
        os << "-- Memory usage --" << std::endl;
        os << "Reprise objects : " << m_size << " bytes" << std::endl;
    }

private:
    size_t m_size;
};

class StructAnalyser : public StatisticsAnalyserBase
                     , public OPS::Visitor<StructType>
{
public:
    StructAnalyser()
        :m_totalStruct(0)
        ,m_totalUnion(0)
        ,m_recurse(0)
    {}

    void printStatistics(std::ostream &os) const
    {
        os << "-- Structs --" << std::endl;
        os << "Structs : " << m_totalStruct << std::endl;
        os << "Unions : " << m_totalUnion << std::endl;
        os << "Recurse : " << m_recurse << std::endl;
    }

    void visit(StructType& str)
    {
        TypeDeclaration* decl = 0;
        decl = str.getParent()->cast_ptr<TypeDeclaration>();

        if (decl == 0)
        {
            if (TypedefType* typed = str.getParent()->cast_ptr<TypedefType>())
                decl = typed->getParent()->cast_ptr<TypeDeclaration>();
        }

        if (decl)
        {
            if (m_names.find(decl->getName()) != m_names.end())
                return;
            else
                m_names.insert(decl->getName());
        }

        if (str.isUnion())
            m_totalUnion++;
        else
            m_totalStruct++;

        if (str.isFullType())
        {
            for(int i = 0; i < str.getMemberCount(); ++i)
            {
                TypeBase& memberType = OPS::Reprise::Editing::desugarType(str.getMember(i).getType());
                if (PtrType* ptr = memberType.cast_ptr<PtrType>())
                {
                    if (&str == &OPS::Reprise::Editing::desugarType(ptr->getPointedType()))
                        m_recurse++;
                }
            }
        }
    }
private:
    int m_totalStruct;
    int m_totalUnion;
    int m_recurse;
    std::set<std::string> m_names;
};

StatisticsWalker::StatisticsWalker(const std::list<StatisticsAnalyserBase *> &analysers)
    :m_analysers(analysers)
{
}

void StatisticsWalker::printStatistics(std::ostream &os) const
{
    for(AnalysersList::const_iterator it = m_analysers.begin(); it != m_analysers.end(); ++it)
    {
        (*it)->printStatistics(os);
        os << std::endl;
    }
}

void StatisticsWalker::operator()(RepriseBase &node)
{
    for(AnalysersList::const_iterator it = m_analysers.begin(); it != m_analysers.end(); ++it)
    {
        node.accept(**it);
    }
}

void OPS::Analysis::Statistics::makeStatistics(OPS::Reprise::RepriseBase &node, const std::list<StatisticsAnalyserBase *> &analysers, std::ostream &os)
{
    StatisticsWalker walker(analysers);
    OPS::Reprise::Service::makePreOrderTraversal(node, walker);
    walker.printStatistics(os);
}

StatisticsAnalyserBase* OPS::Analysis::Statistics::createNodeCountAnalyser()
{
    return new NodeCounterAnalyser;
}

StatisticsAnalyserBase* OPS::Analysis::Statistics::createVariableTypesAnalyser()
{
    return new VariableTypeAnalyser;
}

StatisticsAnalyserBase* OPS::Analysis::Statistics::createForLoopHeaderAnalyser()
{
    return new ForLoopHeaderAnalyser;
}

StatisticsAnalyserBase* OPS::Analysis::Statistics::createForStructureAnalyser()
{
    return new ForStructureAnalyser;
}

StatisticsAnalyserBase* OPS::Analysis::Statistics::createSubscriptAnalyser()
{
    return new SubscriptAnalyser;
}

StatisticsAnalyserBase* OPS::Analysis::Statistics::createMemorySizeAnalyser()
{
    return new MemorySizeAnalyser;
}

StatisticsAnalyserBase* OPS::Analysis::Statistics::createStructAnalyser()
{
    return new StructAnalyser;
}
