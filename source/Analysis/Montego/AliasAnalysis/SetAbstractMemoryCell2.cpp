#include "Analysis/Montego/AliasAnalysis/SetAbstractMemoryCell2.h"
#include "MemoryCellContainer.h"
#include <valarray>
#include <cstring>
#include <iostream>

namespace OPS
{
namespace Montego
{

SetAbstractMemoryCell::Impl::Impl()
        :m_content(0)
        ,m_size(0)
        ,m_flagIsUniversal(false)
        ,m_memoryCellContainer(0)
        ,m_refCount(1)
{
}

SetAbstractMemoryCell::Impl::~Impl()
{
    clear();
}

void SetAbstractMemoryCell::Impl::release()
{
    if (--m_refCount == 0)
    {
        delete this;
    }
}

void SetAbstractMemoryCell::Impl::clear()
{
    m_size = 0;
    m_flagIsUniversal = false;
    delete[] m_content;
    m_content = 0;
}

SetAbstractMemoryCell::Impl* SetAbstractMemoryCell::Impl::clone()
{
    Impl* c = new Impl;
    c->m_memoryCellContainer = m_memoryCellContainer;
    c->m_flagIsUniversal = m_flagIsUniversal;
    c->m_size = m_size;

    if (m_size > 0)
    {
        c->m_content = new MemoryCellPointer[m_size];
        memcpy(c->m_content, m_content, m_size*sizeof(MemoryCellPointer));
    }
    else c->m_content = 0;
    return c;
}

SetAbstractMemoryCell::Impl* SetAbstractMemoryCell::Impl::detach()
{
    if (m_refCount == 1)
        return this;
    else
    {
        release();
        return clone();
    }
}

SetAbstractMemoryCell::SetAbstractMemoryCell()
    :p(new Impl)
{
}

SetAbstractMemoryCell* SetAbstractMemoryCell::constructEmptySet(MemoryCellContainer& memoryCellContainer)
{
    SetAbstractMemoryCell* set = new SetAbstractMemoryCell;
    set->p->m_memoryCellContainer = &memoryCellContainer;
    return set;
}

SetAbstractMemoryCell::SetAbstractMemoryCell(MemoryCellContainer& memoryCellContainer)
    :p(memoryCellContainer.getEmptySet().p)
{
    p->addRef();
}

SetAbstractMemoryCell::SetAbstractMemoryCell(MemoryCellContainer& memoryCellContainer, std::set<MemoryCellOffset*>& s)
    :p(memoryCellContainer.getEmptySet().p)
{
    p->addRef();
    p = p->detach();
    p->clear();
    p->m_content = new MemoryCellPointer[s.size()];
    p->m_size = s.size();
    int i = 0;
    for (std::set<MemoryCellOffset*>::iterator it = s.begin(); it != s.end(); ++it, ++i)
        p->m_content[i] = *it;
}

SetAbstractMemoryCell::~SetAbstractMemoryCell()
{
    p->release();
}

void SetAbstractMemoryCell::clear()
{
    MemoryCellContainer* cont = p->m_memoryCellContainer;
    p->release();
    p = cont->getEmptySet().p;
    p->addRef();
}

SetAbstractMemoryCell::SetAbstractMemoryCell(const SetAbstractMemoryCell& other)
    :p(other.p)
{
    p->addRef();
}

SetAbstractMemoryCell& SetAbstractMemoryCell::operator=(const SetAbstractMemoryCell& other)
{
    if (p != other.p)
    {
        p->release();
        p = other.p;
        p->addRef();
    }
    return *this;
}

MemoryCellContainer* SetAbstractMemoryCell::getMemoryCellContainer() const
{
    return p->m_memoryCellContainer;
}


bool SetAbstractMemoryCell::isUniversal() const
{
    return p->m_flagIsUniversal;
}

bool SetAbstractMemoryCell::isEmpty() const
{
    if (isUniversal()) return false;
    else return p->m_size == 0;
}

void SetAbstractMemoryCell::makeUniversal()
{
    p = p->detach();
    p->clear();
    p->m_flagIsUniversal = true;
}

//возвращает изменилось ли текущее множество после объединения
bool SetAbstractMemoryCell::unionWith(const SetAbstractMemoryCell& other)
{
    bool res;
    if (p->m_memoryCellContainer != other.p->m_memoryCellContainer)
        throw OPS::RuntimeError("Попытка вызвать функцию SetAbstractMemoryCell::isUnionWith для SAMC с различными контейнерами ячеек памяти");
    if (!isUniversal())
    {
        if (other.isUniversal())
        {
            makeUniversal();
            res = true;
        }
        else
        {
            size_t oldSize = p->m_size;
            if (other.p->m_size > 0)
            {
                p = p->detach();
                if (p->m_size == 0)
                {
                    *this = other;
                }
                else
                {
                    //выполняем объединение отсортированных массивов как множеств
                    size_t maxsize = p->m_size + other.p->m_size;
                    MemoryCellPointer* un = new MemoryCellPointer[maxsize];
                    size_t i = 0, j = 0, k = 0;
                    while ((i < p->m_size) && (j < other.p->m_size))
                    {
                        if (p->m_content[i] <= other.p->m_content[j])
                        {
                            un[k] = p->m_content[i];
                            if (p->m_content[i] == other.p->m_content[j]) j++;
                            ++i;
                        }
                        else
                        {
                            un[k] = other.p->m_content[j];
                            ++j;
                        }
                        ++k;
                    }
                    if (i < p->m_size)
                        for ( ; i < p->m_size; ++i, ++k) un[k] = p->m_content[i];
                    if (j < other.p->m_size)
                        for ( ; j < other.p->m_size; ++j, ++k) un[k] = other.p->m_content[j];

                    p->clear();
                    p->m_size = k;
                    p->m_content = new MemoryCellPointer[p->m_size];
                    memcpy(p->m_content, un, p->m_size*sizeof(MemoryCellPointer));
                    delete[] un;
                }
            }
            res = (oldSize != p->m_size);
        }
    }
    else res = false;

    return res;
}

std::string SetAbstractMemoryCell::toString() const
{
    std::string res;
    std::set<std::string> lines;
    if (isUniversal()) return "universal";
    if (isEmpty()) return "empty";
    bool useMemoryCellOffset = p->m_content[0]->is_a<MemoryCellOffset>();
    for (size_t i = 0; i < p->m_size-1; ++i)
            lines.insert( p->m_content[i]->toString() );;
    if (useMemoryCellOffset)
        lines.insert( p->m_content[p->m_size-1]->toString() );
    res = "set( ";
    for (std::set<std::string>::iterator i = lines.begin(); i != lines.end(); ++i)
    {
        res += *i;
        std::set<std::string>::iterator it = --(lines.end());
        if (i != it) res += ", ";
    }
    res += " )";
    return res;
}


bool SetAbstractMemoryCell::isIntersectWith(SetAbstractMemoryCell& other, bool useDependentCellsAlso) const
{
    if (p->m_memoryCellContainer != other.p->m_memoryCellContainer)
        throw OPS::RuntimeError("Попытка вызвать функцию SetAbstractMemoryCell::isIntersectWith для SAMC с различными контейнерами ячеек памяти");
    //если хотя бы одно из множеств пустое
    if (isEmpty() || other.isEmpty()) return false;
    //если хотя бы одно из множеств универсальное
    if ( isUniversal() || other.isUniversal() ) return true;

    //если оба множества не универсальные и не пустые
    bool useOffsets = p->m_content[0]->is_a<MemoryCellOffset>();
    if (!useOffsets) useDependentCellsAlso = false;

    if (!useDependentCellsAlso) return isIntersectWithHelper(other);

    //include dependent offsets
    SetAbstractMemoryCell a(*p->m_memoryCellContainer), b(*p->m_memoryCellContainer);
    a = getIntersectedOffsets();
    b = other.getIntersectedOffsets();
    bool res1 = a.isIntersectWithHelper(other);
    bool res2 = b.isIntersectWithHelper((SetAbstractMemoryCell&)*this);
    OPS_ASSERT(res1 == res2);
    return res1 || res2;
}

bool SetAbstractMemoryCell::isIntersectWithHelper(SetAbstractMemoryCell& other) const
{
    bool result = false;
    OPS_ASSERT(!isEmpty() && !other.isEmpty() && !isUniversal() && !other.isUniversal());
    //если оба множества не универсальные и не пустые
    size_t i = 0, j = 0;
    while ((i < p->m_size) && (j < other.p->m_size))
    {
        if (p->m_content[i] <= other.p->m_content[j])
        {
            if (p->m_content[i] == other.p->m_content[j]) {result = true; break;}
            ++i;
        }
        else
        {
            ++j;
        }
    }

    return result;
}

MemoryCellPointer SetAbstractMemoryCell::operator[](size_t i) const
{
    if (isUniversal()) throw OPS::RuntimeError("Попытка получить итератор для универсального множества!");
	OPS_ASSERT((i < p->m_size) && (i >= 0));
    return p->m_content[i];
}
/*
MemoryCellPointer& SetAbstractMemoryCell::operator[](int i)
{
    p = p->detach();

    if (isUniversal()) throw OPS::RuntimeError("Попытка получить итератор для универсального множества!");
    OPS_ASSERT((i < p->m_size) && (i >= 0));
    return p->m_content[i];
}
*/
void SetAbstractMemoryCell::erase(size_t i)
{
    p = p->detach();
    OPS_ASSERT(!isUniversal());
    OPS_ASSERT((i < p->m_size) && (i >= 0));
    MemoryCellPointer* newcontent = new MemoryCellPointer[p->m_size-1];
    memcpy(newcontent, p->m_content, i*sizeof(MemoryCellPointer));
    memcpy(newcontent+i, p->m_content+i+1, (p->m_size-i-1)*sizeof(MemoryCellPointer));
    delete[] p->m_content;
    --p->m_size;
    p->m_content = newcontent;
}

void SetAbstractMemoryCell::insert(MemoryCellPointer m)
{
    if (!isUniversal())
    {
        int i = find(m);
        if (i < 0)
        {
            p = p->detach();
            i = -i-1;
            MemoryCellPointer* newcontent = new MemoryCellPointer[p->m_size+1];
            memcpy(newcontent, p->m_content, i*sizeof(MemoryCellPointer));
            newcontent[i] = m;
            memcpy(newcontent+i+1, p->m_content+i, (p->m_size-i)*sizeof(MemoryCellPointer));
            delete[] p->m_content;
            ++p->m_size;
            p->m_content = newcontent;
        }
    }
}

bool SetAbstractMemoryCell::hasOneElement() const
{
    if (isUniversal()) return false;
    return p->m_size == 1;
}
//возвращает положительное значение, если нашли и отрицательное - если нет
//(отрицательное по модулю = номеру позиции + 1, куда надо вставлять)
int SetAbstractMemoryCell::find(MemoryCellPointer key) const
{
    if (isUniversal()) throw OPS::RuntimeError("Попытка получить итератор для универсального множества!");
    if (p->m_size == 0) return -1;
    int left = 0, right = p->m_size-1, middle, ind;
    if (key > p->m_content[right])
        return -(right+2);
    if (key < p->m_content[left])
        return -(left+1);

    MemoryCellPointer middleEl;
    while (right - left > 1)
    {
        middle = (left + right)/2;
        middleEl = p->m_content[middle];
        if (middleEl>=key)  right = middle;
        else left = middle;
    }
    if (p->m_content[left] == key) ind = left;
    else
    {
        if (p->m_content[right] == key) ind = right;
        else
            //элемент не найден
            ind = -(right+1);
    }
    return ind;
}

bool SetAbstractMemoryCell::operator ==(const SetAbstractMemoryCell& other) const
{
    return !(*this != other);
}

bool SetAbstractMemoryCell::operator!= (const SetAbstractMemoryCell& m2) const
{
    if (p == m2.p)
        return false;

    if (isUniversal() || m2.isUniversal())
        return isUniversal() != m2.isUniversal();
    else
    {
        if (p->m_size != m2.p->m_size)   return true;
        if (p->m_size == 0) return false;
        else
        {
            for (size_t i = 0; i < p->m_size; ++i)
                if (p->m_content[i] != m2.p->m_content[i])
                    return true;
            return false;
        }
    }
}

size_t SetAbstractMemoryCell::size() const
{
    return p->m_size;
}

size_t SetAbstractMemoryCell::getHashCode() const
{
    if (isUniversal())
        return 0;
    else if (p->m_size == 0)
        return 1;
    else
    {
        size_t h = 0;
        for(size_t i = 0; i < p->m_size; ++i)
            h += (size_t)p->m_content[i];
        return h;
    }
}

SetAbstractMemoryCell SetAbstractMemoryCell::getIntersectedOffsets() const
{
    SetAbstractMemoryCell a(*p->m_memoryCellContainer);
    if (isEmpty()) return a;
    if (isUniversal()) {a.makeUniversal(); return a;}
    a = *this;
    if (!a[0]->is_a<MemoryCellOffset>())
        std::cout << "a = " << a.toString() << "\n";
    OPS_ASSERT(a[0]->is_a<MemoryCellOffset>());
    for (size_t i = 0; i < size(); i++)
    {
        MemoryCellOffset& off = p->m_content[i]->cast_to<MemoryCellOffset>();
        SetAbstractMemoryCell rab(*p->m_memoryCellContainer, off.getIntersectedOffsets());
        a.unionWith(rab);
    }
    return a;
}

void SetAbstractMemoryCell::addConstToOffsets(long long int addition,  Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType)
{
    if (addition == 0) return;
    if (isEmpty() || isUniversal()) return;
    if (p->m_content[0]->is_a<MemoryCell>()) return;
    std::set<MemoryCellOffset*> newContent;
    for (size_t i = 0; i < size(); i++)
    {
        MemoryCellOffset* of = p->m_content[i]->cast_ptr<MemoryCellOffset>();
        newContent.insert(of->addConst(addition, arrayElementType));
    }
    *this = SetAbstractMemoryCell(*getMemoryCellContainer(), newContent);
}

void SetAbstractMemoryCell::undefineLastOffset(Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType)
{
    if (isEmpty() || isUniversal()) return;
    if (p->m_content[0]->is_a<MemoryCell>()) return;
    std::set<MemoryCellOffset*> newContent;
    for (size_t i = 0; i < size(); i++)
    {
        MemoryCellOffset* of = p->m_content[i]->cast_ptr<MemoryCellOffset>();
        newContent.insert(of->undefineLastOffset(arrayElementType));
    }
    *this = SetAbstractMemoryCell(*getMemoryCellContainer(), newContent);
}

}//end of namespace
}//end of namespace
