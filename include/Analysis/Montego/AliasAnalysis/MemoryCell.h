#pragma once
/*
здесь описан клас MemoryCell ячейка памяти
Это VariableDeclaration переменной или оператор malloc или SubroutineDeclaration
ПОДРОБНОСТИ СМ. В ФАЙЛЕ "/docs/Анализ альясов/SAMC.txt"
*/

#include <list>
#include <map>
#include "Reprise/Types.h"

namespace OPS
{
namespace Montego
{
//ячейка памяти:
    //VariableDeclaration переменной,
    //оператор malloc,
    //SubroutineDeclaration (readOnly),
    //абстрактное имя массива на стеке (readOnly),
    //anotherAllotment № n - какая-то ячейка, память под которую выделилась вне анализируемой программы

//для статических массивов: int (*(a[10][20]))[30] вводится две ячейки памяти (хотя на самом деле программно существует только одна - ячейки массива):
// 1. readOnly = true - Ячейка VariableDeclaration a. В Си она содержит адрес нулевого элемента массива,
//    который мы можем прочитать, но не можем перезаписать. У нас это адрес любого
//    элемента массива т.е. это и a, и a[i], и &a[i][j] (т.к. не проводится анализ индексов). 
//    
// 2. readOnly = false - Ячейка, соответствующая всем элементам массива a[10][20],
//

class MemoryCellContainer;
class MemoryCell;
class MemoryCellOffset;
class FunctionContext;
class SetAbstractMemoryCell;

class MemoryCellorOffset: public TypeConvertibleMix
{
public:
    virtual ~MemoryCellorOffset()
    {
    }

public:
    virtual MemoryCell* getCellPtr() = 0;
    virtual std::string toString() = 0;
};


class MemoryCell : public MemoryCellorOffset
{
public:
    MemoryCell();
    
    MemoryCell(OPS::Reprise::RepriseBase* memoryAllotment, bool isReadOnly, const FunctionContext* functionContext);
    
	const FunctionContext* getFunctionContext() const;
    
    //возвращает ссылку на операцию выделения памяти (объявление переменной, функции или вызов malloc)
    OPS::Reprise::RepriseBase* getMemoryAllotment();
    
    //пока не нужно использовать. Непонятно, что нужно возвращать для malloc и anotherAllotment
    OPS::Reprise::TypeBase* getType();

    OPS::Reprise::ReprisePtr<OPS::Reprise::ExpressionBase> getSize();

    std::string toString();

    std::string toStringWithOffsetsAndTheirIntersections();

    //только для статических массивов, имя массива - read only
    bool isReadOnly();

    //создает новую, отличную от всех других внешнюю ячейку памяти и добавляет ее в контейнер
    static MemoryCell* createAnotherAllotment(MemoryCellContainer*);

    MemoryCellOffset *getZeroOffset();//создает, если такого смещения еще не было

    MemoryCellOffset* getUnknownOffset();//создает, если такого смещения еще не было

    MemoryCell* getCellPtr();

    //добавляет смещение и наводит связи, возвращает указатель на добавленное смещение
    //если такое смещение уже было в контейнере, то добавления не происходит и возвращается указатель на существующее смещение
    MemoryCellOffset* insertOffsetIntoContainer(MemoryCellOffset &off);

    //для VariableDeclaration выдает все элементарные (самые глубокие) смещения в порядке их следования в памяти
    //TODO::что делать с union-ами ?
    std::vector<MemoryCellOffset*> getEveryPossibleOffsetSequence(size_t maxSize);

    SetAbstractMemoryCell getOffsets(MemoryCellContainer& cont);

    //возвращает 0 - точно не совпадают, 1 - совпадают
    //наборы смещений внутри ячеек не сравнивает
    static int compare(MemoryCell &a, MemoryCell &b);

private:

    //обновляет связи при добавлении нового смещения (смещение должно быть уже добавлено!!!)
    void updateIntersections(MemoryCellOffset *off);

    OPS::Reprise::RepriseBase* m_memoryAllotment;
    const FunctionContext* m_functionContext;
    bool m_isReadOnly;//только для статических массивов, true - это элемент массива, false - адрес
    int m_anotherAllotmentIndex; // = 0 для обычных ячеек, >0 для anotherAllotment
    std::list<MemoryCellOffset> m_offsets; //все используемые смещения в данной ячейке памяти
    MemoryCellOffset* m_zeroOffset; //указатель на начало ячейки
    MemoryCellOffset* m_unknownOffset;//указатель на неизвестный байт ячейки

    friend class MemoryCellContainer;
    friend class MemoryCellOffset;
};

//Смещение от начала ячейки - сумма слагаемых, каждое следующее является подмножеством подячейки предыдущего
//Элементы последовательности:
//  для обращения к полю массива на стеке - тип элементов массива и кол-во элементов от начала (число или "неизвестно")
//  для обращения к полю структуры - ссылка на StructDescriptor
//Пример:
//s.a[2].b.c
//Смещение: (отступ поля a   +   отступ на 2 элемента массива   +   отступ поля b   +   отступ поля c)
class MemoryCellOffset : public MemoryCellorOffset
{
public:
    class ElementaryOffset//одно слагаемое
    {
    public:
        ElementaryOffset();//создает некорректное нулевое смещение
        ElementaryOffset(Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType, long long int arrayIndex, bool isArrayIndexUnknown);
        explicit ElementaryOffset(Reprise::StructMemberDescriptor* structField);
        bool isArrayOffset();
        bool isArrayIndexUnknown();
        Reprise::StructMemberDescriptor* getStructField();
        long long int getArrayIndex();
        Reprise::TypeBase* getArrayElementType();
        Reprise::TypeBase* getType();//возвращает тип ячейки массива или поля структуры

        //возвращает 0 - точно не совпадают, 1 - точно совпадают, 2 - могут совпадать, а могут нет (это 2 элемента одного массива), 3 - могут совпадать, а могут нет (в остальных случаях)
        static int compare(ElementaryOffset &a, ElementaryOffset &b);

        std::string toString();

    private:

        //меняет индекс у смещения с известным индексом!
        void setArrayIndex(long long int i);

        bool m_isArrayOffset; //true - для элементов массива на стеке, false для полей структур
        bool m_isArrayIndexUnknown; //имеет смысл только для []
        Reprise::StructMemberDescriptor* m_structField;
        long long int m_arrayIndex; //может оказаться отрицательным! не может = 0
        Reprise::ReprisePtr<Reprise::TypeBase> m_arrayElementType;

        friend class MemoryCellOffset;
    };
    typedef std::map<MemoryCellOffset*, std::list<MemoryCellOffset::ElementaryOffset> > OffsetAdditionMap;

    //использовать с осторожностью - создает смещение без ссылки на ячейку
    MemoryCellOffset();

    //создает нулевое или неизвестное смещение (НЕ добавляет его в контейнер)
    MemoryCellOffset(MemoryCell* memoryCell, bool isUnknownOffset);

    //создает заданное известное смещение (НЕ добавляет его в контейнер)
    MemoryCellOffset(MemoryCell* memoryCell, std::vector<ElementaryOffset>& offset);

    //создает заданное известное смещение (НЕ добавляет его в контейнер)
    MemoryCellOffset(MemoryCell* memoryCell, std::list<ElementaryOffset>& offset);

    MemoryCell* getCellPtr();

    //печатает ячейку и смещение
    std::string toString();

    //печатает только смещение
    std::string toStringOffsetOnly();

    //строит элементарное смещение для заданной операции [] или .
    static ElementaryOffset buildElemOffset(Reprise::ExpressionBase& e, int currentBracketNumber);

    //добавляет к текущему указанное смещение addition, сохраняет новое смещение в контейнере и возвращает его адрес
    MemoryCellOffset* addAndInsertOffsetIntoContainer(std::list<MemoryCellOffset::ElementaryOffset>& addition);

    //приводит подобные в смещении
    void simplifyOffset();

    //возвращает 0 - не пересекаются, 1 - a<b, 2 - a>b, 3 - a=b, 4 - могут пересекаться, а могут и нет
    static int getIntersectType(MemoryCellOffset *a, MemoryCellOffset *b);

    OffsetAdditionMap getInnerOffsetsForStruct(Reprise::StructType& st);

    std::set<MemoryCellOffset*>& getIntersectedOffsets();

    MemoryCellOffset* addConst(long long int addition,  Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType);

    MemoryCellOffset* undefineLastOffset(Reprise::ReprisePtr<Reprise::TypeBase> arrayElementType);

    //проверяет типы в последовательности смещений
    void checkTypes();

private:
    //удаляет старые и наводит новые связи, ссылки на данную ячейку не обновляются!!!!!
    //вызывается из MemoryCell::updateIntersections. Больше ниоткуда вызывать нельзя!!!!
    void updateIntersections();

    MemoryCell* m_memoryCell;
    std::vector<ElementaryOffset> m_offset; //если список пуст, то смещение = 0
    bool m_isUnknownOffset;
    std::set<MemoryCellOffset*> m_intersectedOffsets;//пересекающиеся с данным отступы

    friend class MemoryCellContainer;
    friend class MemoryCell;
};


}//end of namespace
}//end of namespace
