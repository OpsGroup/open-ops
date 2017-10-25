#include "Analysis/Montego/Occurrence.h"
#include "OPS_Core/msc_leakcheck.h"//контроль утечек памяти должен находиться в конце всех include !!!
#include <sstream>
#include "Reprise/Reprise.h"

namespace OPS
{
namespace Montego
{

ComplexOccurrence::ComplexOccurrence(Reprise::SubroutineCallExpression& scall, std::vector<BasicOccurrencePtr>& basicOccurrences)
		:m_subroutineCall(&scall), m_isGenerator(false), m_basicOccurrences(basicOccurrences)
{
	for (size_t i = 0; i < basicOccurrences.size(); ++i)
    {
        if (basicOccurrences[i]->isGenerator())
        {
            m_isGenerator = true;
            return;
        }
    }
}

Reprise::SubroutineCallExpression* ComplexOccurrence::getSubroutineCall() const
{
    return m_subroutineCall;
}

std::vector<BasicOccurrencePtr> ComplexOccurrence::getBasicOccurrences() const
{
    return m_basicOccurrences;
}

Reprise::StatementBase* ComplexOccurrence::getParentStatement() const
{
    return m_subroutineCall->obtainParentStatement(); 
}

Reprise::ExpressionBase* ComplexOccurrence::getSourceExpression() const
{
	return getSubroutineCall();
}

bool ComplexOccurrence::isGenerator() const
{
    return m_isGenerator;
}

bool ComplexOccurrence::isEqual(Occurrence& other) const
{
    ComplexOccurrence* co = other.cast_ptr<ComplexOccurrence>();
    
    if (co != 0) 
        return m_subroutineCall == co->m_subroutineCall;
    else 
        return false;
}


BasicOccurrenceName::BasicOccurrenceName():m_varDecl(0),m_refExprBracketCount(0)
{
}

//заполняет все поля, анализируя внутреннее представление около ReferenceExpression
BasicOccurrenceName::BasicOccurrenceName(OPS::Reprise::ReferenceExpression& refExpr)
{
    m_varDecl = &refExpr.getReference();
    m_refExprBracketCount = 0;
    OPS::Reprise::RepriseBase* parent = &refExpr;
    OPS::Reprise::RepriseBase* nextParent;
    int structFieldNum = 0;
    bool first = true;
    while (parent)
    {
        nextParent = parent->getParent();
        if (&nextParent->getChild(0) != parent) break; //если мы внутри [], выходим из цикла
        parent = nextParent;

        if (parent->is_a<OPS::Reprise::BasicCallExpression>())
        {
            OPS::Reprise::BasicCallExpression* arrayAccess = parent->cast_ptr<OPS::Reprise::BasicCallExpression>();
            if (arrayAccess->getKind() == OPS::Reprise::BasicCallExpression::BCK_ARRAY_ACCESS) 
            {
                //указываем, что все скобки стоят при имени поля структуры № structFieldNum
                if (first)
                {
                    m_refExprBracketCount = arrayAccess->getChildCount()-1;
                    first = false;
                }
                else
                {
                    m_fields[structFieldNum-1].second += arrayAccess->getChildCount()-1;
                }
                //заполняем m_bracketContent
                int oldSize = m_bracketContent.size();
                m_bracketContent.resize(oldSize + arrayAccess->getChildCount()-1);
                for (int i=0; i < arrayAccess->getChildCount()-1; ++i)
                    m_bracketContent[oldSize+i] = arrayAccess->getChild(i+1).cast_ptr<OPS::Reprise::ExpressionBase>();
            }
        }
        else
        {
            first = false;
            if (parent->is_a<OPS::Reprise::StructAccessExpression>())
            {
                ++structFieldNum;
                OPS::Reprise::StructAccessExpression* structAccess = parent->cast_ptr<OPS::Reprise::StructAccessExpression>();
                //заносим имя поля в m_fields
                m_fields.push_back(std::pair<OPS::Reprise::StructMemberDescriptor*,int>(&structAccess->getMember(),0));
            }
            else
                //текущее выражение не является частью обращения к массиву или полю структуры
                break;
        }
    }
    OPS_ASSERT((int)m_bracketContent.size() == getAllBracketCount());
}

//возвращает суммарное число скобок
int BasicOccurrenceName::getAllBracketCount()
{
    int sum = m_refExprBracketCount;
    for (int i = 0; i < (int)m_fields.size(); ++i)
    {
        sum += m_fields[i].second;
    }
    return sum;
}

bool BasicOccurrenceName::operator ==(const BasicOccurrenceName& other) const
{
	return m_varDecl == other.m_varDecl &&
		   m_refExprBracketCount == other.m_refExprBracketCount &&
		   m_fields == other.m_fields;
}

bool BasicOccurrenceName::operator !=(const BasicOccurrenceName& other) const
{
	return !(*this == other);
}

//создает пустой класс
BasicOccurrence::BasicOccurrence():m_refExpr(0),m_bracketCount(0),m_isAddress(false),m_islValue(false)
{
}

//создает вхождение по заданному ReferenceExpression
BasicOccurrence::BasicOccurrence(OPS::Reprise::ReferenceExpression& refExpr)
{
    m_refExpr = &refExpr;
    m_name = BasicOccurrenceName(refExpr);
    m_bracketCount = m_name.getAllBracketCount();
    //заполняем m_isAddress
    determineIsAddress();
    //определеяем генератор ли это 
    determineIsGenerator();
    //заполняет m_islValue
    determineIslValue();
}

// указатель на переменную вхождения в выражении 
//для массива - указатель на имя, для поля структуры - указатель на всю структуру
OPS::Reprise::ReferenceExpression* BasicOccurrence::getRefExpr() const
{
    return m_refExpr;
}

bool BasicOccurrence::isGenerator() const
{
    return isStatus(GENERATOR);
}

bool BasicOccurrence::isUsage() const
{
    return isStatus(USAGE);
}

bool BasicOccurrence::isEqual(Occurrence& other) const
{
    BasicOccurrence* bo = other.cast_ptr<BasicOccurrence>();
    if (bo != 0)
    {
        return m_refExpr == bo->m_refExpr;
    }
    else 
        return false;
}

// указатель на оператор, которому принадлежит вхождение
OPS::Reprise::StatementBase* BasicOccurrence::getParentStatement() const
{
    return m_refExpr->obtainParentStatement();
}

Reprise::ExpressionBase* BasicOccurrence::getSourceExpression() const
{
	return getHeadNode();
}

// Составное имя вхождения
BasicOccurrenceName BasicOccurrence::getName() const
{
    return m_name;
}

/// количество операций [], использованных в данном вхождении, раньше это неправильно называлось размерностью m_dim
/// для структур - суммарное число []
int BasicOccurrence::getBracketCount() const
{
    return m_bracketCount;
}

/// присутсвует ли перед всем вхождением & пример: &a[i] (заметим, что такого: (&a)[i] быть не может (см. преобразование 2))
bool BasicOccurrence::isAddress() const
{
    return m_isAddress;
}

/// является ли данное вхождение l-value (является ли оно ячейкой памяти, которая имеет адрес, например,
/// для int c[10][10]; вхождения c; &c, c[2]; &c[2]; &c[1][1]; не являются l-value, а c[3][2] является)
bool BasicOccurrence::islValue() const
{
    return m_islValue;
}

Reprise::ReprisePtr<Reprise::StructType> BasicOccurrence::isReturnStruct() const
{
    using namespace Reprise;
    ReprisePtr<TypeBase> headType = getHeadNode()->getResultType();
    StructType* headStruct = Editing::desugarType(*headType).cast_ptr<StructType>();
    return headStruct == 0
            ? ReprisePtr<StructType>()
            : ReprisePtr<StructType>(headStruct);
}

//заполняет m_isAddress
void BasicOccurrence::determineIsAddress()
{
    m_isAddress = false;
    OPS::Reprise::RepriseBase* head = getHeadNode();
    if (head->is_a<OPS::Reprise::BasicCallExpression>())
    {
        OPS::Reprise::BasicCallExpression* basicCallExpr = head->cast_ptr<OPS::Reprise::BasicCallExpression>();
        if (basicCallExpr->getKind() == OPS::Reprise::BasicCallExpression::BCK_TAKE_ADDRESS)
        {
            m_isAddress = true;
        }
    }
}


void BasicOccurrence::determineIsGenerator()
{
    OPS::Reprise::RepriseBase* head = getHeadNode();
    OPS::Reprise::RepriseBase* parent = head->getParent();
    //заполняем USAGE
    OPS_ASSERT(head != 0);
    OPS::Reprise::ExpressionBase& headExpr = head->cast_to<OPS::Reprise::ExpressionBase>();
    OPS::Reprise::ExpressionBase& rootExpr = headExpr.obtainRootExpression();
    if (&rootExpr != &headExpr) addStatus(USAGE);
    if (parent->is_a<Reprise::ReturnStatement>()) addStatus(USAGE);
    //заполняем GENERATOR
    if (parent)
    {
        if (parent->is_a<OPS::Reprise::BasicCallExpression>())
        {
            OPS::Reprise::BasicCallExpression* basicCallExpr = parent->cast_ptr<OPS::Reprise::BasicCallExpression>();
            if (basicCallExpr->getKind() == OPS::Reprise::BasicCallExpression::BCK_ASSIGN)
            {
                if (&basicCallExpr->getChild(0) == head) 
                {
                    addStatus(GENERATOR);
                    if (basicCallExpr == &rootExpr) m_status.unsetStatus(USAGE);
                }
            }
        }
    }
}

void BasicOccurrence::determineIslValue()
{
    m_islValue = false;
    if (m_isAddress)
    {
        m_islValue = false;
        return;
    }
    //get last descriptor
    Reprise::RepriseBase* node;
    Reprise::TypeBase* nodeType;
    int bracketCount;
    if (m_name.m_fields.size() == 0)
    {
        node = m_refExpr;
        nodeType = &m_name.m_varDecl->getType();
        bracketCount = m_bracketCount;
    }
    else
    {
        Reprise::StructMemberDescriptor* smd = m_name.m_fields[m_name.m_fields.size()-1].first;
        node = smd;
        nodeType = &smd->getType();
        bracketCount = m_name.m_fields[m_name.m_fields.size()-1].second;
    }
    nodeType = &Reprise::Editing::desugarType(*nodeType);
    for (int i = 0; i < bracketCount; i++)
    {
        nodeType = nodeType->getChild(0).cast_ptr<Reprise::TypeBase>();
        nodeType = &Reprise::Editing::desugarType(*nodeType);
    }
    m_islValue = !nodeType->is_a<Reprise::ArrayType>();
	OPS_UNUSED(node);
}

/// возвращает самый верхний узел данного вхождения (входит в состав вхождения)
/// например для a[10].b[2] вернет операцию (...)[2]
OPS::Reprise::ExpressionBase* BasicOccurrence::getHeadNode() const
{
    OPS_ASSERT(m_refExpr!=0);
    OPS::Reprise::RepriseBase* previous = m_refExpr;
    OPS::Reprise::RepriseBase* parent = previous->getParent();
    bool flagContinue = true;
    while (parent && flagContinue)
    {
        if (&parent->getChild(0) != previous) break;
        flagContinue = false;
        if (parent->is_a<OPS::Reprise::BasicCallExpression>())
        {
            OPS::Reprise::BasicCallExpression* basicCallExpr = parent->cast_ptr<OPS::Reprise::BasicCallExpression>();
            if ((basicCallExpr->getKind() == OPS::Reprise::BasicCallExpression::BCK_ARRAY_ACCESS) 
                || (basicCallExpr->getKind() == OPS::Reprise::BasicCallExpression::BCK_TAKE_ADDRESS))
                flagContinue = true;
        }
        else
        {
            if (parent->is_a<OPS::Reprise::StructAccessExpression>())
                flagContinue = true;
        }
        if (flagContinue)
        {
            previous = parent;
            parent = parent->getParent();
        }
    }
    Reprise::ExpressionBase* result = previous->cast_ptr<Reprise::ExpressionBase>();
    //проверка
    Reprise::BasicCallExpression* bce = result->cast_ptr<Reprise::BasicCallExpression>();
    if (bce)
    {
        OPS_ASSERT((bce->getKind() == Reprise::BasicCallExpression::BCK_TAKE_ADDRESS) ||
            (bce->getKind() == Reprise::BasicCallExpression::BCK_ARRAY_ACCESS));
    }
    else
    {
        Reprise::ReferenceExpression* re = result->cast_ptr<Reprise::ReferenceExpression>();
        Reprise::StructAccessExpression* sae = result->cast_ptr<Reprise::StructAccessExpression>();
        OPS_ASSERT((re!=NULL)||(sae!=NULL));
    }
    return result;
}

/// возвращает родительский узел данного вхождения
OPS::Reprise::RepriseBase* BasicOccurrence::getParent() const
{
    return getHeadNode()->getParent();
}

std::string BasicOccurrence::toString() const
{
	return getHeadNode()->dumpState();
/*
    std::string res;
    if (m_isAddress) res += "&";
    //добавляем имя структуры
    res += m_name.m_varDecl->getName();
    for (int i = 0; i < m_name.m_refExprBracketCount; ++i) 
    {
        stringstream sss;
        Backends::OutToC writer(sss);

        res += "[]"; //TODO: надо еще линейное выражение внутри скобок печатать (если оно построено)
    }
    //добавляем имена полей
	for (size_t i = 0; i < m_name.m_fields.size(); ++i)
    {
        res += "." + m_name.m_fields[i].first->getName();
        for (int j = 0; j < m_name.m_fields[i].second; ++j) 
        {
            res += "[]"; //TODO: надо еще линейное выражение внутри скобок печатать (если оно построено)
        }
    }
    return res;
    */
}

//для проверки статусов
bool BasicOccurrence::isStatus(SafeStatus::status_t s) const
{
    return m_status.isStatus(s);
}

void BasicOccurrence::addStatus(SafeStatus::status_t s)
{
    m_status.addStatus(s);
}

}//end of namespace
}//end of namespace
