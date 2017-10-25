#include "Analysis/Montego/AliasAnalysis/MemoryCell.h"
#include "MemoryCellContainer.h"
#include <string>
#include <iostream>
#include "Analysis/Montego/AliasAnalysis/SetAbstractMemoryCell2.h"
#include "Reprise/Service/DeepWalker.h"
#include <set>
#include "Shared/ExpressionHelpers.h"
using namespace OPS::Shared::ExpressionHelpers;

using namespace std;
using namespace OPS::Reprise;


namespace OPS
{
namespace Montego
{

bool getInteger(RepriseBase* expr, long long int& res);

MemoryCell::MemoryCell():m_memoryAllotment(0), m_anotherAllotmentIndex(0), m_zeroOffset(0), m_unknownOffset(0)
{

}

MemoryCell::MemoryCell(OPS::Reprise::RepriseBase* memoryAllotment, bool isReadOnly, const FunctionContext* functionContext):
    m_memoryAllotment(memoryAllotment), m_functionContext(functionContext), m_isReadOnly(isReadOnly), m_anotherAllotmentIndex(0),
    m_zeroOffset(0), m_unknownOffset(0)
{

}

const FunctionContext* MemoryCell::getFunctionContext() const
{
    return m_functionContext;
}

OPS::Reprise::RepriseBase* MemoryCell::getMemoryAllotment()
{
    return m_memoryAllotment;
}

//только для статических массивов, имя массива - read only
bool MemoryCell::isReadOnly()
{
    return m_isReadOnly;
}

OPS::Reprise::TypeBase* MemoryCell::getType()
{
    if (m_memoryAllotment == 0) return 0;

    if (m_memoryAllotment->is_a<OPS::Reprise::VariableDeclaration>())
    {
        OPS::Reprise::VariableDeclaration& varDecl = m_memoryAllotment->cast_to<OPS::Reprise::VariableDeclaration>();
        return & varDecl.getType();
    }
    else
    {
        if (m_memoryAllotment->is_a<OPS::Reprise::SubroutineDeclaration>())
        {
            OPS::Reprise::SubroutineDeclaration& subrDecl = m_memoryAllotment->cast_to<OPS::Reprise::SubroutineDeclaration>();
            return & subrDecl.getType();
        }
        else
        {
            if (m_memoryAllotment->is_a<OPS::Reprise::SubroutineCallExpression>())
            {
                OPS::Reprise::SubroutineCallExpression& expr = m_memoryAllotment->cast_to<OPS::Reprise::SubroutineCallExpression>();
                //что за тип нужно возвращать - неизвестно!
                OPS_ASSERT(!"Еще не реализовано! Непонятно что делать!");
				OPS_UNUSED(expr);
                return 0;
            }
            else
            {
                throw OPS::RuntimeError("Недопустимый тип memoryAllotment в функции MemoryCell::getType()");
                return 0;
            }
        }
    }
}

OPS::Reprise::ReprisePtr<OPS::Reprise::ExpressionBase> MemoryCell::getSize()
{

    if (m_memoryAllotment->is_a<OPS::Reprise::VariableDeclaration>())
    {
        OPS::Reprise::VariableDeclaration& varDecl = m_memoryAllotment->cast_to<OPS::Reprise::VariableDeclaration>();
        return ReprisePtr<ExpressionBase>(new BasicCallExpression(BasicCallExpression::BCK_SIZE_OF,
                                                           new ReferenceExpression(varDecl)));
    }
    else
    {
        if (m_memoryAllotment->is_a<OPS::Reprise::SubroutineDeclaration>())
        {
            throw OPS::RuntimeError("Попытка посчитать размер объявления функции");
        }
        else
        {
            if (m_memoryAllotment->is_a<OPS::Reprise::SubroutineCallExpression>())
            {
                OPS::Reprise::SubroutineCallExpression& expr = m_memoryAllotment->cast_to<OPS::Reprise::SubroutineCallExpression>();
                string mallocName = expr.getCallExpression().cast_to<SubroutineReferenceExpression>().getReference().getName();
                if (mallocName != "polybench_alloc_data")
                    return ReprisePtr<ExpressionBase>(expr.getArgument(0).clone());
                else
                {
                    ReprisePtr<ExpressionBase> count = ReprisePtr<ExpressionBase>(expr.getArgument(0).clone());
                    ReprisePtr<ExpressionBase> oneSz = ReprisePtr<ExpressionBase>(expr.getArgument(1).clone());
                    return ReprisePtr<ExpressionBase>(&( op(count)*op(oneSz) ));
                }
            }
            else
            {
                throw OPS::RuntimeError("Недопустимый тип memoryAllotment в функции MemoryCell::getType()");
                return ReprisePtr<ExpressionBase>();
            }
        }
    }
}

std::string MemoryCell::toString()
{
    if (m_anotherAllotmentIndex > 0) return "external" + OPS::Strings::format("%d",m_anotherAllotmentIndex);
    if (m_memoryAllotment == 0) return "(Error. MemoryCell allotment = NULL)";

    if (m_memoryAllotment->is_a<OPS::Reprise::VariableDeclaration>())
    {
        OPS::Reprise::VariableDeclaration& varDecl = m_memoryAllotment->cast_to<OPS::Reprise::VariableDeclaration>();
        if (isReadOnly())
            return "read only " + varDecl.getName();
        else
            return varDecl.getName();
    }
    else
    {
        if (m_memoryAllotment->is_a<OPS::Reprise::SubroutineDeclaration>())
        {
            OPS::Reprise::SubroutineDeclaration& subrDecl = m_memoryAllotment->cast_to<OPS::Reprise::SubroutineDeclaration>();
            return "subroutine " + subrDecl.getName();
        }
        else
        {
            if (m_memoryAllotment->is_a<OPS::Reprise::SubroutineCallExpression>())
            {
                OPS_ASSERT(m_memoryAllotment->getChild(0).is_a<OPS::Reprise::SubroutineReferenceExpression>());
                return m_memoryAllotment->dumpState();
            }
            else
            {
                throw OPS::RuntimeError("Недопустимый тип memoryAllotment в функции MemoryCell::toString()");
                return 0;
            }
        }
    }
}

std::string MemoryCell::toStringWithOffsetsAndTheirIntersections()
{
    string res = "";
    res += "MemoryCell: " + toString() + "\n";
    for (list<MemoryCellOffset>::iterator it = m_offsets.begin(); it != m_offsets.end(); ++it)
    {
        MemoryCellOffset mo = *it;
        res += "Offset: " + mo.toStringOffsetOnly() + "  intersects with: ";
        for (set<MemoryCellOffset*>::iterator it2 = mo.m_intersectedOffsets.begin(); it2 != mo.m_intersectedOffsets.end(); ++it2)
        {
            if ((mo.toStringOffsetOnly() == "[10]") && ((*it2)->toStringOffsetOnly() == "[11]"))
                cout << "!!!";
            int t = MemoryCellOffset::getIntersectType(&mo, *it2);
            res += (*it2)->toStringOffsetOnly() + "(t=" + OPS::Strings::format("%d",t)  + "); ";
        }
        res += "\n";
    }
    return res;
}

MemoryCell *MemoryCell::createAnotherAllotment(MemoryCellContainer* cont)
{
    MemoryCell m;
    m.m_anotherAllotmentIndex = ++(cont->m_lastAnotherAllotmentIndex);
    cont->m_container.push_back(m);
    return &cont->m_container.back();
}

MemoryCell* MemoryCell::getCellPtr()
{
    return this;
}

MemoryCellOffset* MemoryCell::getZeroOffset()
{
    if (m_zeroOffset)
        return m_zeroOffset;
    else
    {
        MemoryCellOffset off(this, false);
        return insertOffsetIntoContainer(off);
    }
}

MemoryCellOffset* MemoryCell::getUnknownOffset()
{
    if (m_unknownOffset)
        return m_unknownOffset;
    else
    {
        MemoryCellOffset off(this, true);
        return insertOffsetIntoContainer(off);
    }
}

//добавляет смещение и наводит связи, возвращает указатель на добавленное смещение
//если такое смещение уже было в контейнере, то добавления не происходит и возвращается указатель на существующее смещение
MemoryCellOffset* MemoryCell::insertOffsetIntoContainer(MemoryCellOffset &off)
{
    //проверяем наличие равных смещений
    off.m_memoryCell = this;
    std::list<MemoryCellOffset>::iterator it = m_offsets.begin();
    for ( ; it != m_offsets.end(); ++it)
        if (MemoryCellOffset::getIntersectType(&off, &*it) == 3) break;
    if (it != m_offsets.end()) //нашли равное смещение
        return &*it;
    //если добавляемое смещение - новое
    m_offsets.push_back(off);
    updateIntersections(&m_offsets.back());
    if (off.m_offset.size() == 0)        //это нулевое или неизвестное смещение
    {
        if (off.m_isUnknownOffset)  m_unknownOffset = &m_offsets.back();
        else m_zeroOffset = &m_offsets.back();
    }
    return &m_offsets.back();
}

//обновляет связи при добавлении нового смещения (смещение должно быть уже добавлено!!!)
void MemoryCell::updateIntersections(MemoryCellOffset* off)
{
    OPS_ASSERT(off->getCellPtr() == this);
    off->updateIntersections();
    std::set<MemoryCellOffset*>::iterator it = off->m_intersectedOffsets.begin();
    for ( ; it != off->m_intersectedOffsets.end(); ++it)
    {
        MemoryCellOffset* intersectedOffset = *it;
        intersectedOffset->m_intersectedOffsets.insert(off);
    }
}

class OffsetSearchVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    OffsetSearchVisitor(size_t maxSize, MemoryCell* m){m_hasUnion = false; m_hasArrayOfUnknownSize = false; m_maxSize = maxSize; m_m = m;}

    void addCurrentOffset()
    {
        if (m_buildedOffsets.size() >= m_maxSize) return;
        MemoryCellOffset o(m_m, m_currentOffsetBase);
        m_buildedOffsets.push_back(m_m->insertOffsetIntoContainer(o));
    }
    void visit(StructType& t)
    {
        if (t.isUnion()) {m_hasUnion = true; return;}
        for (int i = 0; i < t.getMemberCount(); i++)
        {
            MemoryCellOffset::ElementaryOffset eo(&t.getMember(i));
            m_currentOffsetBase.push_back(eo);
            t.getMember(i).getType().accept(*this);
            m_currentOffsetBase.pop_back();
            if (m_buildedOffsets.size() >= m_maxSize) return;
        }
    }
    void visit(ArrayType& t)
    {
        if (m_buildedOffsets.size() >= m_maxSize) return;
        long long int n;
        if (!t.hasCountExpression())
        {
            n = t.getElementCount();
            OPS_ASSERT(n>0);
            for (int i = 0; i < n; i++)
            {
                MemoryCellOffset::ElementaryOffset eo(ReprisePtr<TypeBase>(&t.getBaseType()), i, false);
                m_currentOffsetBase.push_back(eo);
                t.getBaseType().accept(*this);
                m_currentOffsetBase.pop_back();
                if (m_buildedOffsets.size() >= m_maxSize) return;
            }
        }
    }
    void visit(BasicType& ) {addCurrentOffset();}
    void visit(PtrType& ) {addCurrentOffset();}
    void visit(EnumType& ) {addCurrentOffset();}
    void visit(SubroutineType& ) {addCurrentOffset();}
    void visit(DeclaredType& d){d.getDeclaration().accept(*this);}

    std::list<MemoryCellOffset::ElementaryOffset> m_currentOffsetBase;
    std::list<MemoryCellOffset*> m_buildedOffsets;
    bool m_hasUnion;
    bool m_hasArrayOfUnknownSize;
    size_t m_maxSize;
    MemoryCell* m_m;
};

std::vector<MemoryCellOffset*> MemoryCell::getEveryPossibleOffsetSequence(size_t minSize)
{
    std::vector<MemoryCellOffset*> result;
    OPS_ASSERT(minSize > 0);
    VariableDeclaration* v = m_memoryAllotment->cast_ptr<VariableDeclaration>();
    if (v == 0) throw RuntimeError("Trying to get EveryPossibleOffsetSequence of not a VariableDeclaration");
    OffsetSearchVisitor visitor(minSize, this);
    v->getType().accept(visitor);
    if (visitor.m_hasUnion || visitor.m_hasArrayOfUnknownSize)
    {
        for (size_t i = 0; i < minSize; i++)
            result.push_back(getUnknownOffset());
        return result;
    }
    if (visitor.m_buildedOffsets.size() == 0)
    {
        cout << "visitor.m_buildedOffsets.size() == 0 of variable " << v->dumpState() << "\n" << "maxSize = " << minSize <<"\n";
        OffsetSearchVisitor visitor(minSize, this);
        v->getType().accept(visitor);
    }
    OPS_ASSERT(visitor.m_buildedOffsets.size() != 0);

    //проверка на совпадение смещений
    std::set<MemoryCellOffset*> checkSet;
    checkSet.insert(visitor.m_buildedOffsets.begin(), visitor.m_buildedOffsets.end());
    OPS_ASSERT(checkSet.size() == visitor.m_buildedOffsets.size());

    result.insert(result.end(), visitor.m_buildedOffsets.begin(), visitor.m_buildedOffsets.end());
    return result;
}

SetAbstractMemoryCell MemoryCell::getOffsets(MemoryCellContainer& cont)
{
    std::set<MemoryCellOffset*> setoff;
    for (std::list<MemoryCellOffset>::iterator it = m_offsets.begin(); it != m_offsets.end(); ++it)
        setoff.insert(&*it);
    SetAbstractMemoryCell res(cont, setoff);
    return res;
}

int MemoryCell::compare(MemoryCell &a, MemoryCell &b)
{
    if (a.m_memoryAllotment == b.m_memoryAllotment
            && a.m_isReadOnly == b.m_isReadOnly
            && a.getFunctionContext() == b.getFunctionContext())
        return 1;
    else
        return 0;
}

MemoryCellOffset::MemoryCellOffset() : m_memoryCell(0), m_isUnknownOffset(false)
{

}

MemoryCellOffset::MemoryCellOffset(MemoryCell* memoryCell, bool isUnknownOffset)
            : m_memoryCell(memoryCell), m_isUnknownOffset(isUnknownOffset)
{

}

MemoryCellOffset::MemoryCellOffset(MemoryCell* memoryCell, std::vector<ElementaryOffset>& offset)
	: m_memoryCell(memoryCell), m_offset(offset), m_isUnknownOffset(false)
{
    simplifyOffset();
}

MemoryCellOffset::MemoryCellOffset(MemoryCell* memoryCell, std::list<ElementaryOffset>& offset)
: m_memoryCell(memoryCell), m_isUnknownOffset(false)
{
    vector<ElementaryOffset> off(offset.begin(), offset.end());
    m_offset = off;
    simplifyOffset();
}


MemoryCell* MemoryCellOffset::getCellPtr()
{
    return m_memoryCell;
}

std::string MemoryCellOffset::toString()
{
    string res = m_memoryCell->toString();
    return res + toStringOffsetOnly();
}

std::string MemoryCellOffset::toStringOffsetOnly()
{
    string res;
    if (m_isUnknownOffset) return res+"+???";
    if (m_offset.empty()) return res+"+0";
    for (size_t i = 0; i < m_offset.size(); i++)
    {
        ElementaryOffset& eo = m_offset[i];
        res += eo.toString();
    }
    return res;
}

bool getInteger(RepriseBase* expr, long long int& res)
{
    Reprise::StrictLiteralExpression* sle;
    int sign = 1;
    if (expr->is_a<BasicCallExpression>() && expr->cast_to<BasicCallExpression>().getKind() == BasicCallExpression::BCK_UNARY_MINUS)
    {
        sle = expr->getChild(0).cast_ptr<Reprise::StrictLiteralExpression>();
        sign = -1;
    }
    else
        sle = expr->cast_ptr<Reprise::StrictLiteralExpression>();
    bool isInteger = sle && sle->isInteger();
    if (isInteger)
    {
        switch (sle->getLiteralType())
        {
        case BasicType::BT_INT8 : res = sle->getInt8(); break;
        case BasicType::BT_UINT8 : res = sle->getUInt8(); break;
        case BasicType::BT_INT16 : res = sle->getInt16(); break;
        case BasicType::BT_UINT16 : res = sle->getUInt16(); break;
        case BasicType::BT_INT32 : res = sle->getInt32(); break;
        case BasicType::BT_UINT32 : res = sle->getUInt32(); break;
        case BasicType::BT_INT64 : res = sle->getInt64(); break;
        case BasicType::BT_UINT64 : res = sle->getUInt64(); break;
        default:
            OPS_ASSERT(false);
        }
        res *= sign;
    }
    return isInteger;
}

MemoryCellOffset::ElementaryOffset MemoryCellOffset::buildElemOffset(Reprise::ExpressionBase& e, int currentBracketNumber)
{
    if (Reprise::StructAccessExpression* sae = e.cast_ptr<Reprise::StructAccessExpression>())
    {
        OPS_ASSERT(currentBracketNumber == 0);
        MemoryCellOffset::ElementaryOffset offset(&sae->getMember());
        return offset;
    }
    else
    {
        Reprise::BasicCallExpression* bce = e.cast_ptr<Reprise::BasicCallExpression>();
        OPS_ASSERT(bce != 0);
        RepriseBase& ch0 = bce->getChild(0);
        TypeBase* t;
        if (ch0.is_a<ReferenceExpression>())
            t = &ch0.cast_to<ReferenceExpression>().getReference().getType();
        else
            if (ch0.is_a<StructAccessExpression>())
                t = &ch0.cast_to<StructAccessExpression>().getMember().getType();
            else
                throw RuntimeError("Zero child of [] is not occurrence. Do canonical transformations first.");
        t = &Editing::desugarType(*t);
        OPS_ASSERT(t->is_a<ArrayType>() || t->is_a<PtrType>());

        long long int arrayIndex = 0;
        OPS_ASSERT(currentBracketNumber<bce->getChildCount()-1);
        RepriseBase* arg = &bce->getChild(1+currentBracketNumber);
        bool isInteger = getInteger(arg, arrayIndex);
        for (int i = 0; i <= currentBracketNumber; i++)
            t = &Reprise::Editing::desugarType(t->getChild(0).cast_to<Reprise::TypeBase>());
        MemoryCellOffset::ElementaryOffset offset(ReprisePtr<TypeBase>(t), arrayIndex, !isInteger);
        return offset;
    }
}


bool hasChildType(TypeBase* parent, TypeBase* child)
{
    TypeBase* t = &Editing::desugarType(*parent);
    TypeBase* t2 = &Editing::desugarType(*child);
    while (t->is_a<ArrayType>() && !t->isEqual(*t2))
    {
        ArrayType* at = t->cast_ptr<ArrayType>();
        t = &Editing::desugarType(at->getBaseType());
    }
    return t->isEqual(*t2);
}

//добавляет к текущему указанное смещение addition, сохраняет новое смещение в контейнере и возвращает его адрес
MemoryCellOffset* MemoryCellOffset::addAndInsertOffsetIntoContainer(
        std::list<MemoryCellOffset::ElementaryOffset>& addition)
{
    MemoryCellOffset newOffset = *this;

    //check, whether we can add
    if (m_offset.size()>0 && addition.size()>0)
    {
        ElementaryOffset& eo = m_offset[m_offset.size()-1];
        ElementaryOffset& add = addition.front();
        if (!eo.isArrayOffset() && !eo.getType()->is_a<ArrayType>() && add.isArrayOffset() && ( add.isArrayIndexUnknown() || add.getArrayIndex() != 0 ) )
        {
            //последнее смещение в m_offset указывает на уединенное поле структуры, а мы пытаемся к нему добавить арифметику
            //делаем смещение неопределенным
            return m_memoryCell->getUnknownOffset();
        }
        if (!hasChildType(eo.getType(),   add.isArrayOffset() ? add.getType() : add.getStructField()->getParent()->cast_ptr<TypeBase>()))
            //добавляем смещение на число элементов типа, не являющегося child-ом от уже добавленного
            return m_memoryCell->getUnknownOffset();

        if (eo.isArrayOffset() && add.isArrayOffset() && ( add.isArrayIndexUnknown() || add.getArrayIndex() != 0 ) && !eo.getType()->isEqual(*add.getType()))
        {
            if (!eo.getType()->is_a<ArrayType>() || !eo.getType()->getChild(0).cast_to<TypeBase>().isEqual(*add.getType()))
            {
                //добавляем смещение на число элементов непонятного типа, не равного и не являющегося потомком последнего в m_offset
                //делаем смещение неопределенным
                /*cout << "While adding ";
                for (list<MemoryCellOffset::ElementaryOffset>::iterator it = addition.begin(); it != addition.end(); ++it)
                    cout << it->toString();
                cout << " make offset " << toString() << " unknown, because:\n";
                cout << "Type of m_offset[end] = " << eo.getType()->dumpState() << endl;
                cout << "Type of add[0] = " << add.getType()->dumpState() << endl;
                */
                return m_memoryCell->getUnknownOffset();
            }
        }
    }
    newOffset.m_offset.insert(newOffset.m_offset.end(), addition.begin(), addition.end());
    /*cout << "Offset before simplification: " << newOffset.toString() << endl;
    if (newOffset.toString() == "STR2.s[2][4][1]")
    {
        cout << "Type of newOffset.m_offset[2] = " << newOffset.m_offset[2].getType()->dumpState() << endl;
        cout << "Type of newOffset.m_offset[3] = " << newOffset.m_offset[3].getType()->dumpState() << endl;
    }*/
    newOffset.simplifyOffset();
    //cout << "Offset after simplification: " << newOffset.toString() << endl;

    //проверка    
    newOffset.checkTypes();
    checkTypes();

    if (newOffset.m_offset.size()>0)
    {
        ElementaryOffset &a = newOffset.m_offset[0];
        if (a.isArrayOffset() && !a.isArrayIndexUnknown())
            if (a.getArrayIndex() == 0)
            {
                newOffset.simplifyOffset();
                OPS_ASSERT(false);
            }
    }
    return m_memoryCell->insertOffsetIntoContainer(newOffset);
}


void MemoryCellOffset::simplifyOffset()
{
    std::vector<ElementaryOffset>::iterator it, next;
    int j = 0;
    if (m_offset.size() <= 1) goto removeZeroOffsets;
    //складываем подобные
    it = m_offset.begin();
    while (j <= ((int)m_offset.size())-2)
    {
        next = it; next++;
        ElementaryOffset &a = *it, &b = *next;
        if (a.isArrayOffset() && b.isArrayOffset() && a.getArrayElementType()->isEqual(*b.getArrayElementType()) )
        {
            if (!a.isArrayIndexUnknown() && !b.isArrayIndexUnknown())
                a.setArrayIndex(a.getArrayIndex() + b.getArrayIndex());
            else
            {
                a.m_isArrayIndexUnknown = true;
                a.m_arrayIndex = 0;
            }
            m_offset.erase(next);
        }
        if (j <= ((int)m_offset.size())-2) {++it; ++j;}
    }
    //удаляем нулевые
removeZeroOffsets:
    it = m_offset.begin();
    for (int i = 0; i < (int)m_offset.size(); ++i)
    {
        ElementaryOffset &a = m_offset[i];
        if (a.isArrayOffset() && !a.isArrayIndexUnknown())
        {
            if (a.getArrayIndex() == 0)  {it = m_offset.erase(it); i--;}
            else ++it;
        }
        else ++it;
    }
}

//удаляет старые и наводит новые связи, ссылки на данную ячейку не обновляются!!!!!
void MemoryCellOffset::updateIntersections()
{
    m_intersectedOffsets.clear();
    std::list<MemoryCellOffset>& otherOffsets = m_memoryCell->m_offsets;
    std::list<MemoryCellOffset>::iterator it = otherOffsets.begin();
    for ( ; it != otherOffsets.end(); ++it)
    {
        if (&*it == this) continue;
        int t = getIntersectType(this, &*it);
        OPS_ASSERT(t != 3);//равных быть не должно!
        if (t != 0) m_intersectedOffsets.insert(&*it);
    }
}

bool isOfSameStruct(StructMemberDescriptor* m1, StructMemberDescriptor* m2)
{
    Reprise::StructType* sta = m1->getParent()->cast_ptr<Reprise::StructType>();
    Reprise::StructType* stb = m2->getParent()->cast_ptr<Reprise::StructType>();
    if (sta == stb) return true;
    return sta->isEqual(*stb);
}

enum NextCompareSegmentType {
    TwoArrayOffsetsOfOneType = 0,
    TwoStructMembersOfOneStruct,
    FirstIsZero,//первое смещение содержит [0], дальше у смещений еще будет совпадение
    SecondIsZero,//второе смещение содержит [0], дальше у смещений еще будет совпадение
    NoMoreEqualTypes, //дальше у смещений идут дескрипторы разных структур и перед ними нет совпадающих по типу []
    theEnd //просмотрели все смещения, нужно выходить из цикла
};

//сравнивает сегменты под курсорами и
//смещает курсоры i1, i2 к следующей позиции
//для FirstIsZero и SecondIsZero курсоры устанавливаются на совпадение
//для NoMoreEqualTypes положение курсоров не меняется
NextCompareSegmentType getNextSegmentsCompareType(vector<MemoryCellOffset::ElementaryOffset>& a, vector<MemoryCellOffset::ElementaryOffset>& b, size_t& i1, size_t& i2)
{
    //обрабатываем случай, когда в одном из смещений уже дошли до конца
    if ( i1 >= a.size() && i2 >= b.size() )  return theEnd;
    if ( i1 >= a.size() || i2 >= b.size() )
    {
        vector<MemoryCellOffset::ElementaryOffset>* vec = i1 >= a.size() ? &b : &a; //указатель не не до конца пройденное смещение
        size_t* i = i1 >= a.size() ? &i2 : &i1;
        MemoryCellOffset::ElementaryOffset e = (*vec)[*i];
        //считаем, что у пройденного смещения удалено несколько нулей в конце
        if (e.isArrayOffset()) {  NextCompareSegmentType result = i1 >= a.size() ? FirstIsZero : SecondIsZero; (*i)++; return result; }
        else return theEnd;
    }
    OPS_ASSERT((i1 < a.size()) && (i2 < b.size()));
    MemoryCellOffset::ElementaryOffset e1 = a[i1], e2 = b[i2];
    if (e1.isArrayOffset() && e2.isArrayOffset() && e1.getArrayElementType()->isEqual(*e2.getArrayElementType()) )
    {
        i1++;  i2++;
        return TwoArrayOffsetsOfOneType;
    }
    if (!e1.isArrayOffset() && !e2.isArrayOffset())
    {
        if ( isOfSameStruct(e1.getStructField(), e2.getStructField()) )
        {
            i1++;  i2++;
            return TwoStructMembersOfOneStruct;
        }
        else return NoMoreEqualTypes;
    }
    //пытаемся найти совпадение
    TypeBase *t1, *t2, *t01, *t02;
    //случай, когда оба смещения - массивы
    if (e1.isArrayOffset() && e2.isArrayOffset() )
    {
        OPS_ASSERT(!e1.getArrayElementType()->isEqual(*e2.getArrayElementType()));
        //алгоритм должен работать даже в ситуации: s[0][i].p и s[j][0].p
        t01 = t1 = e1.getArrayElementType();
        t02 = t2 = e2.getArrayElementType();

        //проверяем пропущены ли нули в b
        while (t1->is_a<ArrayType>() && !t1->isEqual(*t02) )
            t1 = & Editing::desugarType( t1->cast_to<ArrayType>().getBaseType() );
        if ( t1->isEqual(*t02) ) {i2++; return SecondIsZero;}

        //проверяем пропущены ли нули в a
        while (t2->is_a<ArrayType>() && !t2->isEqual(*t01) )
            t2 = & Editing::desugarType( t2->cast_to<ArrayType>().getBaseType() );
        if ( t2->isEqual(*t01) ) {i1++; return FirstIsZero;}

        return NoMoreEqualTypes;
    }
    //случай, когда одно - смещение в массиве, а другое - поле структуры
    //делаем так, чтобы e1 было смещением в массиве
    vector<MemoryCellOffset::ElementaryOffset> *pa, *pb;
    MemoryCellOffset::ElementaryOffset *pe1, *pe2;
    bool isSwap = !e1.isArrayOffset();
    if (!isSwap)
    {
        pa = &a; pb = &b;
        pe1 = &e1; pe2 = &e2;
        OPS_ASSERT(!e2.isArrayOffset());
    }
    else
    {
        pa = &b; pb = &a;
        pe1 = &e2; pe2 = &e1;
        OPS_ASSERT(e2.isArrayOffset());
    }
    t1 = & Editing::desugarType( *pe1->getArrayElementType() );
    t2 = & Editing::desugarType( pe2->getStructField()->getParent()->cast_to<TypeBase>() );
    OPS_ASSERT((t1 != 0) && (t2 != 0));
    while (t1->is_a<ArrayType>() && !t1->isEqual(*t2) )
        t1 = & Editing::desugarType( t1->cast_to<ArrayType>().getBaseType() );
    if ( t1->isEqual(*t2) )
    {
        if (!isSwap)  {i1++;   return SecondIsZero; }
        else          {i2++;   return FirstIsZero;  }
    }
    return NoMoreEqualTypes;
}

//возвращает 0 - точно не пересекаются, 1 - a<b, 2 - a>b, 3 - a=b, 4 - могут пересекаться, а могут и нет
int MemoryCellOffset::getIntersectType(MemoryCellOffset *a, MemoryCellOffset *b)
{
    OPS_ASSERT(a->getCellPtr() == b->getCellPtr());
    if (a->m_isUnknownOffset && b->m_isUnknownOffset) return 3;
    if (a->m_isUnknownOffset || b->m_isUnknownOffset) return 4;
    if (a->m_offset.size() == 0 && b->m_offset.size() == 0) return 3;
    size_t i1 = 0, i2 = 0; //указатели на текущие рассматриваемые элементарные смещения
    size_t oldi1, oldi2;
    bool flagCanCurrentBeNotEqual = false;//может ли рассмотренная часть смещений указывать на разные байты
    NextCompareSegmentType compareType = TwoArrayOffsetsOfOneType;
    while ( compareType != theEnd )
    {
        oldi1 = i1;  oldi2 = i2;
        compareType = getNextSegmentsCompareType(a->m_offset, b->m_offset, i1, i2);
        int c;
        switch (compareType)
        {
        case TwoArrayOffsetsOfOneType:
        {
            c = ElementaryOffset::compare(a->m_offset[oldi1], b->m_offset[oldi2]);
            if (c == 0) return 0; //точно не пересекаются
            if (c == 2) flagCanCurrentBeNotEqual = true;
            OPS_ASSERT(c != 3);
            break;
        }
        case TwoStructMembersOfOneStruct:
        {
            c = ElementaryOffset::compare(a->m_offset[oldi1], b->m_offset[oldi2]);
            if (c == 0) return 0; //точно не пересекаются
            OPS_ASSERT(c != 2);
            if (c == 3) return 4; //fields of one union
            break;
        }
        case FirstIsZero:
        {
            //во втором массиве было пропущено несколько ненулевых смещений
            //если они все имеют неопределенные индексные выражения, то flagCanCurrentBeNotEqual = true;
            //если существует хотя бы одно определенное, то return 0
            for (size_t i = oldi2; i < i2; i++)
            {
                MemoryCellOffset::ElementaryOffset& e = b->m_offset[i];
                if (!e.isArrayOffset())
                    cout << "oldi1="<<oldi1<<" oldi2="<<oldi2 << " i1="<<i1<<" i2="<<i2 << " e = " << e.toString() << "\na = " << a->toString() << "   b = " << b->toString() << "\n";
                OPS_ASSERT(e.isArrayOffset());
                if (!e.isArrayIndexUnknown()) return 0;
            }
            flagCanCurrentBeNotEqual = true;
            break;
        }
        case SecondIsZero:
        {
            //в первом массиве было пропущено несколько ненулевых смещений
            //если они все имеют неопределенные индексные выражения, то flagCanCurrentBeNotEqual = true;
            //если существует хотя бы одно определенное, то return 0
            for (size_t i = oldi1; i < i1; i++)
            {
                MemoryCellOffset::ElementaryOffset& e = a->m_offset[i];
                OPS_ASSERT(e.isArrayOffset());
                if (!e.isArrayIndexUnknown()) return 0;
            }
            flagCanCurrentBeNotEqual = true;
            break;
        }
        case NoMoreEqualTypes:
        {
            return 4;
            break;
        }
        case theEnd: break;
        default: OPS_ASSERT(false);
        }
    }
    //общие части вхождений совпадают
    if ((i1 == a->m_offset.size()) && (i2 == b->m_offset.size()) )
    {
        if (flagCanCurrentBeNotEqual) return 4;
        else return 3;
    }
    else
        if (i1 < a->m_offset.size()) //i2 дошло до конца, i1 - может еще уточняться
        {
            OPS_ASSERT(i2 == b->m_offset.size());
            ElementaryOffset e = a->m_offset[i1];
            if ( !e.isArrayOffset() ) return 1;
            else return 0;
        }
        else
        {
            OPS_ASSERT(i1 == a->m_offset.size());
            OPS_ASSERT(i2 < b->m_offset.size());
            ElementaryOffset e = b->m_offset[i2];
            if ( !e.isArrayOffset() ) return 2;
            else return 0;
        }
}

MemoryCellOffset::OffsetAdditionMap MemoryCellOffset::getInnerOffsetsForStruct(Reprise::StructType& st)
{
    //находим все смещения, которые являются подмножеством данного и содержат после данного
    //одно из полей указанной структуры
    OffsetAdditionMap res;
    std::list<MemoryCellOffset>& offsets = getCellPtr()->m_offsets;
    std::list<MemoryCellOffset>::iterator it;
    for (it = offsets.begin(); it != offsets.end(); ++it)
    {
        MemoryCellOffset* other = &*it;
        int t = MemoryCellOffset::getIntersectType(this, other);
        if (t == 2)
        {
            std::list<MemoryCellOffset::ElementaryOffset> addition;
            std::vector<MemoryCellOffset::ElementaryOffset>::reverse_iterator ite = other->m_offset.rbegin();
            for (int i = 0; i < other->m_offset.size() - m_offset.size(); i++, ++ite) addition.push_front(*ite);
            res[&*it] = addition;
        }
    }
    return res;
}

std::set<MemoryCellOffset*>& MemoryCellOffset::getIntersectedOffsets()
{
    return m_intersectedOffsets;
}

MemoryCellOffset* MemoryCellOffset::addConst(long long int addition,  Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType)
{
    //addition - is arithmetic add, for example: &a[5][0] + 3  or  &s.x + 5
    ElementaryOffset e(arrayElementType, addition, false);
    list<ElementaryOffset> toAdd;
    toAdd.push_back(e);
    return addAndInsertOffsetIntoContainer(toAdd);
}

MemoryCellOffset* MemoryCellOffset::undefineLastOffset(Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType)
{
    ElementaryOffset e(arrayElementType, 0, true);
    list<ElementaryOffset> toAdd;
    toAdd.push_back(e);
    MemoryCellOffset* res = addAndInsertOffsetIntoContainer(toAdd);
    OPS_ASSERT(res != 0);
    if (res == 0)  return m_memoryCell->getUnknownOffset();
    return res;
}

void MemoryCellOffset::checkTypes()
{
    if (m_offset.size() <= 1) return;
    TypeBase *currentType,
            *prevType = &Editing::desugarType(*m_offset[0].getType());
    for (size_t i = 1; i < m_offset.size(); i++)
    {
        ElementaryOffset& e = m_offset[i];
        ElementaryOffset& prev_e = m_offset[i-1];
        currentType = &Editing::desugarType(*e.getType());

        bool has = hasChildType(prevType,   e.isArrayOffset() ? currentType : e.getStructField()->getParent()->cast_ptr<TypeBase>());
        if (!has)
        {
            std::cout << "CheckTypes found error in offset: " << toString() << "\n";
            std::cout << "Previous offset = " << prev_e.toString() << "   Current = " << e.toString() << "\n";
            std::cout << "PrevType = " << prevType->dumpState() << "  currentType = " << currentType->dumpState() << "\n";
        }
        OPS_ASSERT(has);
        prevType = currentType;
    }
}

MemoryCellOffset::ElementaryOffset::ElementaryOffset()
    :m_isArrayOffset(true)
    ,m_isArrayIndexUnknown(false)
    ,m_structField(0)
    ,m_arrayIndex(0)
    ,m_arrayElementType(0)
{

}

MemoryCellOffset::ElementaryOffset::ElementaryOffset(Reprise::ReprisePtr<TypeBase> arrayElementType, long long arrayIndex, bool isArrayIndexUnknown)
    :m_isArrayOffset(true)
    ,m_isArrayIndexUnknown(isArrayIndexUnknown)
    ,m_structField(0)
    ,m_arrayIndex(arrayIndex)
{
    if (arrayElementType.get() != 0)
        m_arrayElementType = ReprisePtr<TypeBase>( & Editing::desugarType(*arrayElementType) );
    else
        m_arrayElementType = ReprisePtr<TypeBase>(BasicType::charType());
}

MemoryCellOffset::ElementaryOffset::ElementaryOffset(Reprise::StructMemberDescriptor* structField)
    :m_isArrayOffset(false)
    ,m_isArrayIndexUnknown(false)
    ,m_structField(structField)
    ,m_arrayIndex(0)
{

}

bool MemoryCellOffset::ElementaryOffset::isArrayOffset()
{
    return m_isArrayOffset;
}

bool MemoryCellOffset::ElementaryOffset::isArrayIndexUnknown()
{
    OPS_ASSERT(m_isArrayOffset == true);
    return m_isArrayIndexUnknown;
}

Reprise::StructMemberDescriptor* MemoryCellOffset::ElementaryOffset::getStructField()
{
    OPS_ASSERT(m_isArrayOffset == false);
    return m_structField;
}

long long int MemoryCellOffset::ElementaryOffset::getArrayIndex()
{
    OPS_ASSERT(m_isArrayOffset == true);
    if (m_isArrayIndexUnknown != false)
        OPS_ASSERT(m_isArrayIndexUnknown == false);
    return m_arrayIndex;
}

Reprise::TypeBase* MemoryCellOffset::ElementaryOffset::getArrayElementType()
{
    OPS_ASSERT(m_isArrayOffset == true);
    return m_arrayElementType.get();
}

//возвращает тип ячейки массива или поля структуры
Reprise::TypeBase* MemoryCellOffset::ElementaryOffset::getType()
{
    if (m_isArrayOffset)
        return m_arrayElementType.get();
    else
        return &m_structField->getType();
}

void MemoryCellOffset::ElementaryOffset::setArrayIndex(long long int i)
{
    OPS_ASSERT(m_isArrayOffset == true);
    OPS_ASSERT(m_isArrayIndexUnknown == false);
    m_arrayIndex = i;
}

//возвращает 0 - точно не совпадают, 1 - точно совпадают, 2 - могут совпадать, а могут нет (это 2 элемента одного массива), 3 - могут совпадать, а могут нет (в остальных случаях)
int MemoryCellOffset::ElementaryOffset::compare(ElementaryOffset &a, ElementaryOffset &b)
{
    //если оба - индексы массива
    if (a.isArrayOffset() && b.isArrayOffset())
    {
        if (!a.isArrayIndexUnknown() && !b.isArrayIndexUnknown())
        {
            if (a.getArrayElementType()->isEqual(*b.getArrayElementType()))
            {
                if (a.getArrayIndex() == b.getArrayIndex()) return 1; else return 0;
            }
            else return 3;
        }
        else
        {
            if (a.getArrayElementType()->isEqual(*b.getArrayElementType()))
            {
                if (a.isArrayIndexUnknown() && b.isArrayIndexUnknown()) return 1;
                else   return 2;
            }
            else return 3;
        }
    }
    //если оба - поля одной структуры
    if ((!a.isArrayOffset() && !b.isArrayOffset()) && isOfSameStruct(a.getStructField(), b.getStructField()))
    {
        Reprise::StructType* sta = a.getStructField()->getParent()->cast_ptr<Reprise::StructType>();
        Reprise::StructType* stb = b.getStructField()->getParent()->cast_ptr<Reprise::StructType>();
        OPS_ASSERT((sta != 0) && (stb != 0));
        if (a.getStructField() == b.getStructField()) return 1;
        else if (sta->isUnion()) return 3; else return 0;
    }
    return 3;
}

std::string MemoryCellOffset::ElementaryOffset::toString()
{
    string res;
    if (isArrayOffset())
    {
        if (isArrayIndexUnknown())
            res += "[?]";
        else
            res += "[" + OPS::Strings::format("%d", getArrayIndex()) + "]";
    }
    else
        res += "." + getStructField()->getName();
    return res;
}

}//end of namespace
}//end of namespace
