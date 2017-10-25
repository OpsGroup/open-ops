#pragma once
/*
здесь описан клас MemoryCellContainer место хранения ячеек памяти
Это VariableDeclaration переменной или оператор malloc или SubroutineDeclaration
ПОДРОБНОСТИ СМ. В ФАЙЛЕ "/docs/Анализ альясов/SAMC.txt"
*/

#include <list>
#include "Reprise/Reprise.h"
#include "Analysis/Montego/AliasAnalysis/MemoryCell.h"

namespace OPS
{
namespace Montego
{
class SetAbstractMemoryCell;

class MemoryCellContainer :
        public OPS::NonCopyableMix
{
public:
    MemoryCellContainer();
    ~MemoryCellContainer();

    //возвращает адрес ячейки из контейнера (или нулевого смещения в ячейке) по заданному контексту и памяти (VariableDeclaration переменной или оператор malloc или SubroutineDeclaration)
    //если не нашла, возвращает 0
    MemoryCellorOffset* getMemoryCellorOffset(const FunctionContext* f, OPS::Reprise::RepriseBase* memoryAllotment, bool isReadOnly, bool useMemoryCellOffsets);

    //добавляет в контейнер все ячейки из последней процедуры в контексте, включая формальные параметры
    void addCellsFrom(const FunctionContext& f);

    //добавляет глобальные параметры (включая функции)
    void addGlobalCells(Reprise::ProgramUnit& pu);

    //возвращает множество всех ячеек для заданной переменной
    //возвращается контекстно независимая информация!
    SetAbstractMemoryCell getAllCellsOf(OPS::Reprise::VariableDeclaration& v, bool useOffsets);

    SetAbstractMemoryCell& getEmptySet() { return *m_emptySet; }

    const MemoryCellOffset *getZeroOffsetOf(MemoryCell* m);

    bool isEmpty();

    std::string toString();

private:
    void addVariableDeclaration(const FunctionContext* fcon, OPS::Reprise::VariableDeclaration& var);
    void addCell(MemoryCell &cell);

    typedef std::list< MemoryCell > CellList;
    typedef std::multimap<OPS::Reprise::RepriseBase*, MemoryCell*> AllotmentToCellMap;
    typedef std::set<const FunctionContext*> FunctionContextSet;

    CellList m_container;
    AllotmentToCellMap m_hash;
    FunctionContextSet m_addedContexts;
    SetAbstractMemoryCell* m_emptySet;
    int m_lastAnotherAllotmentIndex;//максимальный индекс существующей внешней ячейки, = 0 когда внешних ячеек нет

    friend class MemoryCell;
};


}//end of namespace
}//end of namespace
