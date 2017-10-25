#pragma once
/*
здесь описан клас ProgramState - состояние памяти программы в текущей точке выполнения
По сути это содержимое всех ячеек памяти, map <MemoryCell, SetAbstractMemoryCell>
*/

#include <list>
#include "Reprise/Reprise.h"
#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"
#include "Analysis/Montego/AliasAnalysis/SetAbstractMemoryCell2.h"
#include "Analysis/Montego/AliasAnalysis/MemoryCell.h"

namespace OPS
{
namespace Montego
{

class OccurrenceContainer;
class MemoryCellContainer;
class ExpressionAnalyser;

class ProgramState
{
public:
    
    typedef std::map<MemoryCellorOffset*, SetAbstractMemoryCell >::iterator iterator;
    typedef std::map<MemoryCellorOffset*, SetAbstractMemoryCell >::const_iterator const_iterator;

    ProgramState(MemoryCellContainer&, const FunctionContext*, bool useMemoryCellOffsets);

    //Cоставляет внутреннее ProgramState. Оно является копией внешнего с добавленными ячейками из функции.
    //Инициализирует ячейки формальных параметров 
    //в соотв. с фактическими параметрами. Последний элемент FunctionContext - анализируемая процедура
    ProgramState(ProgramState&, const FunctionContext*, std::vector<SetAbstractMemoryCell >&, std::vector<SuboffsetsContent >&, bool useMemoryCellOffsets, ExpressionAnalyser* ea);
    
    //Считывает все объявления переменных в процедуре, включая формальные параметры
    //и создает соответствующие ячейки памяти. Не забывает и о глобальных переменных.
    //Нужно применять только для первой функции, обходимой анализатором псевдонимов.
    //Для последующих функций, на вызовы которых анализатор натыкается по ходу обхода,
    //применяется конструктор ProgramState(ProgramState&, FunctionContext&);
    void init(const FunctionContext* mainContext, ExpressionAnalyser* ea);

    //Обновляет текущее состояние памяти, после исполнения кода функции. Вообще говоря обновлять ничего не нужно.
    //Т.к. при входе в функцию, внутреннее состояние наследует всю информацию из внешнего,
    //то после анализа функции достаточно просто присвоить внешнему состоянию внутреннее.
    //Чтобы съэкономить память мы удаляем из состояния информацию обо всех локальных переменных
    //функции кроме динамически выделенной памяти и статических переменных(статическая - это та же 
    //глобальная переменная, но с ограниченной областью видимости).
    void updateFrom(ProgramState& innerState, OPS::Reprise::SubroutineDeclaration& sdecl, 
                    OPS::Reprise::SubroutineCallExpression& scall);

    MemoryCellContainer* getMemoryCellContainer();

    const FunctionContext* getFunctionContext() const;

    void setCellContents(MemoryCellorOffset* cell, const SetAbstractMemoryCell& samc);
    int setCellContents(SetAbstractMemoryCell& generators, const SetAbstractMemoryCell &copiedData, bool canReplace); //возвращает не 0, если присвоить нечему
    SetAbstractMemoryCell getCellContents(MemoryCellorOffset* cell) const;
    SetAbstractMemoryCell getCellContents(const SetAbstractMemoryCell& cells) const;//возвращает объединение содержимых
    SetAbstractMemoryCell& getCellContentsRef(MemoryCellorOffset* cell);
    void unionCellContentsWith(MemoryCellorOffset* cell, const SetAbstractMemoryCell& samc);
    void unionWithProgramState(ProgramState& other);

    // сравнение (для скорости надо бы ввести хеши)
    bool operator==(const ProgramState& other) const;
    
    iterator begin();

    iterator end();

    const_iterator begin() const;

    const_iterator end() const;

    int size();

    iterator find(MemoryCellorOffset*);

    const_iterator find(MemoryCellorOffset*) const;

    void clear();

    void erase(iterator it);

    void makeAllUniversal();

	size_t getHashCode() const;

    std::string toString();

    std::string toString(SetAbstractMemoryCell& samc);

    //проверка однородности
    void checkUniformity();

    //для отладки - печать отличий состояний
    void printDifferenceFrom(ProgramState& other);

private:

    MemoryCellContainer* m_memoryCellContainer; //не владеет
	const FunctionContext* m_functionContext; //не владеет
    std::map<MemoryCellorOffset*, SetAbstractMemoryCell > m_cellContent;
    bool m_useMemoryCellOffsets;//использовать ячейки или смещения в них

//    friend class MemoryCell;
//    friend class SetAbstractMemoryCell;
//    friend class MemoryCellContainer;
};


}//end of namespace
}//end of namespace
