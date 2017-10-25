#include "MemoryCellContainer.h"
#include "Analysis/Montego/AliasAnalysis/SetAbstractMemoryCell2.h"
#include "DynamicMemoryAllotmentSearchVisitor.h"
#include "FunctionContext.h"
#include <iostream>

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{

MemoryCellContainer::MemoryCellContainer()
    :m_emptySet(SetAbstractMemoryCell::constructEmptySet(*this))
    ,m_lastAnotherAllotmentIndex(0)
{
}

//добавляет глобальные параметры (включая функции)
void MemoryCellContainer::addGlobalCells(Reprise::ProgramUnit& pu)
{
    FunctionContext* globalContext = 0;//он пустой
    ProgramUnit* programUnit = &pu;
    OPS_ASSERT(programUnit != 0);

    for(int unit = 0; unit < programUnit->getUnitCount(); ++unit)
    {
        Declarations& globalDecl = programUnit->getUnit(unit).getGlobals();
        for (Declarations::VarIterator iter = globalDecl.getFirstVar(); iter.isValid(); ++iter)
        {
            VariableDeclaration& var = *iter;
            addVariableDeclaration(globalContext,var);
        }
        //теперь функции
        for (Declarations::SubrIterator iter = globalDecl.getFirstSubr(); iter.isValid(); ++iter)
        {
            MemoryCell m(&*iter, true, globalContext);
            addCell(m);
        }
    }
}

MemoryCellContainer::~MemoryCellContainer()
{
    delete m_emptySet;
}

MemoryCellorOffset* MemoryCellContainer::getMemoryCellorOffset(const FunctionContext* f, RepriseBase* memoryAllotment, bool isReadOnly, bool useMemoryCellOffsets)
{
    AllotmentToCellMap::const_iterator it = m_hash.lower_bound(memoryAllotment),
        itEnd = m_hash.end();

    while((it != itEnd) && (it->first == memoryAllotment))
    {
        MemoryCell* cell = it->second;
        if ((cell->isReadOnly() == isReadOnly) &&
            ((cell->getFunctionContext() == f) || cell->getFunctionContext() == 0))
        {
            if (useMemoryCellOffsets) return (MemoryCellorOffset*)cell->getZeroOffset(); else return cell;
        }
        ++it;
    }

    return 0;
}

void MemoryCellContainer::addCell(MemoryCell &cell)
{
    AllotmentToCellMap::iterator it0 = m_hash.lower_bound(cell.getMemoryAllotment());
    if (it0 != m_hash.end())
    {
        AllotmentToCellMap::iterator it = it0, it1 = m_hash.upper_bound(cell.getMemoryAllotment());
        for(; it != it1; it++)
            if (MemoryCell::compare(*(it->second), cell) == 1)
            {
                //std::cout << "Attempt to add duplicate cell to container: " << cell.toString() << "\n";
                //std::cout << cell.getFunctionContext()->getLastSubroutine().dumpState() << "\n";
                //std::cout << "====================================\n";
                //std:: cout << cell.getFunctionContext()->getLastSubroutine().getDeclarations().dumpState() << "\n";
                //throw RuntimeError("Attempt to add duplicate cell to container: " + cell.toString());
                return;
            }
    }
    m_container.push_back(cell);
    m_hash.insert(std::make_pair(m_container.back().getMemoryAllotment(), &m_container.back()));
}

void MemoryCellContainer::addVariableDeclaration(const FunctionContext* fcon, VariableDeclaration& var)
{
    TypeBase & t = var.getType();
    bool readOnly = Editing::desugarType(t).is_a<ArrayType>();
    MemoryCell m(&var, readOnly, fcon);
    addCell(m);
    //если это статический массив, то добавляем еще его ячейки
    if (readOnly)
    {
        MemoryCell m_el(&var, false, fcon);
        addCell(m_el);
    }
}

//добавляет в контейнер все ячейки из последней процедуры в контексте, включая формальные параметры
void MemoryCellContainer::addCellsFrom(const FunctionContext& f)
{
    // проверяем, не добавлялись ли уже из этого контекста ячейки
    if (m_addedContexts.find(&f) != m_addedContexts.end())
        return;

    const FunctionContext* globalContext = 0;//он пустой. Туда будем добавлять статические переменные
    SubroutineDeclaration& sdecl = f.getLastSubroutine();
    // в цикле по всем объявленным переменным добавляем их в ProgramState
    Declarations& decl = sdecl.getDeclarations();
    for (Declarations::VarIterator iter = decl.getFirstVar(); iter.isValid(); ++iter)
    {
        VariableDeclaration& var = *iter;
        if (var.declarators().isStatic())
            addVariableDeclaration(globalContext,var);
        else
            addVariableDeclaration(&f,var);
    }
    //анализируем код функции и находим все функции динамического выделения памяти
    DynamicMemoryAllotmentSearchVisitor v(sdecl);
    std::list<SubroutineCallExpression*> dinMemList = v.getDinMemAllotmentList();

    //добавляем соответствующие ячейки
    std::list<SubroutineCallExpression*>::iterator it;
    for (it = dinMemList.begin(); it != dinMemList.end(); ++it)
    {
        MemoryCell m(*it,false,&f);
        addCell(m);
    }
    /*
    //добавляем формальные параметры - они уже добавлены из sdecl.getDeclarations()
    SubroutineType& t = sdecl.getType();
    for (int i = 0; i < t.getParameterCount(); ++i)
    {
        ParameterDescriptor& p = t.getParameter(i);
        VariableDeclaration& var = p.getAssociatedVariable();
        addVariableDeclaration(&f,var);
    }
    */
    // помечаем конекст как посещенный
    m_addedContexts.insert(&f);
}

//возвращает множество всех ячеек для заданной переменной
//возвращается контекстно независимая информация!
SetAbstractMemoryCell MemoryCellContainer::getAllCellsOf(VariableDeclaration& v, bool useOffsets)
{
    SetAbstractMemoryCell result(*this);
    CellList::iterator it;
    for (it = m_container.begin(); it != m_container.end(); ++it)
    {
        if (it->getMemoryAllotment() == &v)
        {
            MemoryCell* m = &(*it);
            if (useOffsets) result.unionWith(m->getOffsets(*this));
            else  result.insert(m);
        }
    }
    return result;
}

bool MemoryCellContainer::isEmpty()
{
    return m_container.empty();
}

std::string MemoryCellContainer::toString()
{
    std::string res;
    for(AllotmentToCellMap::iterator it = m_hash.begin(); it != m_hash.end(); ++it)
    {
        std::string allotmentStr;
        if (it->first->is_a<SubroutineDeclaration>())  allotmentStr = "function " + it->first->cast_to<SubroutineDeclaration>().getName();
        else allotmentStr = it->first->dumpState();
        res += " allotment = " + allotmentStr + "    cells: ";
        for (AllotmentToCellMap::iterator it2 = m_hash.lower_bound(it->first); it2 != m_hash.upper_bound(it->first); ++it2)
            res += it2->second->toString()+", ";
        res += "\n";
    }
    return res;
}

const MemoryCellOffset* MemoryCellContainer::getZeroOffsetOf(MemoryCell* m)
{
    return m->getZeroOffset();
}

}//end of namespace
}//end of namespace
