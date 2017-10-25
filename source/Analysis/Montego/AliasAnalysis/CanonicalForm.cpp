#include "Analysis/Montego/AliasAnalysis/CanonicalForm.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "FrontTransforms/C2RPass.h"
#include<fstream>

namespace OPS
{
namespace Montego
{

//проверяет, является ли данное выражение элементарным вхождением
bool isOccurrence(ExpressionBase& e, bool allowReference)
{
    //если это переменная
    ReferenceExpression* ref = e.cast_ptr<ReferenceExpression>();
    if (ref != 0) return true;

    //если это обращение к элементу массива
    BasicCallExpression* bce = e.cast_ptr<BasicCallExpression>();
    if (bce != 0)
    {
        if (bce->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
            return isOccurrence(bce->getArgument(0), false);
        else
            if (bce->getKind() == BasicCallExpression::BCK_TAKE_ADDRESS && allowReference) return true;
            return false;
    }
    
    //если это обращение к полю структуры
    StructAccessExpression* sac = e.cast_ptr<StructAccessExpression>();
    if (sac != 0) return isOccurrence(sac->getStructPointerExpression(), false);

    //если это имя функции
    SubroutineReferenceExpression* sref = e.cast_ptr<SubroutineReferenceExpression>();
    if (sref != 0) return true;

    //если это ничто из вышеперечисленного
    return false;
}

//возвращает true для нулевого аргумента операций [] и ".", если он не является элементарным вхождением
bool predicateFor2Transform(ExpressionBase& e)
{
    ExpressionBase& headNode = e.obtainRootExpression();
    if (&e == &headNode) return false;
    
    RepriseBase* parent = e.getParent();
    BasicCallExpression* p_bce = parent->cast_ptr<BasicCallExpression>();
    if (p_bce != 0) 
    {
        if (p_bce->getKind() != BasicCallExpression::BCK_ARRAY_ACCESS) return false;
        if (&p_bce->getChild(0) != &e) return false;
        return !isOccurrence(e, false);
    }
    else
    {
        StructAccessExpression* p_sac = parent->cast_ptr<StructAccessExpression>();
        if (p_sac == 0) return false;
        if (&p_sac->getChild(0) != &e) return false;
        return !isOccurrence(e, false);
    }
}

class IsConstantVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    IsConstantVisitor() : m_isConstant(true) {}
    bool isConstant() { return m_isConstant; }
	void visit(ReferenceExpression&) { m_isConstant = false; }
	void visit(SubroutineCallExpression&) { m_isConstant = false; }

private:
    bool m_isConstant;
};

//возвращает true для аргументов вызовов функции, которые содержат вхождения, но ими не являются
//или содержат вызовы функций
//И для имени функции, если оно не является вхождением
bool predicateFor3Transform(ExpressionBase& e)
{
    ExpressionBase& headNode = e.obtainRootExpression();
    if (&e == &headNode) return false;

    RepriseBase* parent = e.getParent();
    SubroutineCallExpression* p_sce = parent->cast_ptr<SubroutineCallExpression>();
    if (p_sce == 0) return false;
    if (&p_sce->getChild(0) == &e)
    {
        //e - имя функции
        return !isOccurrence(e, false);
    }
    else
    {
        //е - аргумент функции
        IsConstantVisitor vis;
        e.accept(vis);
        if (!isOccurrence(e, true) && !vis.isConstant())
            return true;
        else return false;
    }
}
//возвращает true если это выражение ? :
bool predicateForConditionalExpressions(ExpressionBase& e)
{
    BasicCallExpression* bce = e.cast_ptr<BasicCallExpression>();
    if (bce != 0)
    {
        if (bce->getKind() == BasicCallExpression::BCK_CONDITIONAL &&
            bce->obtainParentStatement() != 0)
            return true;
    }
    return false;
}

void apply4CanonicalTransforms(RepriseBase& code)
{
    convertDereferencesToArrayAccess(code);

    convertConditionalExpressions(code);


    convertSubexpressionsToReferences(code, predicateFor2Transform);

    convertSubexpressionsToReferences(code, predicateFor3Transform);
}

//находит все * и &
class DereferenceSeacher : public OPS::Reprise::Service::DeepWalker
{
public:
    std::list<BasicCallExpression*> m_derefs;
    std::list<BasicCallExpression*> m_refs;
    DereferenceSeacher(){}
    ~DereferenceSeacher(){}

    void visit(OPS::Reprise::BasicCallExpression& e)
    {
        DeepWalker::visit(e);
        if (e.getKind() == Reprise::BasicCallExpression::BCK_DE_REFERENCE)
            m_derefs.push_back(&e);
        if (e.getKind() == Reprise::BasicCallExpression::BCK_TAKE_ADDRESS)
            m_refs.push_back(&e);
    }
};

bool isPointer2Func(RepriseBase* node)
{
    ExpressionBase* e = node->cast_ptr<ExpressionBase>();
    if (!e) return false;
    ReprisePtr<TypeBase> resultType = e->getResultType();
    TypeBase* t = &Editing::desugarType(*resultType);
    if (t->is_a<SubroutineType>()) return true;
    PtrType* pt = t->cast_ptr<PtrType>();
    if (pt && Editing::desugarType(pt->getPointedType()).is_a<SubroutineType>()) return true;
    return false;
}

void convertDereferencesToArrayAccess(RepriseBase& code)
{
    DereferenceSeacher walker;
    code.accept(walker);
    std::list<BasicCallExpression*> derefs = walker.m_derefs;
    std::list<BasicCallExpression*>::iterator it;
    for (it = derefs.begin(); it != derefs.end(); ++it)
    {
        BasicCallExpression& e = **it;
        //if it is pointer to subroutine, we remove unused *
        if (isPointer2Func(&e.getChild(0)))
        {
            Editing::replaceExpression(e, ReprisePtr<ExpressionBase>(e.getChild(0).cast_ptr<ExpressionBase>()));
            continue;
        }

        Reprise::StrictLiteralExpression* zero = new Reprise::StrictLiteralExpression();
        zero->setUInt32(0);

        Reprise::BasicCallExpression* chBce = e.getArgument(0).cast_ptr<Reprise::BasicCallExpression>();
        Reprise::BasicCallExpression* parBce = e.getParent()->cast_ptr<Reprise::BasicCallExpression>();

        Reprise::BasicCallExpression* headNodeToReplace;
        int bracketNumParent = 0, bracketNumChild = 0;

        if ((parBce != 0 ) && (parBce->getKind() == Reprise::BasicCallExpression::BCK_ARRAY_ACCESS)
            && &parBce->getChild(0) == &e)
        {
            headNodeToReplace = parBce;
			bracketNumParent = parBce->getArgumentCount() - 1;
        }
        else
            headNodeToReplace = &e;
        if ( (chBce != 0) && (chBce->getKind() == Reprise::BasicCallExpression::BCK_ARRAY_ACCESS) )
			bracketNumChild = chBce->getArgumentCount() - 1;

        //формируем новую операцию [][][][]
        Reprise::BasicCallExpression* newBce = new Reprise::BasicCallExpression(Reprise::BasicCallExpression::BCK_ARRAY_ACCESS);
        
        //нулевой аргумент
        if (bracketNumChild > 0)
			newBce->addArgument(&chBce->getArgument(0));
        else
			newBce->addArgument(&e.getArgument(0));

        //остальные аргументы
        if (bracketNumChild > 0)
            for (int i = 0; i < bracketNumChild; ++i)
                newBce->addArgument(&chBce->getArgument(i+1));
        newBce->addArgument(zero);
        if (bracketNumParent > 0)
            for (int i = 0; i < bracketNumParent; ++i)
                newBce->addArgument(&parBce->getArgument(i+1));

        ReprisePtr<BasicCallExpression> p(newBce);
        Editing::replaceExpression(*headNodeToReplace, p);
    }
    //delete unused & before functions
    for (it = walker.m_refs.begin(); it != walker.m_refs.end(); ++it)
    {
        BasicCallExpression& e = **it;
        if (e.getChild(0).is_a<SubroutineReferenceExpression>())
            Editing::replaceExpression(e, ReprisePtr<ExpressionBase>(e.getChild(0).cast_ptr<ExpressionBase>()));
    }
}

//оставляем внутри текущего выражения только, если это условие while или for
bool isPutSubexInside(ExpressionBase& e)
{
    bool res = false;
    if (e.obtainParentStatement()->is_a<WhileStatement>())
        res = true;
    if (e.obtainParentStatement()->is_a<ForStatement>())
    {
        ForStatement* forstmt = e.obtainParentStatement()->cast_ptr<ForStatement>();
        if (&e.obtainRootExpression() != &forstmt->getInitExpression())
            res = true;
    }
    return res;
}

// 0 - если e внутри ExprStatement или InitExpr For
// 1 - если e внутри FinalExpr For или Expr While do
// 2 - если e внутри StepExpr For или Expr do while
int whereToPutIf(ExpressionBase& e)
{
    WhileStatement* whl = e.obtainParentStatement()->cast_ptr<WhileStatement>();
    if (whl && (whl->isPreCondition()))  return 1;  
    if (whl && (!whl->isPreCondition()))  return 2;  
    
    ForStatement* forstmt = e.obtainParentStatement()->cast_ptr<ForStatement>();
    if (forstmt)
    {
        ExpressionBase* pare = &e.obtainRootExpression();
        if (pare == &forstmt->getInitExpression())  return 0;
        if (pare == &forstmt->getFinalExpression())  return 1;
        if (pare == &forstmt->getStepExpression())  return 2;
    }

    return 0;
}

ReprisePtr<TypeBase> getTypeOfExpression(ExpressionBase& e)
{
    ReprisePtr<TypeBase> t = e.getResultType();
    t->setConst(false);
    return t;
}

//создает переменную, заменяет ею подвыражение expr, добавляет присвоение этой переменной подвыражения
void addAssign(ExpressionBase& expr)
{
    bool flagisPutSubexInside = isPutSubexInside(expr);
    ReprisePtr<ExpressionBase> pexpr(&expr);
    ExpressionBase& headNode = expr.obtainRootExpression();
    StatementBase* thisStmt = expr.obtainParentStatement();
    BlockStatement& block = thisStmt->getParentBlock();

    //создаем переменную
    ReprisePtr<TypeBase> tp = getTypeOfExpression(expr);
    VariableDeclaration& var = Editing::createNewVariable(*tp,block);
    ReprisePtr<ReferenceExpression> varref(new ReferenceExpression(var));
    Editing::replaceExpression(expr,varref);//заменили подвыражение на переменную

    //конструируем присвоение
    BasicCallExpression* assign = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, varref->clone(), pexpr.release());

    if (flagisPutSubexInside)
    {
        //добавляем присвоение к головному узлу выражения, через запятую
        EmptyExpression* empty2 = new EmptyExpression();
        ReprisePtr<BasicCallExpression> comma(new BasicCallExpression(BasicCallExpression::BCK_COMMA, assign, empty2));
        ReprisePtr<ExpressionBase> pHeadNode = Editing::replaceExpression(headNode,comma);
        Editing::replaceExpression(*empty2,pHeadNode);
    }
    else
    {
        //добавляем присвоение перед оператором            
        ExpressionStatement* assign_stmt = new ExpressionStatement(assign);
        block.addBefore(block.convertToIterator(thisStmt),assign_stmt);
        //перемещаем метку, если она есть
        OPS::Reprise::Editing::moveLabels(*thisStmt, *assign_stmt);
    }
}

//создает переменную, заменяет ею подвыражение expr = ?:, добавляет оператор if с присвоениями 
//этой переменной подвыражений истины и лжи
void addIf(ExpressionBase& exprB)
{
    BasicCallExpression* tmp = exprB.cast_ptr<BasicCallExpression>();
    OPS_ASSERT(tmp != 0);
    BasicCallExpression& expr = *tmp;
    ReprisePtr<ExpressionBase> pexpr(&expr);//чтобы не удалилось раньше времени

    int whereToPut = whereToPutIf(expr);
	ReprisePtr<ExpressionBase> pexprCond(&expr.getArgument(0));
	ReprisePtr<ExpressionBase> pexprTrue(&expr.getArgument(1));
	ReprisePtr<ExpressionBase> pexprFalse(&expr.getArgument(2));
	//ExpressionBase& headNode = expr.obtainRootExpression();
    StatementBase* thisStmt = expr.obtainParentStatement();
    BlockStatement& block = thisStmt->getParentBlock();

    //создаем переменную
	ReprisePtr<TypeBase> tp(getTypeOfExpression(expr.getArgument(1)));
    VariableDeclaration& var = Editing::createNewVariable(*tp,block);
    ReprisePtr<ReferenceExpression> varref(new ReferenceExpression(var));
    Editing::replaceExpression(expr, varref);//заменили подвыражение на переменную

    //конструируем оператор if с присвоениями
    EmptyExpression* emptyTrue =new EmptyExpression();
    BasicCallExpression* assignTrue = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, varref->clone(), emptyTrue);
    EmptyExpression* emptyFalse =new EmptyExpression();
    BasicCallExpression* assignFalse = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, varref->clone(), emptyFalse);
    Editing::replaceExpression(*emptyTrue, pexprTrue);
    Editing::replaceExpression(*emptyFalse, pexprFalse);
	IfStatement* ifstmt = new IfStatement(&expr.getArgument(0));
    BlockStatement* blockTrue = new BlockStatement();
    BlockStatement* blockFalse = new BlockStatement();
    blockTrue->addFirst(new ExpressionStatement(assignTrue));
    blockFalse->addFirst(new ExpressionStatement(assignFalse));
    ifstmt->setThenBody(blockTrue);
    ifstmt->setElseBody(blockFalse);

    switch (whereToPut)
    {
    case 0:
        {
            //добавляем if перед оператором            
            block.addBefore(block.convertToIterator(thisStmt),ifstmt);
            //перемещаем метку, если она есть
            if (thisStmt->hasLabel())
            {
                ifstmt->setLabel(thisStmt->getLabel());
                thisStmt->setLabel("");
            }
            break;
        }
    case 1:
        {
            //выносим initExpr, если это for
            if (thisStmt->is_a<ForStatement>())
            {
                ForStatement* fstmt = thisStmt->cast_ptr<ForStatement>();
                ExpressionStatement* initExprStmt = new ExpressionStatement(&fstmt->getInitExpression());
                fstmt->setInitExpression(new EmptyExpression);
                block.addBefore(block.convertToIterator(thisStmt),initExprStmt);
            }
            //добавляем if перед оператором for или while
            block.addBefore(block.convertToIterator(thisStmt),ifstmt);
            //добавляем if в конец тела цикла
            BlockStatement* cycleBlock = 0;
            if (ForStatement* fstmt = thisStmt->cast_ptr<ForStatement>()) cycleBlock = &fstmt->getBody();
            if (WhileStatement* wstmt = thisStmt->cast_ptr<WhileStatement>()) cycleBlock = &wstmt->getBody();
            OPS_ASSERT(cycleBlock != 0);
            cycleBlock->addLast(ifstmt->clone());
            break;
        }
    case 2:
        {
            //добавляем if в конец тела цикла
            BlockStatement* cycleBlock = 0;
            if (ForStatement* fstmt = thisStmt->cast_ptr<ForStatement>()) cycleBlock = &fstmt->getBody();
            if (WhileStatement* wstmt = thisStmt->cast_ptr<WhileStatement>()) cycleBlock = &wstmt->getBody();
            OPS_ASSERT(cycleBlock != 0);
            cycleBlock->addLast(ifstmt->clone());
            break;
        }
    default: throw OPS::RuntimeError("Unknown type!");
    }
}

//Заменяет все подвыражения, удовлетворяющие предикату на ReferenceExpression новой переменной,
//предварительно определив эту переменную и присвоив ей подвыражение 
//Если подвыражение внутри ExpressionStatement, то присвоения добавляем операторами перед ним.
//Если выражение внутри условия while, for то добавляем в текущее выражение через запятую
//Делает преобразование в два этапа: сначала составляет список подвыражений, которые нужно 
//сделать переменными (подвыражения могут получаться вложенными друг в друга). Порядок
//в списке очень важен - сначала внутренние, потом внешние.
//Второй этап - вводим новые переменные и выносим.
void convertSubexpressionsToReferences(RepriseBase& code, bool (*predicate)(Reprise::ExpressionBase&))
{
    //собираем все подвыражения, которые нужно сделать переменными
    SubexpressionCollector subexpressionCollector(predicate);
    code.accept(subexpressionCollector);
    std::list<ExpressionBase*> exToConvert = subexpressionCollector.m_exToConvert;
    
    std::list<ExpressionBase*>::iterator it;
    for (it = exToConvert.begin(); it != exToConvert.end(); ++it)
    {
        ExpressionBase& expr = **it;
        addAssign(expr);
    }
}

//Заменяет все подвыражения ?: на ReferenceExpression новой переменной и если придется, то первые аргументы
//операций с фиксированным порядком выполнения аргументов на ReferenceExpression новой переменной
//Предварительно определяем переменные и присваиваем им подвыражения 
//(для ?: определяются 2 переменные для 2х выражений - иначе будет некорректно из-за преобразования типов)
//Если подвыражение ?: внутри ExpressionStatement или инициализирующем выражении for, 
//то оператор if с присвоениями добавляем операторам перед ним.
//Если выражение ?: внутри условия while или finalExpr for то добавляем перед оператором и 
//внутрь в конец блока этого оператора оператор if с присвоениями
//Делает преобразование в два этапа: сначала составляет список подвыражений, которые нужно 
//сделать переменными (подвыражения могут получаться вложенными друг в друга). Порядок
//в списке очень важен - сначала внутренние, потом внешние.
//Второй этап - вводим новые переменные и выносим.
void convertConditionalExpressions(RepriseBase& code)
{
    //собираем все подвыражения, которые нужно сделать переменными
    SubexpressionCollector subexpressionCollector(predicateForConditionalExpressions);
    code.accept(subexpressionCollector);
    std::list<ExpressionBase*> exToConvert = subexpressionCollector.m_exToConvert;

    std::list<ExpressionBase*>::iterator it;
    for (it = exToConvert.begin(); it != exToConvert.end(); ++it)
    {
        ExpressionBase& expr = **it;
        if (predicateForConditionalExpressions(expr))
        {
            addIf(expr);
        }
        else //это не ?:
            addAssign(expr);
    }
}


SubexpressionCollector::SubexpressionCollector(bool (*predicate)(ExpressionBase&))
{
    m_predicate = predicate;
}
SubexpressionCollector::~SubexpressionCollector()
{
}

void SubexpressionCollector::visit(BasicLiteralExpression& e)
{
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(StrictLiteralExpression& e)
{
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(CompoundLiteralExpression& e)
{
    DeepWalker::visit(e);
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(ReferenceExpression& e)
{
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(SubroutineReferenceExpression& e)
{
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(StructAccessExpression& e)
{
    DeepWalker::visit(e);
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(EnumAccessExpression& e)
{
    DeepWalker::visit(e);
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(TypeCastExpression& e)
{
    DeepWalker::visit(e);
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}
void SubexpressionCollector::visit(BasicCallExpression& e)
{
    BasicCallExpression::BuiltinCallKind kind = e.getKind();
    switch (kind)
    {
    //	Unary
    case BasicCallExpression::BCK_UNARY_PLUS:
    case BasicCallExpression::BCK_UNARY_MINUS:        
    case BasicCallExpression::BCK_SIZE_OF:
    case BasicCallExpression::BCK_TAKE_ADDRESS:
    case BasicCallExpression::BCK_DE_REFERENCE:
    case BasicCallExpression::BCK_LOGICAL_NOT:
    case BasicCallExpression::BCK_BINARY_PLUS:
    case BasicCallExpression::BCK_BINARY_MINUS:
    case BasicCallExpression::BCK_MULTIPLY:
    case BasicCallExpression::BCK_DIVISION:
    case BasicCallExpression::BCK_INTEGER_DIVISION:
    case BasicCallExpression::BCK_INTEGER_MOD:
    case BasicCallExpression::BCK_ASSIGN:
    case BasicCallExpression::BCK_LESS:
    case BasicCallExpression::BCK_GREATER:
    case BasicCallExpression::BCK_LESS_EQUAL:
    case BasicCallExpression::BCK_GREATER_EQUAL:
    case BasicCallExpression::BCK_EQUAL:
    case BasicCallExpression::BCK_NOT_EQUAL:
    case BasicCallExpression::BCK_LEFT_SHIFT:
    case BasicCallExpression::BCK_RIGHT_SHIFT:
    case BasicCallExpression::BCK_BITWISE_NOT:
    case BasicCallExpression::BCK_BITWISE_AND:
    case BasicCallExpression::BCK_BITWISE_OR:
    case BasicCallExpression::BCK_BITWISE_XOR:
    case BasicCallExpression::BCK_ARRAY_ACCESS:
    case BasicCallExpression::BCK_CONDITIONAL:
        {
            DeepWalker::visit(e);
            break;
        }
    case BasicCallExpression::BCK_LOGICAL_AND:
    case BasicCallExpression::BCK_LOGICAL_OR:
    case BasicCallExpression::BCK_COMMA:
        {
			e.getArgument(0).accept(*this);
            std::list<ExpressionBase*>::iterator lastAddedIt = m_exToConvert.end();
            if (m_exToConvert.size() > 0) lastAddedIt = --(m_exToConvert.end());
            m_exToConvert.push_back(&e.getChild(0).cast_to<ExpressionBase>()); //сначала добавляем в список, потом удалим если не надо выносить
            size_t oldSize = m_exToConvert.size();
			e.getArgument(1).accept(*this);
            //если во втором аргументе нет выносимых подвыражений, или если уже его добавляли
            //удаляем добавленный первый аргумент
            if (oldSize == m_exToConvert.size())
                m_exToConvert.erase(--m_exToConvert.end());
            else
				if ( (lastAddedIt != m_exToConvert.end())  &&  (*lastAddedIt == &e.getArgument(0)) ) 
                    m_exToConvert.erase(lastAddedIt);
            break;
        }
    default: throw RuntimeError("Unknown expression kind????");        
    }
    
    //добавляем, если нужно, текущее выражение
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(SubroutineCallExpression& e)
{
    DeepWalker::visit(e);
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(EmptyExpression& e)
{
    if (m_predicate(e)) m_exToConvert.push_back(&e);
}

void SubexpressionCollector::visit(Canto::HirCCallExpression&)
{
    throw RuntimeError("Program should be converted to Reprise!");
}

//for нужно обходить в другом порядке: сначала init, потом step и потом final, чтобы добавлять в 
//конец тела цикла операторы в нужном порядке
void SubexpressionCollector::visit(ForStatement& f)
{
    f.getInitExpression().accept(*this);
    f.getStepExpression().accept(*this);
    f.getFinalExpression().accept(*this);
    f.getBody().accept(*this);
}



CanonicalFormChecker::CanonicalFormChecker(OccurrenceContainer& occurrenceContainer)
                    :m_flagIsProgramCanonical(true),
                     m_occurrenceContainer(&occurrenceContainer)
{
}

CanonicalFormChecker::~CanonicalFormChecker()
{
}

void CanonicalFormChecker::clear()
{
    m_flagIsProgramCanonical = true;
    m_reason = "";
}

bool CanonicalFormChecker::isFragmentCanonical(OPS::Reprise::RepriseBase& fragment)
{
	m_flagIsProgramCanonical = true;
	fragment.accept(*this);
    return m_flagIsProgramCanonical;
}

std::string CanonicalFormChecker::getReason() const
{
	return m_reason;
}

void CanonicalFormChecker::visit(OPS::Reprise::BasicCallExpression& e)
{
    switch (e.getKind())
    {
    case Reprise::BasicCallExpression::BCK_ARRAY_ACCESS :
        {
            //нулевой аргумент должен быть был переменной или полем структуры
            if ( (!e.getArgument(0).is_a<Reprise::ReferenceExpression>()) && (!e.getArgument(0).is_a<Reprise::StructAccessExpression>()) )
            {
                m_flagIsProgramCanonical = false;
                m_reason = "Zero argument of [] must be a reference or struct access !";
                break;
            }
            DeepWalker::visit(e);
            break;
        }
    case Reprise::BasicCallExpression::BCK_DE_REFERENCE :
        {
            // * в программе быть не должно!!!
            m_flagIsProgramCanonical = false;
            m_reason = "Program shouldn't contain operation * !";
            break;
        }
	default: DeepWalker::visit(e);
    }
}

class SearchSubroutineCallExprVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    SearchSubroutineCallExprVisitor():m_count(0){}
    ~SearchSubroutineCallExprVisitor(){}
    void visit(OPS::Reprise::SubroutineCallExpression& e)
    {
        ++m_count;
        DeepWalker::visit(e);
    }
    int m_count;
};

void CanonicalFormChecker::visit(OPS::Reprise::SubroutineCallExpression& e)
{
    BasicOccurrencePtr o;
    if (!isOccurrence(e.getCallExpression(), false))
    {
        m_flagIsProgramCanonical = false;
        m_reason = "Call expression of function call must be an occurrence !";
        return;
    }
    for (int i = 0; i < e.getArgumentCount(); ++i)
    {
        if (m_occurrenceContainer->getOccurBy(e.getArgument(i),o))
        {
            //аргумент не является вхождением
            //проверяем, содержит ли он вхождения
            if (OccurrenceContainer::containOccurrences(e.getArgument(i)))
            {
                m_flagIsProgramCanonical = false;
                m_reason = "Arguments of function call must be an occurrences or expressions without occurrences and functions !";
                return;
            }
        }
    }
}

}//end of namespace

namespace Stage
{

CanonicalFormPass::CanonicalFormPass()
{
}

bool CanonicalFormPass::run()
{
    Montego::apply4CanonicalTransforms(workContext().program());
    workContext().addService<AliasCanonicalForm>(new AliasCanonicalForm);
    return true;
}

AnalysisUsage CanonicalFormPass::getAnalysisUsage() const
{
    return AnalysisUsage()
            .addRequired<NoCanto>()
            .addRequired<RepriseCanonical>();
}

}

}//end of namespace
