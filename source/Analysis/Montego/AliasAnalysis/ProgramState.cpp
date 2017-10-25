#include "ProgramState.h"
#include "MemoryCellContainer.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "ExpressionAnalyser.h"
#include "FunctionContext.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "Reprise/Service/DeepWalker.h"
#include "AliasAnalysisContext.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{

//определена в ExpressionAnalyser.cpp
void copySuboffsets(SetAbstractMemoryCell& generators, SuboffsetsContent& copiedData, ProgramState& programState, bool debug);

ProgramState::ProgramState(MemoryCellContainer& memoryCellContainer, const FunctionContext* functionContext, bool useMemoryCellOffsets):
                    m_memoryCellContainer(&memoryCellContainer), 
                    m_functionContext(functionContext),
                    m_useMemoryCellOffsets(useMemoryCellOffsets)
{

}

MemoryCellContainer* ProgramState::getMemoryCellContainer()
{
    return m_memoryCellContainer;
}

const FunctionContext* ProgramState::getFunctionContext() const
{
    return m_functionContext;
}

void ProgramState::setCellContents(MemoryCellorOffset *cell, const SetAbstractMemoryCell &samc)
{
#if OPS_BUILD_DEBUG
    //проверка однородности
    //std::cout << "cell = " << cell->toString() << "\n";
    if (m_useMemoryCellOffsets)
    {
        if (!cell->is_a<MemoryCellOffset>())
        {
            std::cout << "We use offsets, but have MemoryCell " << cell->toString() <<" in program state::setCellContents argument. Program state: :\n" << toString() << "\n";
            std::cout.flush();
            throw RuntimeError("We use offsets, but have MemoryCell " + cell->toString() + " in program state::setCellContents argument");
        }
    }
    else
    {
        if (!cell->is_a<MemoryCell>())
        {
            std::cout << "We don't use offsets, but have MemoryCellOffset " << cell->toString() <<" in program state::setCellContents argument. Program state: :\n" << toString() << "\n";
            std::cout.flush();
            throw RuntimeError("We don't use offsets, but have MemoryCellOffset " + cell->toString() + " in program state::setCellContents argument");
        }
    }

    for (size_t i = 0; i < samc.size(); i++)
    {
        if (!samc[i])
        {
            std::cout << "Null in program state setCellContents for cell " << cell->toString() << "\n";
            std::cout.flush();
            throw RuntimeError("Null in program state setCellContents");
        }
        if (m_useMemoryCellOffsets && !samc[i]->is_a<MemoryCellOffset>())
        {
            std::cout << "We use offsets, but have MemoryCell " << samc[i]->toString() <<" in program state:\n" << toString() << "\n";
            std::cout.flush();
            throw RuntimeError("We use offsets, but have MemoryCell " + samc[i]->toString() + " in program state");
        }
        if (!m_useMemoryCellOffsets && !samc[i]->is_a<MemoryCell>())
        {
            std::cout << "We dont use offsets, but have MemoryCellOffset " << samc[i]->toString() <<" in program state:\n" << toString() << "\n";
            std::cout.flush();
            throw RuntimeError("We dont use offsets, but have MemoryCellOffset " + samc[i]->toString() + " in program state");
        }
    }
#endif
    iterator it = m_cellContent.find(cell);

	if (!samc.isEmpty())
	{
		// добавляем только непустые ячейки
		// проверяем, есть ли уже такая ячейка
		if (it != m_cellContent.end())
			it->second = samc; // да - меняем текущее значение
		else
			m_cellContent.insert(std::make_pair(cell, samc)); //нет - вставляем новое
	}
	else
	{
		if (it != m_cellContent.end())
			m_cellContent.erase(it);
	}
}

int ProgramState::setCellContents(SetAbstractMemoryCell& generators, const SetAbstractMemoryCell &copiedData, bool canReplace)
{
    if (!generators.isUniversal())
    {
        if (!m_useMemoryCellOffsets)
            //удаляем из generators все ячейки readOnly
            for (size_t i = 0; i < generators.size(); )
                if (generators[i]->cast_to<MemoryCell>().isReadOnly())
                {
                    generators.erase(i);
                }
                else ++i;
        if (generators.isEmpty())  return -1;

        //обновляем m_programState
        if ((generators.hasOneElement()) && canReplace)
            //заменяем старое содержимое на новое
            setCellContents(generators[0], copiedData);
        else
            //так как не знаем чему точно присваивается, то придется добавлять ко всем
            for (size_t i = 0; i < generators.size(); ++i)
                unionCellContentsWith(generators[i], copiedData);
    }
    else // генератор может указывать на все подряд
    {
        //так как не знаем чему точно присваивается, то придется добавлять ко всем
        ProgramState::iterator it;
        for (it = begin(); it != end(); ++it)
        {
            if (m_useMemoryCellOffsets)
            {
                if (!it->first->cast_to<MemoryCellOffset>().getCellPtr()->isReadOnly())
                    it->second.unionWith(copiedData);
            }
            else
                if (!it->first->cast_to<MemoryCell>().isReadOnly())
                    it->second.unionWith(copiedData);
        }
    }
    return 0;
}

SetAbstractMemoryCell ProgramState::getCellContents(MemoryCellorOffset *cell) const
{
	const_iterator it = m_cellContent.find(cell);

	if (it != m_cellContent.end())
		return it->second;
	else
        return SetAbstractMemoryCell(*m_memoryCellContainer);
}

//возвращает объединение содержимых
SetAbstractMemoryCell ProgramState::getCellContents(const SetAbstractMemoryCell& cells) const
{
    SetAbstractMemoryCell res(*m_memoryCellContainer);
    if (cells.isUniversal())
    {
        res.makeUniversal();
        return res;
    }
	for (size_t i = 0; i < cells.size(); i++)
    {
        res.unionWith(getCellContents(cells[i]));
        if (res.isUniversal()) break;
    }
    return res;
}

// find element matching _Keyval or insert with default mapped
SetAbstractMemoryCell& ProgramState::getCellContentsRef(MemoryCellorOffset *keyval)
{	
    iterator it = m_cellContent.find(keyval);
    if (it == m_cellContent.end())
    {
        //если нет - создаем пустую
        SetAbstractMemoryCell samc(*m_memoryCellContainer);
		return m_cellContent.insert(std::make_pair(keyval,samc)).first->second;
    }
    else
    {
        return it->second;
    }
}

void ProgramState::unionCellContentsWith(MemoryCellorOffset *cell, const SetAbstractMemoryCell &samc)
{
	if (samc.isEmpty())
		return;

	iterator it = m_cellContent.find(cell);

	if (it == m_cellContent.end())
		m_cellContent.insert(std::make_pair(cell, samc));
	else
		it->second.unionWith(samc);
}

void ProgramState::unionWithProgramState(ProgramState& other)
{
    iterator it = other.begin();
    for ( ; it != other.end(); ++it)
    {
        unionCellContentsWith(it->first, it->second);
    }
}

// сравнение (для скорости надо бы ввести хеши)
bool ProgramState::operator==(const ProgramState& other) const
{
	const_iterator first1 = m_cellContent.begin(), last1 = m_cellContent.end(),
			first2 = other.m_cellContent.begin(), last2 = other.m_cellContent.end();

	while(first1 != last1 && first2 != last2)
	{
		if (first1->second.isEmpty()) first1++; // пропускаем пустые
		else if (first2->second.isEmpty()) first2++; // пропускаем пустые
		else if (*first1++ != *first2++)
			return false;
	}

	// Мы здесь - значит один или оба контейнера кончились

	while(first1 != last1)
	{
		if (!first1->second.isEmpty())
			return false; // осталась непустая ячейка - не равны
		first1++;
	}

	while(first2 != last2)
	{
		if (!first2->second.isEmpty())
			return false;// осталась непустая ячейка - не равны
		first2++;
	}

	return true; // остались только пустые ячейки - равны
}

class SimpleOccurSearchVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    SimpleOccurSearchVisitor(){}

    void visit(OPS::Reprise::ReferenceExpression& e) {m_constOrRefExpr.push_back(&e);}
    void visit(OPS::Reprise::SubroutineReferenceExpression& e) {m_constOrRefExpr.push_back(&e);}
    void visit(OPS::Reprise::StrictLiteralExpression& e) {m_constOrRefExpr.push_back(&e);}
	void visit(OPS::Reprise::EnumAccessExpression& e) {m_constOrRefExpr.push_back(&e);}

    std::list<RepriseBase*> m_constOrRefExpr;
};



std::vector<SetAbstractMemoryCell> gatherInitExprResults(ExpressionBase& e, ProgramState& p, ExpressionAnalyser *ea)
{
    SimpleOccurSearchVisitor visitor;
    e.accept(visitor);
    std::vector<SetAbstractMemoryCell> result;
    result.resize(visitor.m_constOrRefExpr.size(), ea->getAnalysisContext().memoryCellContainer().getEmptySet());
    int i = 0;
    //std::cout << "gatherInitExprResults of " << e.dumpState() << "\n";  std::cout.flush();
    //std::cout << "visitor found " << visitor.m_constOrRefExpr.size() << " constOrRefExpr\n";
    //std::cout.flush();
    RepriseBase* previousHeadNode = 0;
    for (std::list<RepriseBase*>::iterator it = visitor.m_constOrRefExpr.begin(); it != visitor.m_constOrRefExpr.end(); ++it)
    {
        //находим очередное элементарное инициализирующее выражение
        RepriseBase* parent = (*it)->getParent(), *headNode = *it;
        while (!parent->is_a<VariableDeclaration>() && !parent->is_a<CompoundLiteralExpression>())
        {
            headNode = parent;
            parent = parent->getParent();
        }
        if (previousHeadNode == headNode) continue;
        //std::cout << "visiting " << headNode->dumpState() << "\n";  std::cout.flush();
        ea->visit(headNode->cast_to<ExpressionBase>(), p, 0);
        result[i] = ea->getVisitReturnValue();
        //std::cout << "result = " << result[i].toString() << "\n";  std::cout.flush();
        i++;
        previousHeadNode = headNode;
    }
    result.resize(i, ea->getAnalysisContext().memoryCellContainer().getEmptySet());
    return result;
}

//возвращает связанные с данным вхождением ячейки памяти
SetAbstractMemoryCell addVarDeclToProgramState(ProgramState& p, const FunctionContext* functionContext, VariableDeclaration& var, bool useMemoryCellOffsets, ExpressionAnalyser* ea)
{
    MemoryCellContainer* memoryCellContainer = p.getMemoryCellContainer();
    SetAbstractMemoryCell res(*memoryCellContainer);

    if (useMemoryCellOffsets)
    {
        MemoryCellorOffset* m0 = memoryCellContainer->getMemoryCellorOffset(functionContext, &var, false, false);
        if (m0 == 0)
        {
            std::cout << "memoryCellContainer = " << memoryCellContainer->toString() << "\n";
            std::cout.flush();
            throw RuntimeError("Very strange? I can't find memory cell in ProgramState, addVarDeclToProgramState1");
        }
        MemoryCell* m = m0->cast_ptr<MemoryCell>();
        res.insert(m->getZeroOffset());
        if (var.hasNonEmptyInitExpression())
        {
            OPS_ASSERT(ea != 0);
            //собираем результаты всех инициализирующих выражений в порядке появления
            std::vector<SetAbstractMemoryCell> initExprResults = gatherInitExprResults(var.getInitExpression(), p, ea);
            //std::cout << "Variable " << var.dumpState() << " has non empty init expression: " << var.getInitExpression().dumpState() << "\n";
            //std::cout << "We gather " << initExprResults.size() << " results:\n ";
            //for (size_t i = 0; i < initExprResults.size(); i++) std::cout << initExprResults[i].toString() <<"; ";
            //std::cout.flush();
            OPS_ASSERT(!initExprResults.empty());
            //находим смещения, которым они присваиваются
            std::vector<MemoryCellOffset*> offsets = m->getEveryPossibleOffsetSequence(initExprResults.size());
            //std::cout << "and " << offsets.size() << " offsets:\n";
            //for (size_t i = 0; i < offsets.size(); i++) std::cout << offsets[i]->toString() <<"; ";
            //std::cout << "\n";
            std::set<MemoryCellOffset*> off(offsets.begin(), offsets.end());
            SetAbstractMemoryCell samc(*p.getMemoryCellContainer(), off);
            res = samc;
            OPS_ASSERT(offsets.size() >= initExprResults.size());
            for (size_t i = 0; i < initExprResults.size(); i++)
                p.setCellContents(offsets[i], initExprResults[i]);
        }
    }
    else
    {
        bool readOnly = Editing::desugarType(var.getType()).is_a<ArrayType>();
        MemoryCellorOffset* m = memoryCellContainer->getMemoryCellorOffset(functionContext, &var, readOnly, false);
        if (m == 0)
            throw RuntimeError("Very strange? I can't find memory cell in ProgramState, addVarDeclToProgramState2");
        res.insert(m);
        SetAbstractMemoryCell content(*memoryCellContainer), initExprRes(*memoryCellContainer);
        if (var.hasNonEmptyInitExpression())
        {
            //посещаем анализатором выражений
            OPS_ASSERT(ea != 0);
            ea->visit(var.getInitExpression(), p, functionContext);
            initExprRes = ea->getVisitReturnValue();
        }
        //если это имя массива(статически распределенного), то содержимым имени будут ячейки массива
        if (readOnly)
        {
            MemoryCellorOffset* m_arr = memoryCellContainer->getMemoryCellorOffset(functionContext, &var, false, false);
            res.insert(m_arr);
            OPS_ASSERT(m_arr!=0);
            if (m_arr == 0)
                throw OPS::RuntimeError("Very strange? I can't find memory cell in ProgramState, addVarDeclToProgramState");
            content.insert(m_arr);
            p.setCellContents(m_arr, initExprRes);
        }
        else
            content.unionWith(initExprRes);
        p.setCellContents(m, content);
    }
    p.checkUniformity();
    return res;
}

//Считывает все объявления переменных в процедуре, включая формальные параметры
//и создает соответствующие ячейки памяти. Не забывает и о глобальных переменных.
//Нужно применять только для первой функции, обходимой анализатором псевдонимов.
//Для последующих функций, на вызовы которых анализатор натыкается по ходу обхода,
//применяется конструктор ProgramState(ProgramState&, FunctionContext&);
void ProgramState::init(const FunctionContext* mainContext, ExpressionAnalyser* ea)
{
    // отчищаем старое
    clear();

    //добавляем глобальные переменые
    SetAbstractMemoryCell globalCells(*m_memoryCellContainer);
    const FunctionContext* globalContext = 0;//он пустой
    SubroutineDeclaration& sdecl = mainContext->getLastSubroutine();
    ProgramUnit* programUnit = sdecl.findProgramUnit();
    OPS_ASSERT(programUnit != 0);

    for(int unit = 0; unit < programUnit->getUnitCount(); ++unit)
    {
        //функции
        Declarations& globalDecl = programUnit->getUnit(unit).getGlobals();
        for (Declarations::SubrIterator iter = globalDecl.getFirstSubr(); iter.isValid(); ++iter)
        {
            SubroutineDeclaration& sub = *iter;
            MemoryCellorOffset* m = m_memoryCellContainer->getMemoryCellorOffset(globalContext, &sub, true, m_useMemoryCellOffsets);
            globalCells.insert(m);
            OPS_ASSERT(m!=0);
            if (m == 0)
                throw RuntimeError("Very strange? I can't find memory cell in ProgramState::init");
            setCellContents(m, SetAbstractMemoryCell(*m_memoryCellContainer));
        }
        //глобальные переменные
        for (Declarations::VarIterator iter = globalDecl.getFirstVar(); iter.isValid(); ++iter)
        {
            VariableDeclaration& var = *iter;
            globalCells.unionWith(addVarDeclToProgramState(*this, globalContext, var, m_useMemoryCellOffsets, ea));
        }
    }

    //Добавляем параметры. Их содержимое считаем произвольным, но отличным от внутренних ячеек памяти процедуры
    MemoryCell* externalMemoryCell0 = MemoryCell::createAnotherAllotment(m_memoryCellContainer);
    MemoryCellorOffset* externalMemoryCell = m_useMemoryCellOffsets ? (MemoryCellorOffset*)externalMemoryCell0->getUnknownOffset() : (MemoryCellorOffset*)externalMemoryCell0;
    globalCells.insert(externalMemoryCell);
    setCellContents(externalMemoryCell, globalCells);
    for (int i = 0; i < sdecl.getType().getParameterCount(); i++)
    {
        VariableDeclaration& var = sdecl.getType().getParameter(i).getAssociatedVariable();
        MemoryCellorOffset* m = m_memoryCellContainer->getMemoryCellorOffset(mainContext, &var, false, m_useMemoryCellOffsets);
        setCellContents(m, globalCells);
    }

    // в цикле по всем объявленным переменным добавляем их в ProgramState
    Declarations& decl = sdecl.getDeclarations();
    for (Declarations::VarIterator iter = decl.getFirstVar(); iter.isValid(); ++iter)
    {
        VariableDeclaration& var = *iter;
        if (!var.hasParameterReference()) //если это не параметр процедуры
            addVarDeclToProgramState(*this, m_functionContext, var, m_useMemoryCellOffsets, ea);
    }
    checkUniformity();
}

//Cоставляет внутреннее ProgramState. Оно является копией внешнего с добавленными ячейками из функции.
//Инициализирует ячейки формальных параметров
//в соотв. с фактическими параметрами. Последний элемент FunctionContext - анализируемая процедура
ProgramState::ProgramState(ProgramState& programState, const FunctionContext* functionContext, 
                           std::vector<SetAbstractMemoryCell >& argumentContents, std::vector<SuboffsetsContent >& structArgumentsContent,
                           bool useMemoryCellOffsets, ExpressionAnalyser *ea)
            :m_useMemoryCellOffsets(useMemoryCellOffsets)
{
    //OPS_ASSERT(functionContext.size() - 1 == programState.getFunctionContext()->size());

	SubroutineDeclaration& sdecl = functionContext->getLastSubroutine();
	SubroutineCallExpression& scall = functionContext->getLastCall();

    if (sdecl.getType().isVarArg())
    {
        OPS_ASSERT(!"Еще не реализовано!");
    }

    //копируем
    *this = programState;
    m_functionContext = functionContext;
    m_memoryCellContainer->addCellsFrom(*functionContext);

    //инициализируем формальные параметры
    OPS_ASSERT(sdecl.getType().getParameterCount() == scall.getChildCount()-1);
    if (sdecl.getType().getParameterCount() != scall.getChildCount()-1)
        throw OPS::RuntimeError("Количества формальных и фактических параметров не совпадают!");
    for (int i = 1; i < scall.getChildCount(); ++i)
    {
        //ячейка памяти формального параметра
        VariableDeclaration& var = sdecl.getType().getParameter(i-1).getAssociatedVariable();
        bool readOnly = Editing::desugarType(var.getType()).is_a<Reprise::ArrayType>();
        MemoryCellorOffset* m = m_memoryCellContainer->getMemoryCellorOffset(m_functionContext, &var, readOnly, useMemoryCellOffsets);
        OPS_ASSERT(m!=0);
        if (m == 0) throw OPS::RuntimeError("Very strange? I can't find memory cell in ProgramState(ProgramState&, FunctionContext&, OccurContainer&)");
        if (!Editing::desugarType(var.getType()).is_a<StructType>())
        {
            //заполняем ее
            setCellContents(m, argumentContents[i-1]);
        }
        else
        {
            SetAbstractMemoryCell generators(*m_memoryCellContainer);
            generators.insert(m);
            copySuboffsets(generators, structArgumentsContent[i-1], *this, false);
            SetAbstractMemoryCell offs = m->getCellPtr()->getOffsets(*m_memoryCellContainer);
            /*std::cout << "Function " << sdecl.getName() << " parameter " << var.getName() << " :\n" <<
                         toString(offs);
            std::cout << "structArgumentsContent = " << structArgumentsContent[i-1].toString() << "\n";
            */
        }
    }

    //теперь можно добавлять переменные
    Declarations& decl = sdecl.getDeclarations();
    for (Declarations::VarIterator iter = decl.getFirstVar(); iter.isValid(); ++iter)
    {
        VariableDeclaration& var = *iter;
        if (!var.hasParameterReference())
            addVarDeclToProgramState(*this, m_functionContext, var, useMemoryCellOffsets, ea);
    }
    checkUniformity();
}

void addVarDeclToSAMC(SetAbstractMemoryCell& s, const FunctionContext* functionContext, VariableDeclaration& var, bool useMemoryCellOffsets)
{
    MemoryCellContainer* memoryCellContainer = s.getMemoryCellContainer();
    bool readOnly = var.getType().is_a<ArrayType>();
    MemoryCellorOffset* m = memoryCellContainer->getMemoryCellorOffset(functionContext,&var,readOnly, useMemoryCellOffsets);
    OPS_ASSERT(m != 0);
    s.insert(m);
    if (readOnly)
    {
        MemoryCellorOffset* m_arr = memoryCellContainer->getMemoryCellorOffset(functionContext,&var,false, useMemoryCellOffsets);
        if (m_arr != 0) 
			s.insert(m_arr);
    }
}

//Обновляет текущее состояние памяти, после исполнения кода функции. Вообще говоря обновлять ничего не нужно.
//Т.к. при входе в функцию, внутреннее состояние наследует всю информацию из внешнего,
//то после анализа функции достаточно просто присвоить внешнему состоянию внутреннее.
//Чтобы съэкономить память мы удаляем из состояния информацию обо всех локальных переменных
//функции кроме динамически выделенной памяти и статических переменных(статическая - это та же 
//глобальная переменная, но с ограниченной областью видимости).
void ProgramState::updateFrom(
                              ProgramState& innerState, //состояние внутри функции
                              SubroutineDeclaration& sdecl, 
                              SubroutineCallExpression& scall) 
{
    //копируем внутреннее состояние
    ProgramState p = innerState;

    /*
    обновлять содержимое ячеек памяти, соответствующим локальным параметрам не нужно,
    во-первых потому что они передаются не по ссылке, а во вторых внутри функции
    изменяется не содержимое, а содержимое ячеек по адресу и ячеек по адресу внутри ячеек по адресу и т.п.
    Это все делается внутри функции в процессе анализа, т.к. ячейки снаружи переходят вовнутрь.
    */

    //изменяем внутреннее состояние, убирая все ссылки на локальные переменные
    ProgramState::iterator it, it1, itFind;
    Declarations& decls = sdecl.getDeclarations();
    
    //составляем множество локальных ячеек памяти
    SetAbstractMemoryCell localMem(*m_memoryCellContainer);
    //переменные
	for (Declarations::VarIterator iter = decls.getFirstVar(); iter.isValid(); ++iter)
    {
        VariableDeclaration& var = *iter;
        if (!var.declarators().has(VariableDeclarators::DECL_STATIC))
        {
            addVarDeclToSAMC(localMem, p.m_functionContext, var, m_useMemoryCellOffsets);
        }
    }
    /*они возвращаются в составе getDeclarations
    //формальные параметры
    for (int i = 0; i < sdecl.getType().getParameterCount(); ++i)
    {
        addVarDeclToSAMC(localMem,*(innerState.m_functionContext),sdecl.getType().getParameter(i).getAssociatedVariable());
    }
    */
    //удаляем локальные ячейки памяти из p == innerState
    for (it = p.begin(); it != p.end(); )
    {
        if ( localMem.find(it->first) >= 0)
        {
            it1 = it;
            ++it1;
            p.erase(it);
            it = it1;
        }
        else ++it;
    }
    // для каждой оставшейся ячейки памяти в p удаляем из ее содержимого локальные ячейки памяти
    for (it = p.begin(); it != p.end(); ++it)
    {
        if (!it->second.isUniversal())
        {
            for (size_t it2 = 0; it2 < it->second.size(); )
            {
                if ( localMem.find(it->second[it2]) >= 0 ) 
                {
                    it->second.erase(it2);
                }
                else ++it2;
            }
        }
    }
    //добавляем все, что осталось, в текущее состояние с заменой
    for (it = p.begin(); it != p.end(); ++it)
    {
		setCellContents(it->first, it->second);
    }
    checkUniformity();
}

ProgramState::iterator ProgramState::begin()
{
    return m_cellContent.begin();
}

ProgramState::iterator ProgramState::end()
{
    return m_cellContent.end();
}

ProgramState::const_iterator ProgramState::begin() const
{
    return m_cellContent.begin();
}

ProgramState::const_iterator ProgramState::end() const
{
    return m_cellContent.end();
}

int ProgramState::size()
{
    return m_cellContent.size();
}

ProgramState::iterator ProgramState::find(MemoryCellorOffset *m)
{
    return m_cellContent.find(m);
}

ProgramState::const_iterator ProgramState::find(MemoryCellorOffset *m) const
{
    return m_cellContent.find(m);
}

void ProgramState::clear()
{
    m_cellContent.clear();
}

void ProgramState::erase(iterator it)
{
    m_cellContent.erase(it);
}

void ProgramState::makeAllUniversal()
{
    for (iterator it = begin(); it != end(); ++it)
    {
        it->second.makeUniversal();
    }
}

size_t ProgramState::getHashCode() const
{
	size_t h = 0;
	for (const_iterator it = begin(); it != end(); ++it)
	{
		h += (size_t)it->first ^ it->second.getHashCode();
	}
	return h;
}

std::string ProgramState::toString()
{
    std::string res;
    std::set<std::string> lines;

    const_iterator it = m_cellContent.begin();
    for(; it != m_cellContent.end(); ++it)
    {
        if (m_useMemoryCellOffsets)
            lines.insert(it->first->cast_to<MemoryCellOffset>().toString() + " = " + it->second.toString());
        else
            lines.insert(it->first->cast_to<MemoryCell>().toString() + " = " + it->second.toString());
    }
    for (std::set<std::string>::iterator i = lines.begin(); i != lines.end(); ++i)
        res += *i + "\n";
    return res;
}

std::string ProgramState::toString(SetAbstractMemoryCell& samc)
{
    std::string res;
    std::set<std::string> lines;
    for (size_t i = 0; i < samc.size(); i++)
    {
        MemoryCellorOffset* m = samc[i];
        std::map<MemoryCellorOffset*, SetAbstractMemoryCell >::iterator it = m_cellContent.find(m);
        if (it != m_cellContent.end())
        {
            SetAbstractMemoryCell& content = it->second;
            lines.insert("Content of " + m->toString() + " = " + content.toString());
        }
        else lines.insert("Content of " + m->toString() + " = empty");
    }
    for (std::set<std::string>::iterator i = lines.begin(); i != lines.end(); ++i)
        res += *i + "\n";
    return res;
}

//проверка однородности
void ProgramState::checkUniformity()
{
#if OPS_BUILD_DEBUG
    std::map<MemoryCellorOffset*, SetAbstractMemoryCell >::iterator it = m_cellContent.begin();
    for ( ; it != m_cellContent.end(); ++it)
        if (m_useMemoryCellOffsets)
        {
            MemoryCellOffset* m;
            m = it->first->cast_ptr<MemoryCellOffset>();
            if (m == 0) throw OPS::RuntimeError("Program state in terms of offsets contains MemoryCell: " + it->first->cast_ptr<MemoryCell>()->toString());
            size_t i = 0;
            for ( ; i < it->second.size(); i++)
            {
                m = it->second[i]->cast_ptr<MemoryCellOffset>();
                if (m == 0) break;
            }
            if (m == 0) throw OPS::RuntimeError("Program state in terms of offsets contains MemoryCell: " + it->second[i]->cast_ptr<MemoryCell>()->toString());
        }
        else
        {
            MemoryCell* m;
            m = it->first->cast_ptr<MemoryCell>();
            if (m == 0) throw OPS::RuntimeError("Program state in terms of cells contains offset: " + it->first->cast_ptr<MemoryCellOffset>()->toString());
            size_t i = 0;
            for ( ; i < it->second.size(); i++)
            {
                m = it->second[i]->cast_ptr<MemoryCell>();
                if (m == 0) break;
            }
            if (m == 0) throw OPS::RuntimeError("Program state in terms of cells contains offset: " + it->second[i]->cast_ptr<MemoryCellOffset>()->toString());
        }
#endif

}

void ProgramState::printDifferenceFrom(ProgramState& other)
{
    //проверяем наличие в текущем ячеек, отсутствующих в other
    for(iterator it = begin(); it != end(); ++it)
        if (other.find(it->first) == other.end())
            std::cout << "cell " << it->first->toString() << " is missing in other\n";
    //проверяем наличие в текущем ячеек, отсутствующих в this
    for(iterator it = other.begin(); it != other.end(); ++it)
        if (find(it->first) == end())
            std::cout << "cell " << it->first->toString() << " is missing in this\n";
    //проверяем совпадение содержимого ячеек
    for(iterator it = begin(); it != end(); ++it)
        if (other.find(it->first) != other.end())
        {
            SetAbstractMemoryCell& a = it->second;
            SetAbstractMemoryCell& b = other.find(it->first)->second;
            if (a != b)
            {
                std::cout << "Contents of " << it->first->toString() << " are different:\n";
                for (size_t i = 0; i < a.size(); i++)
                    if (b.find(a[i]) < 0)
                        std::cout << "content of " << it->first->toString() << " in other miss cell " << a[i]->toString() << "\n";
                for (size_t i = 0; i < b.size(); i++)
                    if (a.find(b[i]) < 0)
                        std::cout << "content of " << it->first->toString() << " in this miss cell " << b[i]->toString() << "\n";
            }
        }
}


}//end of namespace
}//end of namespace
