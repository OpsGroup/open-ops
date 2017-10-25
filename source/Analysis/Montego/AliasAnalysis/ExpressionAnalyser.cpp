#include "ExpressionAnalyser.h"
#include "AliasProcedureAnalyzer.h"
#include "DynamicMemoryAllotmentSearchVisitor.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "AliasAnalysisContext.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include <iostream>
#include <map>

using namespace std;

namespace OPS
{
namespace Montego
{

bool getInteger(RepriseBase* expr, long long int& res); //определена в MemoryCell.cpp

ExpressionAnalyser::ExpressionAnalyser
    (AliasAnalysisContext* analysisContext)
                                        :m_programState(0),
                                        m_functionContext(0),
                                        m_programStateChanged(false),
                                          m_visitReturnValue(analysisContext->memoryCellContainer()),
                                        m_analysisContext(analysisContext),
                                        m_retCode(0)
{
}

//анализирует оператор-выражение, меняет текущее состояние и обновляет SAMC вхождений
bool ExpressionAnalyser::visit(const OPS::Reprise::ExpressionStatement& e,
                               ProgramState& state, 
                               const FunctionContext* functionContext)
{
    m_retCode = 0;
    m_programState = &state;
	m_visitReturnValue.clear();
    m_functionContext = functionContext;
    m_programStateChanged = false;
    const_cast<ExpressionStatement&>(e).accept(*this);
    return m_programStateChanged;
}

//анализирует выражение, меняет текущее состояние и обновляет SAMC вхождений
bool ExpressionAnalyser::visit(const OPS::Reprise::ExpressionBase& e,
                               ProgramState& state, 
                               const FunctionContext* functionContext)
{
    m_programState = &state;
	m_visitReturnValue.clear();
    m_functionContext = functionContext;
    m_programStateChanged = false;
    const_cast<ExpressionBase&>(e).accept(*this);
    return m_programStateChanged;
}

void ExpressionAnalyser::clear()
{
    m_retCode = 0;
    m_visitReturnValue.clear();
}

ExpressionAnalyser::~ExpressionAnalyser()
{
}


void ExpressionAnalyser::visit(OPS::Reprise::ExpressionStatement& e)
{
    if (m_retCode != 0) return;
    e.get().accept(*this);
}

void ExpressionAnalyser::visit(OPS::Reprise::BasicLiteralExpression&)
{
    if (m_retCode != 0) return;
    //возвращаем пустой указатель, т.к. считается что адреса нельзя задавать вручную
    m_visitReturnValue.clear();
}

void ExpressionAnalyser::visit(OPS::Reprise::StrictLiteralExpression&)
{
    if (m_retCode != 0) return;
    //возвращаем пустой указатель, т.к. считается что адреса нельзя задавать вручную
    m_visitReturnValue.clear();
}

void ExpressionAnalyser::visit(OPS::Reprise::CompoundLiteralExpression& e)
{
    if (m_retCode != 0) return;
    SetAbstractMemoryCell res(m_analysisContext->memoryCellContainer());
    std::string varName;
    /*
    if (e.getParent()->cast_ptr<VariableDeclaration>())
        varName = e.getParent()->cast_to<VariableDeclaration>().getName();
    else
        varName = "unknown";
    std::cout << "Visiting CompoundLiteralExpression of variable decl: " << varName << "\n";
    */
    for (int i = 0; i < e.getValueCount(); i++)
    {
        m_visitReturnValue.clear();
        e.getValue(i).accept(*this);
        //std::cout <<"Value N " << i << " = " << m_visitReturnValue.toString() << "\n";
        res.unionWith(m_visitReturnValue);
    }
    m_visitReturnValue = res;
}

void ExpressionAnalyser::visit(OPS::Reprise::ReferenceExpression& e)
{
    if (m_retCode != 0) return;
    //возвращаем содержимое вхождения
    BasicOccurrencePtr o;
	if (m_analysisContext->occurrenceContainer().getOccurBy(e,o))
        throw OPS::RuntimeError("Occurrences was built for all program? Why I cant found it?");
    m_visitReturnValue = getOccurrenceContent(*o,*m_programState, &m_structReturnValue);
    //добавляем в SAMC вхождения его псевдонимы в данный момент исполнения программы
    addToOccurrenceSAMC(*o, *m_programState, m_functionContext);
}

void ExpressionAnalyser::visit(OPS::Reprise::SubroutineReferenceExpression& e)
{
    //TODO: если сюда попали, значит ЭТО НЕ ВЫЗОВ функции, а указатель на функцию
    //возвращаем указатель на функцию
    if (m_retCode != 0) return;
    m_visitReturnValue.clear();
    MemoryCellorOffset* m = m_analysisContext->memoryCellContainer().getMemoryCellorOffset(m_functionContext, &e.getReference(), true, m_analysisContext->options().recognizeStructFields);
    m_visitReturnValue.insert(m);
}

void ExpressionAnalyser::visit(OPS::Reprise::StructAccessExpression& e)
{
    if (m_retCode != 0) return;
    //возвращаем содержимое вхождения
    BasicOccurrencePtr o;
	if (m_analysisContext->occurrenceContainer().getOccurBy(e,o))
        throw OPS::RuntimeError("Occurrences was built for all program? Why I cant found it?");
    m_visitReturnValue = getOccurrenceContent(*o,*m_programState, &m_structReturnValue);
    //добавляем в SAMC вхождения его псевдонимы в данный момент исполнения программы
    addToOccurrenceSAMC(*o, *m_programState, m_functionContext);
    //проверка
    if (o->islValue() && getOccurrenceAddress(*o, *m_programState).isEmpty())
    {
        if (m_analysisContext->options().debug)
            std::cout << "Warning: container samc of " << o->toString() << " is empty (in visit StructAccessExpression)\n";
        m_retCode = 1;
        return;
    }
}

void ExpressionAnalyser::visit(OPS::Reprise::EnumAccessExpression&)
{
    if (m_retCode != 0) return;
    //возвращаем пустой указатель, т.к. считается что адреса не могут входить в enum
    m_visitReturnValue.clear();
}

void ExpressionAnalyser::visit(OPS::Reprise::TypeCastExpression& e)
{
    if (m_retCode != 0) return;
    //возвращаем значение подвыражения
    e.getCastArgument().accept(*this);
    //m_visitReturnValue - не изменяем
}

void copySuboffsets(SetAbstractMemoryCell& generators, SuboffsetsContent& copiedData, ProgramState& programState, bool debug)
{
    if (debug)
    {
        cout << "Attention: coping structure to cell"  << generators.toString() << "\n";
        cout << "m_structReturnValue = \n";
    }
    SuboffsetsContentIterator it = copiedData.begin();
    for ( ; it != copiedData.end(); ++it)
    {
        std::list<MemoryCellOffset::ElementaryOffset>& addition = it->first;
        SetAbstractMemoryCell& oneCopiedData = it->second;
        if (debug)
        {
            cout << "    ElemOffset = ";
            std::list<MemoryCellOffset::ElementaryOffset>::iterator it = addition.begin();
            for ( ; it != addition.end(); ++it) cout << it->toString() << "; ";
            cout << "  Content = " << oneCopiedData.toString() << "\n";
        }
		for (size_t i = 0; i < generators.size(); ++i)
        {
            MemoryCellOffset* g = generators[i]->cast_ptr<MemoryCellOffset>();
            MemoryCellOffset* m = g->addAndInsertOffsetIntoContainer(addition);
            if (debug)
                cout << "Setting content of generator " << m->toString() << " to value " << oneCopiedData.toString() << "\n";
            programState.setCellContents(m, oneCopiedData);
        }
    }
    if (debug)
    {
        if (generators.size() > 0)
            cout << "Result work of copySuboffsets:\n";
        else
            cout << "generators.size() == 0 in copySuboffsets:\n";
        for (size_t i = 0; i < generators.size(); i++)
        {
            MemoryCellOffset* g = generators[i]->cast_ptr<MemoryCellOffset>();
            SetAbstractMemoryCell tmp(*programState.getMemoryCellContainer());
            tmp = g->getCellPtr()->getOffsets(*programState.getMemoryCellContainer());
            cout << programState.toString(tmp);
        }
    }
}

SetAbstractMemoryCell ExpressionAnalyser::copy(BasicOccurrencePtr dst, ExpressionBase& src, bool flagIsMemcpy)
{
    //посещаем скобки вхождения-назначения
    BasicOccurrenceName dstName = dst->getName();
    for (int i=0; i < dst->getBracketCount(); i++)
        dstName.m_bracketContent[i]->accept(*this);

    //src ====================================================================================================================
    //меняем состояние, дополняем SAMC генератора и возвращаем SAMC правой части
    src.accept(*this);
    
    //только для memcpy
    BasicOccurrencePtr srcOccur;
    if (flagIsMemcpy) 
		if (m_analysisContext->occurrenceContainer().getOccurBy(src,srcOccur)) 
			throw OPS::RuntimeError("Аргумент функции memcpy не является вхождением. Программа не канонизирована.");

    SetAbstractMemoryCell copiedData(m_analysisContext->memoryCellContainer());
    if (flagIsMemcpy)
    {
        //берем содержимое ячеек по адресу src
        SetAbstractMemoryCell srcContent = getOccurrenceContent(*srcOccur,*m_programState, &m_structReturnValue);
        for (size_t i = 0; i < srcContent.size(); ++i)
            copiedData = m_programState->getCellContents(srcContent[i]);
    }
    else
        copiedData = m_visitReturnValue;

    //dst ====================================================================================================================
    //получаем все ячейки, которым может присваиваться содержимое текущим оператором =
    SetAbstractMemoryCell generators(m_analysisContext->memoryCellContainer());
    if (flagIsMemcpy)
        generators = getOccurrenceContent(*dst,*m_programState, 0);
    else
        generators = getOccurrenceAddress(*dst,*m_programState);
    bool canReplace = (dst->getBracketCount()==0);  //нет скобок
    if (!m_analysisContext->options().recognizeStructFields)
        canReplace =canReplace && (dst->getName().m_fields.size()==0);  //нет обращений к полям структур

    ReprisePtr<TypeBase> srcType = src.getResultType();
    if (!Editing::desugarType(*srcType).is_a<StructType>() || !m_analysisContext->options().recognizeStructFields)
    {
        //копируем данные элементарного типа
        //cout << "Coping nonstruct: " << src.dumpState() <<"\n";
        //cout << "Type = " << Editing::desugarType(*srcType).dumpState() << "\n";
        //cout.flush();
        //if (src.dumpState() == "(&(x[int32 0]))+(i)")
            //src.accept(*this);

        //cout << "Coping " << src.dumpState() <<" to " << dst->toString() << "\n";
        //cout << "copiedData = " << copiedData.toString() << "\n";
        //cout << "Program state before setCellContents:\n";
        //cout << m_programState->toString();
        int errorCode = m_programState->setCellContents(generators, copiedData, canReplace);
        //cout << "Program state after setCellContents:\n";
        //cout << m_programState->toString();
        if (errorCode != 0)
        {
            if (m_analysisContext->options().debug)
            {
                SAMCForOneContext samc(m_analysisContext->memoryCellContainer());
                samc = buildOccurrenceSAMC(*dst,*m_programState, 0, true);
                std::cout << "programState = " << m_programState->toString() << std::endl;
                if (flagIsMemcpy)   std::cout << "Warning: return samc of " << dst->toString() << " is empty in copy\n";
                else std::cout << "Warning: container samc of " << dst->toString() << " is empty in copy\n";
            }
            m_retCode = 1;
            return copiedData;
        }
    }
    else
    {
        //cout << "Coping struct of " << src.dumpState() << "\n";
        //копируем структуру
        if (generators.isUniversal()) m_programState->setCellContents(generators, copiedData, false /*canReplace*/);
        else copySuboffsets(generators, m_structReturnValue, *m_programState, m_analysisContext->options().debug);
    }

    //OccurrenceSAMC ====================================================================================================================
    //дополняем SAMC генератора (нужно даже, если isUniversal)
    if (addToOccurrenceSAMC(*dst, *m_programState, m_functionContext))
        m_programStateChanged = true;

    if (flagIsMemcpy)  return getOccurrenceContent(*dst,*m_programState, 0);
    else  return copiedData;
}

//возвращает ноль, если тип не является PtrType
ReprisePtr<TypeBase> getPointedType(TypeBase& ptrType)
{
    TypeBase* t = &Editing::desugarType(ptrType);
    OPS_ASSERT(!t->is_a<ArrayType>());
    if (t->is_a<PtrType>())
        return ReprisePtr<TypeBase>(&Editing::desugarType(t->cast_to<PtrType>().getPointedType()));
    else
        return ReprisePtr<TypeBase>();
}

void ExpressionAnalyser::visit(OPS::Reprise::BasicCallExpression& e)
{
    if (m_retCode != 0) return;
    bool useOffsets = m_analysisContext->options().recognizeStructFields;
    SetAbstractMemoryCell resSAMC(m_analysisContext->memoryCellContainer());
    switch (e.getKind())
    {
    case OPS::Reprise::BasicCallExpression::BCK_ARRAY_ACCESS :
    case OPS::Reprise::BasicCallExpression::BCK_TAKE_ADDRESS :
        {
            //посещаем скобки
            for (int i = 1; i < e.getChildCount(); ++i) e.getChild(i).accept(*this);
            //возвращаем содержимое вхождения
            BasicOccurrencePtr o;
			if (m_analysisContext->occurrenceContainer().getOccurBy(e,o))
                throw OPS::RuntimeError("Occurrences was built for all program? Why I cant found it?");
            resSAMC = getOccurrenceContent(*o, *m_programState, &m_structReturnValue);//именно Content, а не Address. Т.к. & - неотъемлемая часть вхождения!!!
            //добавляем в SAMC вхождения его псевдонимы в данный момент исполнения программы
            addToOccurrenceSAMC(*o, *m_programState, m_functionContext);
            //проверка
            if (o->islValue() && getOccurrenceAddress(*o, *m_programState).isEmpty())
            {
                if (m_analysisContext->options().debug)
                {
                    std::cout << "Warning: container samc of " << o->toString() << " is empty (in visit BasicCallExpression)\n";
                    std::cout << "ProgramState: " << m_programState->toString();
                    SAMCForOneContext samc(m_analysisContext->memoryCellContainer());
                    samc = buildOccurrenceSAMC(*o,*m_programState, 0, true);
                }
                m_retCode = 1;
                return;
            }
            break;
        }

    case OPS::Reprise::BasicCallExpression::BCK_ASSIGN :
        {
            BasicOccurrencePtr o;
			if (m_analysisContext->occurrenceContainer().getOccurBy(e.getArgument(0),o))
                throw OPS::RuntimeError("ExpressionAnalyser::visit(BasicCallExpression): Can't find occurrence for left part of BCK_ASSIGN");
            resSAMC = copy(o, e.getArgument(1), false);
            break;
        }

    case OPS::Reprise::BasicCallExpression::BCK_BINARY_MINUS :
    case OPS::Reprise::BasicCallExpression::BCK_BINARY_PLUS :
        {
            //std::cout << "Plus or minus: " << e.dumpState() << endl;
            //возвращаем объединение SAMC т.к. не известно какой из аргументов является адресом
			e.getArgument(0).accept(*this);
            SetAbstractMemoryCell resSAMC1 = m_visitReturnValue;
			e.getArgument(1).accept(*this);
            SetAbstractMemoryCell resSAMC2 = m_visitReturnValue;
            SetAbstractMemoryCell* resArg = 0;  int resArgInd = 0;
            long long int intConst = 0;
            if (getInteger(&e.getArgument(0), intConst)) {resArg = &resSAMC2; resArgInd = 1;}
            else if (getInteger(&e.getArgument(1), intConst)) resArg = &resSAMC1;
            if (resArg)
            {
                //один из аргументов - константа
                resArg->addConstToOffsets(e.getKind()==BasicCallExpression::BCK_BINARY_PLUS ? intConst: -intConst, getPointedType(*e.getArgument(resArgInd).getResultType()));
                resSAMC = *resArg;
            }
            else
            {
                //оба аргумента указывают неизвестно на что
                //делаем неопределенными смещения аргументов
                resSAMC1.undefineLastOffset(getPointedType(*e.getArgument(0).getResultType()));
                resSAMC2.undefineLastOffset(getPointedType(*e.getArgument(1).getResultType()));
                resSAMC = resSAMC1;
                resSAMC.unionWith(resSAMC2);
            }
            break;
        }
    case OPS::Reprise::BasicCallExpression::BCK_BITWISE_AND :
    case OPS::Reprise::BasicCallExpression::BCK_BITWISE_OR :
    case OPS::Reprise::BasicCallExpression::BCK_BITWISE_XOR :
    case OPS::Reprise::BasicCallExpression::BCK_DIVISION :
    case OPS::Reprise::BasicCallExpression::BCK_INTEGER_DIVISION :
    case OPS::Reprise::BasicCallExpression::BCK_INTEGER_MOD :
    case OPS::Reprise::BasicCallExpression::BCK_MULTIPLY :
        {
            //если SAMC аргументов непустые, то возвращаем универсальное множество - адрес неизвестен
            //если пустые - пустое множество
            e.getChild(0).accept(*this);
            SetAbstractMemoryCell res0 = m_visitReturnValue;
            if (res0.isEmpty() || m_analysisContext->options().noPointerCrazyOperations)
            {
                e.getChild(1).accept(*this);
                SetAbstractMemoryCell res1 = m_visitReturnValue;
                if (res1.isEmpty() || m_analysisContext->options().noPointerCrazyOperations)
                    resSAMC.clear();
                else
                    if (useOffsets) resSAMC.undefineLastOffset(ReprisePtr<TypeBase>());
                    else resSAMC.makeUniversal();
            }
            else
                if (useOffsets) resSAMC.undefineLastOffset(ReprisePtr<TypeBase>());
                else resSAMC.makeUniversal();
            break;
        }
    case OPS::Reprise::BasicCallExpression::BCK_COMMA :
        {
            //анализируем по очереди аргументы и возвращаем SAMC второго
            e.getChild(0).accept(*this);
            e.getChild(1).accept(*this);
            resSAMC = m_visitReturnValue;
            break;
        }
    case OPS::Reprise::BasicCallExpression::BCK_DE_REFERENCE :
        {
            // * в программе быть не должно
            throw OPS::RuntimeError("Canonical transformations were not done!");
            break;
        }
    case OPS::Reprise::BasicCallExpression::BCK_EQUAL :
    case OPS::Reprise::BasicCallExpression::BCK_NOT_EQUAL :
    case OPS::Reprise::BasicCallExpression::BCK_GREATER :
    case OPS::Reprise::BasicCallExpression::BCK_GREATER_EQUAL :
    case OPS::Reprise::BasicCallExpression::BCK_LESS :
    case OPS::Reprise::BasicCallExpression::BCK_LESS_EQUAL :
    case OPS::Reprise::BasicCallExpression::BCK_LOGICAL_AND :
    case OPS::Reprise::BasicCallExpression::BCK_LOGICAL_OR :
    case OPS::Reprise::BasicCallExpression::BCK_LOGICAL_NOT :
    case OPS::Reprise::BasicCallExpression::BCK_SIZE_OF :
        {
            for (int i = 0; i < e.getChildCount(); ++i)
                e.getChild(i).accept(*this);
            //возвращаем пустое множество
            resSAMC.clear();
            break;
        }
    case OPS::Reprise::BasicCallExpression::BCK_LEFT_SHIFT :
    case OPS::Reprise::BasicCallExpression::BCK_RIGHT_SHIFT :
        {
            //если SAMC нулевого аргумента пустая - возвращаем пустое множество
            //иначе - универсальное
            e.getChild(0).accept(*this);
            SetAbstractMemoryCell res0 = m_visitReturnValue;
            if (res0.isEmpty() || m_analysisContext->options().noPointerCrazyOperations)
                resSAMC.clear();
            else
                if (useOffsets) resSAMC.undefineLastOffset(ReprisePtr<TypeBase>());
                else resSAMC.makeUniversal();
            break;
        }
	case OPS::Reprise::BasicCallExpression::BCK_BITWISE_NOT :
    case OPS::Reprise::BasicCallExpression::BCK_UNARY_PLUS : 
    case OPS::Reprise::BasicCallExpression::BCK_UNARY_MINUS :
        {
            //возвращаем SAMC аргумента
            e.getChild(0).accept(*this);
            resSAMC = m_visitReturnValue;
            break;
        }
    default:
        {
            //вроди бы все перечислили
            throw OPS::RuntimeError("Unknown expression kind in ExpressionAnalyser::visit!");
            break;
        }
    }
    m_visitReturnValue = resSAMC;
}

Reprise::Declarations* getGlobals(Reprise::RepriseBase& e)
{
    Reprise::RepriseBase* r = &e;
    Reprise::TranslationUnit* t;
    while (! (t = r->cast_ptr<Reprise::TranslationUnit>()))
    {
        r = r->getParent();
        if (r == 0) return 0;
    }
    return &t->getGlobals();
}

void makeUniversalAllAddressedCells(SetAbstractMemoryCell samc, ProgramState& state, SetAbstractMemoryCell& examinedCells)
{
    if (samc.isUniversal()) 
    {
        state.makeAllUniversal();
        return;
    }
    else
    {
        for (size_t i = 0 ; i < samc.size(); ++i)
        {
            //если еще не смотрели такую ячейку
            if (examinedCells.find(samc[i]) < 0)
            {
                examinedCells.insert(samc[i]);
                //вызываем эту же функцию для samc по адресу *it
                makeUniversalAllAddressedCells(state.getCellContents(samc[i]), state, examinedCells);
                //делаем универсальным содержимое *it
                state.getCellContentsRef(samc[i]).makeUniversal();
            }
        }
    }
}

//Все параметры вызова процедуры должны быть вхождениями (см. канонизирующее преобразование №2 docs/Montego/преобразования запутанных программ)
void ExpressionAnalyser::visit(OPS::Reprise::SubroutineCallExpression& e)
{
    OPS_ASSERT(m_functionContext != 0);
    OPS_ASSERT(m_programState != 0);
    bool useOffsets = m_analysisContext->options().recognizeStructFields;
    if (m_retCode != 0) return;
    e.getCallExpression().accept(*this);

    //считаем содержимое аргументов
    std::vector<SetAbstractMemoryCell> argumentContent;
    std::vector<SuboffsetsContent> structArgumentContent(e.getArgumentCount());
    std::vector<bool> isStructArg(e.getArgumentCount());
    for (int i = 0; i < e.getArgumentCount(); ++i)
    {
        e.getArgument(i).accept(*this);
        SetAbstractMemoryCell s(m_analysisContext->memoryCellContainer());
        BasicOccurrencePtr o;
        isStructArg[i] = false;
        //если аргумент - вхождение, берем его содержимое. Иначе оставляем s пустой
        if (!m_analysisContext->occurrenceContainer().getOccurBy(e.getArgument(i),o))
        {
            if (o->isReturnStruct().get())
            {
                structArgumentContent[i] = m_structReturnValue;
                isStructArg[i] = true;
            }
            s = getOccurrenceContent(*o, *m_programState, 0);
        }
        argumentContent.push_back(s);
    }

    //анализируем адрес какой функции возвращает выражение в нулевом аргументе
    BasicOccurrencePtr occur;
    std::list<OPS::Reprise::SubroutineDeclaration*> sdeclarations;//их может быть много, если мы точно не узнаем
	if ( m_analysisContext->occurrenceContainer().getOccurBy(e.getCallExpression(), occur) )
    {
        //если здесь, то нулевой аргумент не является вхождением

        //проверяем, является ли нулевой аргумент именем функции
		if (e.hasExplicitSubroutineDeclaration())
        {
			OPS::Reprise::SubroutineDeclaration& sdecl = e.getExplicitSubroutineDeclaration();
            if (DynamicMemoryAllotmentSearchVisitor::isMemAllocFuncName(sdecl.getName()))
            {
                MemoryCellorOffset* m = m_analysisContext->memoryCellContainer().getMemoryCellorOffset(m_functionContext, &e, false, m_analysisContext->options().recognizeStructFields);
                if (m == 0)
                {
                    throw OPS::RuntimeError("Неверно работает функция MemoryCellContainer::getMemoryCell, не находит MemoryCell по заданному SubroutineReferenceExpression функции выделения памяти");
                }
                m_visitReturnValue.clear();
                if (sdecl.getName() == "posix_memalign")
                {
                    BasicOccurrencePtr o;
                    int ret = m_analysisContext->occurrenceContainer().getOccurBy(e.getArgument(0),o);
                    OPS_ASSERT(ret == 0);
                    bool canReplace = (o->getBracketCount()==0);  //нет скобок
                    if (!m_analysisContext->options().recognizeStructFields)
                        canReplace =canReplace && (o->getName().m_fields.size()==0);  //нет обращений к полям структур
                    SetAbstractMemoryCell set_m(m_analysisContext->memoryCellContainer());
                    set_m.insert(m);
                    //cout << "program state before posix_memalign: \n" << m_programState->toString();
                    m_programState->setCellContents(argumentContent[0], set_m, canReplace);
                    //cout << "program state after posix_memalign: \n" << m_programState->toString();
                }
                else
                    m_visitReturnValue.insert(m);
                return;
            }
            if (sdecl.getName() == "memcpy")
            {
                BasicOccurrencePtr o;
                if (m_analysisContext->occurrenceContainer().getOccurBy(e.getArgument(0),o))
                    throw OPS::RuntimeError("ExpressionAnalyser::visit(SubroutineCallExpression): Can't find occurrence for 1st arg of memcpy");
                m_visitReturnValue = copy(o, e.getArgument(1), true);
                return;
            }

			SubroutineType& stype = sdecl.hasDefinition() ? sdecl.getDefinition().getType() : sdecl.getType();
            if ((!stype.isVarArg()) && sdecl.hasDefinition())
            {
				if ( stype.getParameterCount() != e.getArgumentCount() )
                {
                    throw OPS::RuntimeError("Некорректный вызов функции "+sdecl.getName()+" ! Количества параметров вызова и объявления не совпадают! Возможно функция не имеет определения?");
                }
            }
            sdeclarations.push_back(&sdecl);//будет только одна
        }
        else
        {
            throw OPS::RuntimeError("Нулевой аргумент вызова функции не является вхождением. Не выполенено каноническое преобразование №3");
        }
    }
    else
    {
        //нулевой аргумент является вхождением => может получиться несколько возможных процедур
        SetAbstractMemoryCell funcSAMC(m_analysisContext->memoryCellContainer());
        if (!useOffsets)
            funcSAMC = getOccurrenceContent(*occur,*m_programState, 0);
        else
        {
            SetAbstractMemoryCell funcOccur = getOccurrenceAddress(*occur,*m_programState);
            funcOccur = funcOccur.getIntersectedOffsets();
            funcSAMC = m_programState->getCellContents(funcOccur);
        }
        //выбираем из funcSAMC все функции
        //TODO - здесь надо бы провести подробный анализ такой, который бы оставлял только одну функцию
        //TODO - одну функцию все равно оставить не получится. Пример: for... p[i](a,b,c) - массив
        //указателей на функции в цикле
        for (size_t i = 0; i < funcSAMC.size(); ++i)
        {
            OPS::Reprise::RepriseBase* memoryAllotment;
            if (useOffsets)
                memoryAllotment = funcSAMC[i]->cast_to<MemoryCellOffset>().getCellPtr()->getMemoryAllotment();
            else
                memoryAllotment = funcSAMC[i]->cast_to<MemoryCell>().getMemoryAllotment();
            if ( memoryAllotment->is_a<OPS::Reprise::SubroutineDeclaration>() )
            {
                OPS::Reprise::SubroutineDeclaration& sdecl = memoryAllotment->cast_to<OPS::Reprise::SubroutineDeclaration>();
                //проверяем количество параметров (TODO: надо бы еще типы проверять)
                if ( sdecl.getType().getParameterCount() == e.getArgumentCount() || sdecl.getType().isVarArg())
                {
                    bool equalStructTypes = true;
                    if (!sdecl.getType().isVarArg())
                        for (int i = 0; i < e.getArgumentCount(); i++)
                        {
                            if (!isStructArg[i] && sdecl.getType().getParameter(i).getType().is_a<StructType>())      equalStructTypes = false;
                            if (isStructArg[i] && !sdecl.getType().getParameter(i).getType().isEqual(*e.getArgument(i).getResultType()))     equalStructTypes = false;
                        }
                    //добавляем функцию
                    if (equalStructTypes) sdeclarations.push_back(&sdecl);
                }
            }
        }
    }
    if (sdeclarations.size() == 0)
    {
        std::string message = "There is no subroutine, corresponding to expression:" +  e.dumpState() +
                "\nCheck code of your program, may be you forgot initialize pointer to func.\nDebug info: " +
                m_analysisContext->aliasInformationContainer()[occur.get()].toString() +"\n";
        message += "Program state = \n" + m_programState->toString() + "\n";
        throw OPS::RuntimeError(message);
    }
    std::list<OPS::Reprise::SubroutineDeclaration*>::iterator it = sdeclarations.begin();
    ProgramState resultProgramState(m_analysisContext->memoryCellContainer(), m_functionContext, m_analysisContext->options().recognizeStructFields);
    SetAbstractMemoryCell resultVisitReturnValue(m_analysisContext->memoryCellContainer());
    SuboffsetsContent resultReturnedStruct;
    for ( ; it != sdeclarations.end(); ++it)
    {
        ProgramState tempProgramState = *m_programState;
        //обрабатываем функцию
		OPS::Reprise::SubroutineDeclaration* sdecl = *it;
        if (sdecl->hasDefinition()) sdecl = &sdecl->getDefinition();
        m_structReturnValue.clear();

        //если функция не определена, то считаем, что она меняет как угодно все глобальные переменные
        //и все ячейки памяти, связанные со своими параметрами
		if (!sdecl->hasImplementation())
        {
			if (!m_analysisContext->isSafeSubroutine(sdecl))
            {
                //делаем универсальным содержимое глобальных переменных
                Reprise::Declarations* globDecls = getGlobals(e);
                if (globDecls != 0)
                {
					for (Declarations::VarIterator iter = globDecls->getFirstVar(); iter.isValid(); ++iter)
                    {
                        Reprise::VariableDeclaration& v = *iter;
                        MemoryCellorOffset* m = m_analysisContext->memoryCellContainer().getMemoryCellorOffset(m_functionContext, &v, false, m_analysisContext->options().recognizeStructFields);
                        OPS_ASSERT(m!=0);
                        tempProgramState.getCellContentsRef(m).makeUniversal();
                    }
                }
                //делаем универсальным содержимое всех ячеек памяти, связанных с параметрами
                for (size_t i = 0; i < argumentContent.size(); ++i)
                {
                    SetAbstractMemoryCell examinedCells(m_analysisContext->memoryCellContainer());
                    makeUniversalAllAddressedCells(argumentContent[i], tempProgramState, examinedCells);
                }

                //возвращаем универсальный SAMC
                m_visitReturnValue.makeUniversal();
            }
            else
            {
                m_visitReturnValue.clear();//считаем, что функция не возвращает адресных данных
            }
        }
        else
        {
			
            //контекст
            const FunctionContext* funcContext = m_functionContext->getChildContext(&e, *sdecl);
            //составляем внутреннее ProgramState. Инициализирует ячейки формальных параметров в соотв. с фактическими параметрами
            std::unique_ptr<ExpressionAnalyser> ea(new ExpressionAnalyser(m_analysisContext));
            ProgramState innerState(tempProgramState, funcContext, argumentContent, structArgumentContent, m_analysisContext->options().recognizeStructFields, ea.get());

            //запускаем анализ тела функции
            AliasProcedureAnalyzer aliasProcedureAnalyzer(m_analysisContext);
			if (!aliasProcedureAnalyzer.Run(sdecl, funcContext, innerState))
            {
                throw OPS::RuntimeError("Анализ псевдонимов в функции, вызываемой в выражении, почему-то вернул 0");
            }

            //обновляем текущее состояние программы
            tempProgramState.updateFrom(innerState, *sdecl, e);

            //возвращаем SAMC из операторов return внутри функции
            m_visitReturnValue = aliasProcedureAnalyzer.getReturnSAMC();
            //cout << "Return type of function " << sdecl->getName() << " is " << sdecl->getType().getReturnType().dumpState() << "\n";
            if (Editing::desugarType(sdecl->getType().getReturnType()).is_a<StructType>())
            {
                m_structReturnValue = aliasProcedureAnalyzer.getReturnedStruct();
                //cout << "Function returns struct. " << m_structReturnValue.toString();
            }
        }
        resultProgramState.unionWithProgramState(tempProgramState);
        resultVisitReturnValue.unionWith(m_visitReturnValue);
        resultReturnedStruct.unionWith(m_structReturnValue);
    }
    //обновляем SAMC вхождений - параметров
    for (int i = 1; i < e.getChildCount(); ++i)
    {
        //получаем вхождение - i-тый параметр
        BasicOccurrencePtr o;
        if (!m_analysisContext->occurrenceContainer().getOccurBy(e.getChild(i),o))
            //добавляем в SAMC вхождения его псевдонимы в данный момент исполнения программы
            addToOccurrenceSAMC(*o, resultProgramState, m_functionContext);
    }
    m_visitReturnValue = resultVisitReturnValue;
    *m_programState = resultProgramState;
    m_structReturnValue = resultReturnedStruct;
    //cout << "Union of returned structs. " << m_structReturnValue.toString();
}

void ExpressionAnalyser::visit(OPS::Reprise::EmptyExpression&)
{
    if (m_retCode != 0) return;
    m_visitReturnValue.clear();
}

SetAbstractMemoryCell ExpressionAnalyser::getVisitReturnValue()
{
    return m_visitReturnValue;
}

SuboffsetsContent ExpressionAnalyser::getStructReturnValue()
{
    return m_structReturnValue;
}

AliasAnalysisContext& ExpressionAnalyser::getAnalysisContext()
{
    return *m_analysisContext;
}

//возвращает возможные ЗНАЧЕНИЯ данного вхождения по заданному состоянию всех ячеек памяти программы
//применяется только при анализе псевдонимов
SetAbstractMemoryCell ExpressionAnalyser::getOccurrenceContent(BasicOccurrence& o, ProgramState& state, SuboffsetsContent *structReturnValue)
{
    SAMCForOneContext osamc(m_analysisContext->memoryCellContainer());
    osamc = buildOccurrenceSAMC(o, state, structReturnValue);
    return osamc.m_returnValue;
}

//возвращает возможные ЯЧЕЙКИ ПАМЯТИ, в которых храниться содержимое вхождения
//применяется только при анализе псевдонимов
SetAbstractMemoryCell ExpressionAnalyser::getOccurrenceAddress(BasicOccurrence& o, ProgramState& state)
{
    if (o.islValue())
    {
        SAMCForOneContext osamc(m_analysisContext->memoryCellContainer());
        osamc = buildOccurrenceSAMC(o, state);
        return osamc.m_containerCells;
    }
    else
    {
        throw OPS::RuntimeError("You can take address only of l-value! But you take of " + o.toString());
    }
}


//обновляет SAMC вхождения в соответствии с текущим состоянием памяти программы
//возвращает true, если SAMC изменилась, false - не поменялась
//применяется только при анализе псевдонимов
bool ExpressionAnalyser::addToOccurrenceSAMC(BasicOccurrence& o, ProgramState& state, const FunctionContext* functionContext)
{
    //ищем существует ли данный контекст
    bool exists = false;
    OccurrenceSAMC& occurrenceSAMC = m_analysisContext->aliasInformationContainer()[&o];
    OccurrenceSAMC::iterator it;
    for (it = occurrenceSAMC.begin(); it != occurrenceSAMC.end(); ++it)
    {
        if (it->first == functionContext)
        {
            exists = true;
            break;
        }
    }
    if (!exists) //если не существует, добавляем
    {
        occurrenceSAMC.push_back
            (PairForOccurrenceSAMC(functionContext, SAMCForOneContext(m_analysisContext->memoryCellContainer())));
        it = occurrenceSAMC.end();
        it--;
    }
    //строим SAMC для нового состояния
    SAMCForOneContext newSAMC(m_analysisContext->memoryCellContainer());
    newSAMC = buildOccurrenceSAMC(o,state);
    SAMCForOneContext& oldSAMC = it->second;

    //объединяем с имеющимся, по ходу проверяя изменяется ли
    bool samcChanged = oldSAMC.m_dependentCells.unionWith(newSAMC.m_dependentCells);
    samcChanged = oldSAMC.m_containerCells.unionWith(newSAMC.m_containerCells) || samcChanged;
    samcChanged = oldSAMC.m_returnValue.unionWith(newSAMC.m_returnValue) || samcChanged;
    return samcChanged;
}

SetAbstractMemoryCell addAndInsertOffsetIntoContainer(MemoryCellContainer& memoryCellContainer,
                                                      SetAbstractMemoryCell& memOffsets,
                                                      std::list<MemoryCellOffset::ElementaryOffset>& addition)
{
    SetAbstractMemoryCell res(memoryCellContainer);
    if (memOffsets.isUniversal()) {res.makeUniversal(); return res;}
	for(size_t i = 0; i < memOffsets.size(); i++)
    {
        MemoryCellOffset* mo = memOffsets[i]->cast_ptr<MemoryCellOffset>();
        res.insert(mo->addAndInsertOffsetIntoContainer(addition));
    }
    return res;
}

//строит (НО НЕ ЗАПОЛНЯЕТ) SAMC вхождения в соответствии с текущим состоянием памяти программы
//применяется только при анализе псевдонимов
//Терминология:
//Рассмотренная часть вхождения (РЧВх)
//Текущая операция (ТО) - следующая после РЧВх
//PrevMemCells, PrevMemOffsets - m_containerCells для РЧВх (если только РЧВх не непоследняя скобка массива на стеке)
//CurrentMemCells, CurrentMemOffsets - m_returnValue для РЧВх (если только РЧВх не непоследняя скобка массива на стеке)
//CurrentType - тип РЧВх, к которому применяется ТО
//BuildingOffset - текущее накапливаемое смещение внутри PrevMemCells (список ElementaryOffsets)
//DependentCellsorOffsets - ячейки памяти, от которых зависит результат данного вхождения (все кроме m_containerCells)
SAMCForOneContext ExpressionAnalyser::buildOccurrenceSAMC(BasicOccurrence& o, ProgramState& programState, SuboffsetsContent *structReturnValue, bool printDebug)
{
    //cout <<o.toString() <<"\n";
    //if (o.toString() == "node[uint32 0].value")  {cout << "programState.toString() = " << programState.toString() << "\nWe are here!!\n"; printDebug = true; }
    SAMCForOneContext res(m_analysisContext->memoryCellContainer());//результат
    RepriseBase *currentExpression;
    SetAbstractMemoryCell prevMemCellsorOffsets(m_analysisContext->memoryCellContainer()),
            currentMemCellsorOffsets(m_analysisContext->memoryCellContainer()),
            dependentCellsorOffsets(m_analysisContext->memoryCellContainer()),
            connectedToPrev(m_analysisContext->memoryCellContainer());
    TypeBase *currentType;
    std::list<MemoryCellOffset::ElementaryOffset> buildingOffset;
    int currentBracketNumber = 0; //номер текущей и последней скобки у ArrayAccessExpression
    bool useOffsets = m_analysisContext->options().recognizeStructFields;
    //первый родительский узел, который уже не нужно просматривать
    RepriseBase* stopNode = o.isAddress() ? o.getHeadNode() : o.getHeadNode()->getParent();

    //инициализация ===============================================================================================
    //Текущая операция (ТО) - следующая после РЧВх
    currentExpression = o.getRefExpr()->getParent();
    //PrevMemCells, PrevMemOffsets - ячейка памяти, соотв. VariableDeclaration
    prevMemCellsorOffsets.insert(m_analysisContext->memoryCellContainer().getMemoryCellorOffset(m_functionContext, o.getName().m_varDecl, false, useOffsets));
    currentType = &o.getName().m_varDecl->getType();

    if (printDebug)
    {
        std::cout << "buildOccurrenceSAMC for "+ o.toString() +" started. prevMemCellsorOffsets = " << prevMemCellsorOffsets.toString() << std::endl;
        std::cout << "program state:\n" << programState.toString();
    }

    //цикл по структуре вхождения ===============================================================================================
    while (currentExpression != stopNode)
    {
        MemoryCellOffset::ElementaryOffset newElemOffset = MemoryCellOffset::buildElemOffset(currentExpression->cast_to<ExpressionBase>(), currentBracketNumber);
        currentType = &Reprise::Editing::desugarType(*currentType);
        if (printDebug) std::cout << "buildOccurrenceSAMC: we in cycle while. newElemOffset = "+newElemOffset.toString() +
                                     " CurrentTypeIsPtr=" << currentType->is_a<PtrType>() <<
                                     " prevMemCellsorOffsets = " << prevMemCellsorOffsets.toString() << std::endl;
        if (currentType->is_a<PtrType>())
        {
            //всем ячейкам из PrevMemCells, у которых нет смещений, являющихся подмножеством BuildingOffset, добавляем BuildingOffset (и налаживаем связи);
            if (useOffsets) prevMemCellsorOffsets = addAndInsertOffsetIntoContainer(m_analysisContext->memoryCellContainer(), prevMemCellsorOffsets, buildingOffset);
            if (printDebug)
            {
                string strOffsets = "";
                for (std::list<MemoryCellOffset::ElementaryOffset>::iterator it = buildingOffset.begin(); it != buildingOffset.end(); ++it) strOffsets += it->toString();
                std::cout << "buildOccurrenceSAMC: we in cycle while after addAndInsertOffsetIntoContainer. Offsets = "+ strOffsets+
                             " prevMemCellsorOffsets = " << prevMemCellsorOffsets.toString() << std::endl;
            }
            if (useOffsets) connectedToPrev = prevMemCellsorOffsets.getIntersectedOffsets();
            else  connectedToPrev = prevMemCellsorOffsets;
            if (printDebug) cout << "prevMemCellsorOffsets = " << prevMemCellsorOffsets.toString() << ";    connectedToPrev = " << connectedToPrev.toString() << "\n";
            dependentCellsorOffsets.unionWith(connectedToPrev);
            currentMemCellsorOffsets = programState.getCellContents(connectedToPrev);
            if (currentMemCellsorOffsets.isEmpty() && printDebug && useOffsets)
            {
                MemoryCellOffset& mo = prevMemCellsorOffsets[0]->cast_to<MemoryCellOffset>();
                cout << "prevMemCellsorOffsets[0] cell:\n" << mo.getCellPtr()->toStringWithOffsetsAndTheirIntersections();
            }
            prevMemCellsorOffsets = currentMemCellsorOffsets;
            buildingOffset.clear();
        }
        buildingOffset.push_back(newElemOffset);
        //переходим к следующей итерации ===============================================================================================
        if (BasicCallExpression* bce = currentExpression->cast_ptr<BasicCallExpression>())
        {
            //мы рассматривали одну из скобок массива
            if (currentBracketNumber == bce->getChildCount()-2)
            {
                //если текущая скобка последняя, обнуляем счетчик скобок и переходим к следующему выражению
                currentBracketNumber = 0;
                currentExpression = currentExpression->getParent();
                OPS_ASSERT(currentExpression->is_a<StructAccessExpression>() || (currentExpression == stopNode));
            }
            else     currentBracketNumber++;
            currentType = currentType->getChild(0).cast_ptr<TypeBase>();
            OPS_ASSERT(currentType != 0);
        }
        else
        {
            //мы рассматривали поле структуры
            StructAccessExpression& sae = currentExpression->cast_to<StructAccessExpression>();
            currentType = &sae.getMember().getType();
            currentExpression = currentExpression->getParent();
            OPS_ASSERT(currentBracketNumber == 0);
        }
    }
    //возвращаем результат ===============================================================================================
    ReprisePtr<StructType> st = o.isReturnStruct();
    if (useOffsets) prevMemCellsorOffsets = addAndInsertOffsetIntoContainer(m_analysisContext->memoryCellContainer(), prevMemCellsorOffsets, buildingOffset);
    if (printDebug) std::cout << "buildOccurrenceSAMC: before return. prevMemCellsorOffsets = " << prevMemCellsorOffsets.toString() << std::endl;
    res.m_dependentCells = dependentCellsorOffsets;
    if (o.islValue())
    {
        if (useOffsets) connectedToPrev = prevMemCellsorOffsets.getIntersectedOffsets();
        else  connectedToPrev = prevMemCellsorOffsets;
        currentMemCellsorOffsets = programState.getCellContents(connectedToPrev); //(т.к. могли корректировать смещение и не обновить CurrentMemCellsorOffsets)
        res.m_returnValue = currentMemCellsorOffsets;
        res.m_containerCells = prevMemCellsorOffsets;//not connectedToPrev!
    }
    else
        res.m_returnValue = prevMemCellsorOffsets;
    //подправляем результат, если возвращается структура
    if (useOffsets && st.get() && structReturnValue)
    {
        //добавляем к возвращаемому значению содержимое всех внутренних ячеек памяти
        SuboffsetsContent innerOffsetsContent;
        if (!res.m_containerCells.isUniversal())
			for (size_t i = 0; i < res.m_containerCells.size(); i++)
            {
                MemoryCellOffset* mo = res.m_containerCells[i]->cast_ptr<MemoryCellOffset>();
                MemoryCellOffset::OffsetAdditionMap innerOffsets = mo->getInnerOffsetsForStruct(*st);
                for (MemoryCellOffset::OffsetAdditionMap::iterator it = innerOffsets.begin(); it != innerOffsets.end(); ++it)
                {
                    PairForSuboffsetsContent p(it->second, programState.getCellContents(it->first));
                    innerOffsetsContent.push_back(p);
                }
            }
        else
        {
            OPS_ASSERT(!"Еще не реализовано");
            //TODO: что делать, когда вхождению соответствует неизвестно какая ячейка памяти?
        }
        if (structReturnValue) *structReturnValue = innerOffsetsContent;
    }
    if (printDebug && useOffsets)
    {
        cout << "Intersected cells of container cells:\n";
        for (size_t i = 0; i < res.m_containerCells.size(); i++)
        {
            MemoryCellOffset& mo = res.m_containerCells[i]->cast_to<MemoryCellOffset>();
            cout << "Container cell " << mo.toString() << endl;
            SetAbstractMemoryCell samc(m_analysisContext->memoryCellContainer(), mo.getIntersectedOffsets());
            cout << "Intersected cells: " << samc.toString() << endl;
        }
    }
    return res;
}

int ExpressionAnalyser::getRetCode()
{
    return m_retCode;
}

}//end of namespace
}//end of namespace
