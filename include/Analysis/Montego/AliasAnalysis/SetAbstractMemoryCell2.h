#pragma once
/*
здесь описан клас SetAbstractMemoryCell - множество адресов ячеек памяти MemoryCell
т.е. множество значениий какого-либо указателя.
Ячейки памяти: VariableDeclaration переменных или операторы malloc или SubroutineDeclaration
ПОДРОБНОСТИ СМ. В ФАЙЛЕ "/docs/Анализ альясов/SAMC.txt"
*/

#include <set>
//#include "Reprise/Reprise.h"
#include "Analysis/Montego/AliasAnalysis/MemoryCell.h"

namespace OPS
{
namespace Montego
{

class MemoryCellContainer;

typedef MemoryCellorOffset* MemoryCellPointer;

class SetAbstractMemoryCell
{
public :

    ~SetAbstractMemoryCell();

    explicit SetAbstractMemoryCell(MemoryCellContainer& memoryCellContainer);
    SetAbstractMemoryCell(MemoryCellContainer& memoryCellContainer, std::set<MemoryCellOffset*>& s);

    static SetAbstractMemoryCell* constructEmptySet(MemoryCellContainer& memoryCellContainer);

    SetAbstractMemoryCell(const SetAbstractMemoryCell& other);

    SetAbstractMemoryCell& operator=(const SetAbstractMemoryCell& other);

    bool isUniversal() const;

    bool isEmpty() const;

    void makeUniversal();

    bool unionWith(const SetAbstractMemoryCell& other);

    std::string toString() const;

    bool isIntersectWith(SetAbstractMemoryCell& other, bool useDependentCellsAlso) const;

    MemoryCellContainer* getMemoryCellContainer() const;

	MemoryCellPointer operator[](size_t i) const;
    //MemoryCell*& operator[](int i);

	void erase(size_t i);

    void clear();

    void insert(MemoryCellPointer m);

    bool hasOneElement() const;

    int find(MemoryCellPointer m) const;

    size_t size() const;

    bool operator==(const SetAbstractMemoryCell& other) const;
    bool operator!=(const SetAbstractMemoryCell& m2) const;

    size_t getHashCode() const;

    SetAbstractMemoryCell getIntersectedOffsets() const;

    void addConstToOffsets(long long int addition,  Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType);

    void undefineLastOffset(Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType);

private :

    SetAbstractMemoryCell();

    bool isIntersectWithHelper(SetAbstractMemoryCell& other) const;

    struct Impl
    {
        MemoryCellPointer* m_content;//массив указателей на ячейки, отсортированный по возрастанию
        size_t m_size;
        bool m_flagIsUniversal;
        MemoryCellContainer* m_memoryCellContainer;

        Impl();
        ~Impl();
        void clear();

        Impl* clone();
        //Если количество ссылок на объект = 1, то возвращает адрес его самого
        //в противном случае выполняет release и возвращает клон себя сомого
        Impl* detach();
        void addRef() { m_refCount++; }
        //уменьшает количество ссылок на массив на 1. Если оно стало = 0, то удаляет объект
        void release();

    private:
        int m_refCount;
    };

    Impl* p;
};


}//end of namespace
}//end of namespace
