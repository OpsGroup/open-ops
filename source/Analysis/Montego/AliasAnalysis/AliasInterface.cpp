#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/AliasAnalysis/AliasImplementation.h"
#include "AliasProcedureAnalyzer.h"
#include "MemoryCellContainer.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "DynamicMemoryAllotmentSearchVisitor.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/Montego/AliasAnalysis/CanonicalForm.h"
#include "Reprise/Reprise.h"
#include "AliasAnalysisContext.h"
#include <set>
#include <iostream>
#include "Shared/StatementsShared.h"
#include "Navigation.h"
#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"
#include "OPS_Core/Kernel.h"

using namespace std;

namespace OPS
{
namespace Montego
{

AliasAnalysisOptions::AliasAnalysisOptions()
    :treatUnknownFuncsAsUnsafe(false)
    ,noPointerCrazyOperations(true)
    ,maximumRecursionDepth(4)
    ,recursionType(RecursionByCall)
    ,recognizeStructFields(true)
    ,debug(false)
    ,meshingIfBranches(false)
    ,cacheProcedureAnalys(false)
{
}

AliasInterface* AliasInterface::create(OPS::Reprise::ProgramUnit& programUnit, OccurrenceContainer& oc)
{
    return new AliasImplementation(programUnit, oc);
}

AliasInterface* AliasInterface::create(OPS::Reprise::ProgramUnit& programUnit, OccurrenceContainer& oc, AliasAnalysisOptions options)
{
    return new AliasImplementation(programUnit, oc, options);
}

AliasImplementation::AliasImplementation(OPS::Reprise::ProgramUnit& programUnit, OccurrenceContainer& oc)
    :m_wasAliasesBuilt(false),
     m_aliasProcedureAnalyzer(0)
{
    AliasAnalysisOptions defaultOpt;
    m_analysisContext = new AliasAnalysisContext(programUnit, &oc, defaultOpt);
}

AliasImplementation::AliasImplementation(OPS::Reprise::ProgramUnit& programUnit, OccurrenceContainer& oc, AliasAnalysisOptions options)
    :m_wasAliasesBuilt(false),
     m_aliasProcedureAnalyzer(0)
{
    m_analysisContext = new AliasAnalysisContext(programUnit, &oc, options);
}

AliasImplementation::~AliasImplementation()
{
    clear();
}

void AliasImplementation::clear()
{
    delete m_aliasProcedureAnalyzer;
    m_aliasProcedureAnalyzer = 0;
    m_wasAliasesBuilt = false;
    delete m_analysisContext;
    m_analysisContext = 0;
    //m_options очищать нельзя!!!!
}

OccurrenceContainer* AliasImplementation::getOccurrenceContainer()
{
    return &m_analysisContext->occurrenceContainer();
}

MemoryCellContainer* AliasImplementation::getMemoryCellContainer()
{
    return &m_analysisContext->memoryCellContainer();
}

//Проводит анализ альясов.
//При этом контейнер вхождений автоматически расширяется до множетсва всех вхождений программы.
//Возвращает 0 - все нормально, 1 - программа не приведена к канонической форме
int AliasImplementation::runAliasAnalysis()
{
    if (m_wasAliasesBuilt == true)
        throw OPS::RuntimeError("Do not use the same AliasImplementation for several alias analysis");
    dword start = OPS::getTickCount();


#if OPS_BUILD_DEBUG
    CanonicalFormChecker check(m_analysisContext->occurrenceContainer());
    if (!check.isFragmentCanonical(*m_analysisContext->occurrenceContainer().getProgramPart())) return 1;
#endif

    //запускаем анализатор альясов для main
    m_aliasProcedureAnalyzer = new AliasProcedureAnalyzer(m_analysisContext);
    //находим следующую функцию для обхода
    SubroutineDeclaration* nextFunc = m_analysisContext->m_navigation.getNextSubroutine();
    while (nextFunc != 0)
    {
        m_aliasProcedureAnalyzer->Run(nextFunc);
        nextFunc = m_analysisContext->m_navigation.getNextSubroutine();
    }
    checkVisitedStmts();

    m_wasAliasesBuilt = true;
    m_analysisContext->occurrenceContainer().addAllComplexOccurrencesIn(*m_analysisContext->occurrenceContainer().getProgramPart(), *this);

    //std::cout << "Alias analysis done in " << (OPS::getTickCount() - start)/1000.0 << " sec." << std::endl;
    //std::cout << toString();

    return 0;
}

//возвращает true, если вхождения могут являться псевдонимами
//false - если точно не являются псевдонимами
//возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
//где была вызвана функция, в теле которой находятся вхождения)
bool AliasImplementation::isAlias(BasicOccurrence& o1, BasicOccurrence& o2)
{
    if (m_wasAliasesBuilt)
    {
        SetAbstractMemoryCell m1(m_analysisContext->memoryCellContainer());
        SetAbstractMemoryCell m2(m_analysisContext->memoryCellContainer());
        if (o1.islValue()) getOccurrenceAddress(o1, m1);
        else getDependentCellsOfOccurrence(o1, m1);
        if (o2.islValue()) getOccurrenceAddress(o2, m2);
        else getDependentCellsOfOccurrence(o2, m2);
        return m1.isIntersectWith(m2, true);
    }
    else
        throw RuntimeError("AliasInterface::isAlias: Aliases was not built, but wanted!");
}

//возвращает набор пар типов зависимостей между вхождениями для заданного контекста вызовов функций
std::list<OccurRolePair> AliasImplementation::getAliasTypes(Occurrence& o1, Occurrence& o2, FunctionContext* funcContext)
{
    std::list<OccurRolePair> result;
    BasicOccurrence* bo1 = o1.cast_ptr<BasicOccurrence>();
    BasicOccurrence* bo2 = o2.cast_ptr<BasicOccurrence>();
    if (bo1 && bo2)
    {
        OccurrenceSAMC osamc1 = getOccurrenceSAMC(*bo1);
        OccurrenceSAMC osamc2 = getOccurrenceSAMC(*bo2);
        if (funcContext == 0)
        {
            //объединяем samc вхождений
            SAMCForOneContext samc1 = osamc1.getUnion();
            SAMCForOneContext samc2 = osamc2.getUnion();
            //проверяем пересекаются ли они и как
            if (samc1.m_containerCells.isIntersectWith(samc2.m_containerCells, true))
            {
                //ячейки памяти, соответствующие данным вхождениям содержат общие части
                if (bo1->isUsage() && bo2->isUsage()) result.push_back(OccurRolePair(USAGE_ROLE, USAGE_ROLE));
            }
        }
    }

    return result;
}

SetAbstractMemoryCell AliasImplementation::getMemoryOfAllOccurences(const VariableDeclaration& variable)
{
    //находим все вхождения, включающие в себя variable
    std::list<BasicOccurrencePtr> occurencesList = m_analysisContext->occurrenceContainer().getAllBasicOccurrencesOf(variable);

    //собираем все ячейки памяти в которых может храниться содержимое этих вхождений
    SetAbstractMemoryCell memorySet(m_analysisContext->memoryCellContainer()), rab(m_analysisContext->memoryCellContainer());
    for (auto it = occurencesList.begin(); it != occurencesList.end(); ++it)
    {
        if ((*it)->islValue())
            getOccurrenceAddress(**it, rab);
        else
            getDependentCellsOfOccurrence(**it, rab);
        memorySet.unionWith(rab);
    }

    return memorySet;
}

//возвращает true, если вхождения могут являться псевдонимами
//false - если точно не являются псевдонимами
//возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
//где была вызвана функция, в теле которой находятся вхождения)
bool AliasImplementation::isAlias(VariableDeclaration& v1, VariableDeclaration& v2)
{
    //TODO: исправить. Если брать только ячейки памяти вхождений, содержащих v1 и v2, то возможна ошибка,
    //когда ячейки содержат одинаковые адреса, они передаются в функцию и там есть псевдонимы. Внутри функции
    //имена переменных - другие, поэтому мы их не отлавливаем.
    //Правильный анализ такой: собираем все ячейки памяти, связанные с v1 и c v2. Добавляем к ним содержимое,
    //содержимое содержимого и т.п. Потом смотрим пересекаются ли полученные множества.

    if (m_wasAliasesBuilt)
    {
        SetAbstractMemoryCell m1 = getMemoryOfAllOccurences(v1);
        SetAbstractMemoryCell m2 = getMemoryOfAllOccurences(v2);

        return m1.isIntersectWith(m2, true);
    }
    else
        throw RuntimeError("AliasInterface::isAlias: Aliases was not built, but wanted!");
}

//удаляет ячейки соответсвующие массивам на стеке, элементы массивов - не трогаем
void removeStaticallyAllocatedCells(SetAbstractMemoryCell& samc, bool useOffsets)
{
    if (samc.isUniversal()) return;
    SetAbstractMemoryCell samc2(*samc.getMemoryCellContainer());
    for (size_t i = 0; i < samc.size(); i++)
    {
        MemoryCell* m = useOffsets ? samc[i]->cast_to<MemoryCellOffset>().getCellPtr() : samc[i]->cast_ptr<MemoryCell>();
        if (!m->isReadOnly()) samc2.insert(samc[i]);
    }
    samc = samc2;
}


// делает черновик draft разбиения вхождений на альясы
// каждому альясу соответствует своя ячейка памяти, draft - и есть данное соответствие
void AliasImplementation::makeDraftAliases(std::vector<BasicOccurrencePtr>& basicOccurrences,
                      std::map <MemoryCellorOffset*, std::set<BasicOccurrencePtrWithGeneratorInformation> >& draft)
{
    BasicOccurrencePtr occur;
    OccurrenceSAMC::iterator itFuncContext;
    //SetAbstractMemoryCell* pSamc;
    std::list<BasicOccurrencePtrWithGeneratorInformation> universalOccurs;
    // цикл по вхождениям внутри текущей функции
    for (int i = 0; i < (int)basicOccurrences.size(); ++i)
    {
        occur = basicOccurrences[i];
        //ячейки памяти, связанные с вхождением, и такие, что
        //в текущем месте программы происходит запись в них
        SetAbstractMemoryCell occurGeneratorMemCells(m_analysisContext->memoryCellContainer());
        //ячейки памяти, связанные с вхождением, и такие, что
        //в текущем месте программы происходит чтение из них
        SetAbstractMemoryCell occurUsageMemCells(m_analysisContext->memoryCellContainer());
        if (!occur->isGenerator())
        {
            if (occur->isUsage())
                //все - в использование
                getCellsConnectedWithOccur(*occur, occurUsageMemCells);
        }
        else
        {
            getOccurrenceAddress(*occur, occurGeneratorMemCells);
            if (occur->isUsage())
                //если вхождение также является еще и использованием
                getCellsConnectedWithOccur(*occur, occurUsageMemCells);
            else
                //добавляем только ячейки, затронутые при вычислении адреса
                getCellsConnectedWithOccur(*occur, occurUsageMemCells, true);
        }
        BasicOccurrencePtrWithGeneratorInformation occurWithInfGeneratorOnly;
        occurWithInfGeneratorOnly.occurrencePtr = occur;
        occurWithInfGeneratorOnly.isGenerator = true;
        occurWithInfGeneratorOnly.isUsage = false;
        BasicOccurrencePtrWithGeneratorInformation occurWithInfUsageOnly;
        occurWithInfUsageOnly.occurrencePtr = occur;
        occurWithInfUsageOnly.isGenerator = false;
        occurWithInfUsageOnly.isUsage = true;
        BasicOccurrencePtrWithGeneratorInformation occurWithInfGenAndUsage;
        occurWithInfGenAndUsage.occurrencePtr = occur;
        occurWithInfGenAndUsage.isGenerator = true;
        occurWithInfGenAndUsage.isUsage = true;

        //обрабатываем универсальные ячейки
        if (occurGeneratorMemCells.isUniversal())
        {
            if (occurUsageMemCells.hasOneElement())
                universalOccurs.push_back(occurWithInfGenAndUsage);
            else
                universalOccurs.push_back(occurWithInfGeneratorOnly);
            continue;
        }
        else
            if (occurUsageMemCells.isUniversal())
            {
                if (occurGeneratorMemCells.hasOneElement())
                    universalOccurs.push_back(occurWithInfGenAndUsage);
                else
                    universalOccurs.push_back(occurWithInfUsageOnly);
                continue;
            }

        removeStaticallyAllocatedCells(occurGeneratorMemCells, m_analysisContext->options().recognizeStructFields);
        removeStaticallyAllocatedCells(occurUsageMemCells, m_analysisContext->options().recognizeStructFields);
        // цикл по множеству ячеек, для которых вхождение - генератор
        for (size_t itMemCell = 0; itMemCell < occurGeneratorMemCells.size(); ++itMemCell)
        {
            //добавляем вхождение во множество псевдонимов данной ячейки памяти
            if (occurUsageMemCells.find(occurGeneratorMemCells[itMemCell]) >= 0)
                draft[occurGeneratorMemCells[itMemCell]].insert(occurWithInfGenAndUsage);
            else
                draft[occurGeneratorMemCells[itMemCell]].insert(occurWithInfGeneratorOnly);
        }

        // цикл по множеству ячеек, для которых вхождение - использование
        for (size_t itMemCell = 0; itMemCell < occurUsageMemCells.size(); ++itMemCell)
        {
            if (occurGeneratorMemCells.find(occurUsageMemCells[itMemCell]) < 0)
                //добавляем вхождение во множество псевдонимов данной ячейки памяти
                draft[occurUsageMemCells[itMemCell]].insert(occurWithInfUsageOnly);
        }
    }
    //добавляем универсальные вхождения ко всем ячейкам памяти
    //TODO: добавлять только к ячейкам памяти текущей функции и к глобальным
    std::map <MemoryCellorOffset*, std::set<BasicOccurrencePtrWithGeneratorInformation> >::iterator it = draft.begin();
    for ( ; it != draft.end(); ++it)
    {
        std::list<BasicOccurrencePtrWithGeneratorInformation>::iterator ito = universalOccurs.begin();
        for ( ; ito != universalOccurs.end(); ++ito)
            it->second.insert(*ito);
    }
}


void deleteDuplicate(std::vector<std::vector<OccurrencePtrWithGeneratorInformation > >& aliases)
{
    std::set<int> toDelete;
    for (size_t i = 0; i < aliases.size(); ++i)
    {
        //проверяем совпадает ли заданный альяс с каким-нибудь из следующих
        for (size_t j = i+1; j < aliases.size(); ++j)
        {
            if (aliases[i].size() == aliases[j].size())
            {
                bool flagDuplicate = true;
                for (size_t k = 0; k < aliases[i].size(); ++k)
                {
                    if (find(aliases[j].begin(),aliases[j].end(),aliases[i][k]) == aliases[j].end())
                    {
                        flagDuplicate = false;
                        break;
                    }
                }

                if (flagDuplicate)
                {
                    toDelete.insert(i);
                    break;
                }
            }
        }
    }
    if (toDelete.size() > 0)
    {
        std::vector<std::vector<OccurrencePtrWithGeneratorInformation > > newAliases(aliases.size() - toDelete.size());
        for (size_t i = 0, j = 0; i < aliases.size(); ++i)
        {
            //если не надо удалять, то переносим в новую структуру
            if (toDelete.find(i) == toDelete.end())
            {
                newAliases[j] = aliases[i];
                ++j;
            }
        }
        aliases.swap(newAliases);
    }
}


//возвращает вектор альясов, состоящих из вхождений, принадлежащих данной
//части кода программы (возвращаются вхождения только из текущей функции),
//Одно вхождение может включаться в несколько множеств.
//память необходимо отчистить после использования!
std::vector<std::vector<OccurrencePtrWithGeneratorInformation > >
    AliasImplementation::getOccursByAlias(OPS::Reprise::RepriseBase& code)
{
    if (!m_wasAliasesBuilt) throw OPS::RuntimeError("Анализ альясов проведен не был, но функция getOccurByAlias вызывается!");
    std::map <MemoryCellorOffset*, std::set<BasicOccurrencePtrWithGeneratorInformation> > draftMain;
    std::vector<BasicOccurrencePtr> basicOccurrences;
    std::vector<ComplexOccurrencePtr> complexOccurrences;

    //строим черновик альясов для вхождений в текущей функции
    m_analysisContext->occurrenceContainer().getAllOccurrencesIn(&code, basicOccurrences, complexOccurrences);
    makeDraftAliases(basicOccurrences, draftMain);

    //для каждого вызова функции строим черновик альясов для внутренних вхождений
    std::vector< std::map <MemoryCellorOffset*, std::set<BasicOccurrencePtrWithGeneratorInformation> > >
        draftSubrCalls(complexOccurrences.size());
    for (int i = 0; i < (int)complexOccurrences.size(); ++i)
    {
        //берем вхождения из функции
        std::vector<BasicOccurrencePtr> occurs;
        std::vector<ComplexOccurrencePtr> complOccurs;
        std::list<OPS::Reprise::SubroutineDeclaration*> sdecls =
            getAllPossibleSubroutinesByPointer(*(complexOccurrences[i]->getSubroutineCall()));
        std::list<OPS::Reprise::SubroutineDeclaration*>::iterator it = sdecls.begin();
        for ( ; it != sdecls.end(); ++it)
        {
            if ((*it)->hasDefinition())
            {
                Reprise::SubroutineDeclaration* sdecl = &(*it)->getDefinition();
                m_analysisContext->occurrenceContainer().getAllOccurrencesIn(&(sdecl->getBodyBlock()), occurs, complOccurs);
                makeDraftAliases(occurs,draftSubrCalls[i]);
            }
            else
            {
                //считаем, что функция меняет все глобальные переменные неизвестным образом
                //OPS_ASSERT(!"Еще не реализовано!");
            }
        }
    }

    //считаем для каждой ячейки памяти в скольких функциях она встречается
    //нужны будут только те ячейки, которые встречаются в главной программе или в более чем 2х функциях
    std::map<MemoryCellorOffset*, int> memCellFuncCount;
    std::map <MemoryCellorOffset*, std::set<BasicOccurrencePtrWithGeneratorInformation> >::iterator itDraft;
    for (itDraft = draftMain.begin(); itDraft != draftMain.end(); ++itDraft)
    {
        memCellFuncCount[itDraft->first] = 2;
    }
    for (int i = 0; i < (int)complexOccurrences.size(); ++i)
    {
        for (itDraft = draftSubrCalls[i].begin(); itDraft != draftSubrCalls[i].end(); ++itDraft)
        {
            memCellFuncCount[itDraft->first] += 1;
        }
    }

    //считаем кол-во альясов
    std::map<MemoryCellorOffset*,int>::iterator itCount;
    int aliasCount = 0;
    for (itCount = memCellFuncCount.begin(); itCount != memCellFuncCount.end(); ++itCount)
    {
        if (itCount->second >= 1) ++aliasCount;
    }

    // перестраиваем черновики в возвращаемое значение
    std::vector< std::vector<OccurrencePtrWithGeneratorInformation> > resMain(aliasCount);
//    std::vector<std::map<ComplexOccurrence*, std::set<BasicOccurrence*> > > resSCalls(aliasCount);
    int alias_i = 0;
    for (itCount = memCellFuncCount.begin(); itCount != memCellFuncCount.end(); ++itCount)
    {
        if (itCount->second >= 1)
        {
            //берем из главной программы
            resMain[alias_i] = std::vector<OccurrencePtrWithGeneratorInformation>(draftMain[itCount->first].begin(), draftMain[itCount->first].end());

            //берем из функций
            for (int i = 0; i < (int)complexOccurrences.size(); ++i)
            {
                if (draftSubrCalls[i][itCount->first].size() > 0)
                {
                    std::vector<BasicOccurrencePtrWithGeneratorInformation> vecbo(draftSubrCalls[i][itCount->first].begin(),draftSubrCalls[i][itCount->first].end());
                    std::vector<BasicOccurrencePtr> vecbo0(vecbo.size());
                    for (size_t rabi=0; rabi<vecbo.size(); rabi++) vecbo0[rabi] = vecbo[rabi].occurrencePtr;
                    ComplexOccurrencePtr co(new ComplexOccurrence(*(complexOccurrences[i]->getSubroutineCall()), vecbo0));
                    //добавляем информацию о генераторе или использовании
                    OccurrencePtrWithGeneratorInformation oWithInf;
                    oWithInf.occurrencePtr = co;
                    //проверяем наличие генераторов
                    bool generatorsExits = false;
                    for (size_t rabi=0; rabi<vecbo.size(); rabi++)
                        if (vecbo[rabi].isGenerator) {generatorsExits=true; break;}
                    //проверяем наличие использований
                    bool usageExits = false;
                    for (size_t rabi=0; rabi<vecbo.size(); rabi++)
                        if (vecbo[rabi].isUsage) {usageExits=true; break;}
                    oWithInf.isGenerator = generatorsExits;
                    oWithInf.isUsage = usageExits;
                    //добавляем в альясы в главной функции составное вхождение
                    resMain[alias_i].push_back(oWithInf);

                    //добавляем элементарные вхождения из составного
//                    resSCalls[alias_i][complexOccurrences[i]] = draftSubrCalls[i][itCount->first];
                }
            }
            ++alias_i;
        }
    }
    //удаляем случайно совпавшие множества
    deleteDuplicate(resMain);
    addFuncAliasesWithSameGlobalVariable(resMain, complexOccurrences);
    return resMain;
}

//возвращает множество вхождений-псевдонимов данного вхождения внутри code
//если code==0, то для всей программы
//возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
//где была вызвана функция, в теле которой находятся вхождения)
//если в code содержатся вызовы функций, то псевдонимы возвращаются и из этих функций
std::list<BasicOccurrencePtr> AliasImplementation::getBasicOccurrencesAliasedWith(BasicOccurrence& o, OPS::Reprise::RepriseBase* code)
{
    if (!m_wasAliasesBuilt)
        throw OPS::RuntimeError("Анализ альясов проведен не был, но функция getOccurrencesAliasedWith вызывается!");
    SetAbstractMemoryCell mo(m_analysisContext->memoryCellContainer());
    getOccurrenceAddress(o,mo);
    return getOccurrencesAliasedWithHelper(mo,code);
}

//возвращает множество вхождений-псевдонимов данного вхождения внутри code
//если code==0, то для всей программы
//возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
//где была вызвана функция, в теле которой находятся вхождения)
std::list<BasicOccurrencePtr> AliasImplementation::getBasicOccurrencesAliasedWith(Reprise::VariableDeclaration& v, OPS::Reprise::RepriseBase* code)
{
    if (!m_wasAliasesBuilt)
        throw OPS::RuntimeError("Анализ альясов проведен не был, но функция getOccurrencesAliasedWith вызывается!");
    SetAbstractMemoryCell samc = m_analysisContext->memoryCellContainer().getAllCellsOf(v, m_analysisContext->options().recognizeStructFields);
    return getOccurrencesAliasedWithHelper(samc, code);
}

//возвращает множество вхождений-псевдонимов имеющих адрес из samc внутри code
//если code==0, то для всей программы
//возвращается контекстно НЕЗАВИСИМАЯ информация
std::list<BasicOccurrencePtr> AliasImplementation::getOccurrencesAliasedWithHelper(SetAbstractMemoryCell& mo, OPS::Reprise::RepriseBase* code)
{
    if (!m_wasAliasesBuilt)
        throw OPS::RuntimeError("Анализ альясов проведен не был, но функция getOccurrencesAliasedWith вызывается!");
    std::list<BasicOccurrencePtr> result;
    std::list<OPS::Reprise::ReferenceExpression*> refList;
    std::list<OPS::Reprise::ReferenceExpression*>::iterator refListIt;
    std::map <OPS::Reprise::ReferenceExpression*, BasicOccurrence>::iterator itOccur;

    std::vector<BasicOccurrencePtr> occurs = m_analysisContext->occurrenceContainer().getDeepAllBasicOccurrencesIn(code, this);

    // цикл по вхождениям
    for (size_t i = 0; i < occurs.size(); ++i)
    {
        BasicOccurrencePtr occur = occurs[i];
        if (occur->islValue())
        {
            SetAbstractMemoryCell m(m_analysisContext->memoryCellContainer());
            getOccurrenceAddress(*occur,m);
            if (m.isIntersectWith(mo, true))  result.push_back(occur);
        }
    }
    return result;
}

bool AliasImplementation::isVariableChanged(BasicOccurrence& o, OPS::Reprise::RepriseBase* code)
{
    std::list<BasicOccurrencePtr> occurs = getBasicOccurrencesAliasedWith(o, code);
    for (std::list<BasicOccurrencePtr>::iterator it = occurs.begin(); it != occurs.end(); ++it)
    {
        BasicOccurrencePtr o = *it;
        if (o->isGenerator()) return true;
    }
    return false;
}

bool AliasImplementation::isVariableChanged(Reprise::VariableDeclaration& v, OPS::Reprise::RepriseBase* code)
{
    std::list<BasicOccurrencePtr> occurs = getBasicOccurrencesAliasedWith(v, code);
    for (std::list<BasicOccurrencePtr>::iterator it = occurs.begin(); it != occurs.end(); ++it)
    {
        BasicOccurrencePtr o = *it;
        if (o->isGenerator()) return true;
    }
    return false;
}

bool AliasImplementation::isFortranAliases(Reprise::VariableDeclaration& v)
{
    std::list<BasicOccurrencePtr> occurs = getBasicOccurrencesAliasedWith(v);
    for (std::list<BasicOccurrencePtr>::iterator it = occurs.begin(); it != occurs.end(); ++it)
    {
        BasicOccurrencePtr o = *it;
        if (o->getName().m_varDecl != &v) return false;
    }
    return true;
}

bool AliasImplementation::isFortranAliases()
{
    OPS_ASSERT(!"TODO");
    return true;
}

std::string AliasImplementation::toString(BasicOccurrence& o)
{
    if (!m_wasAliasesBuilt) return "Alias analysis was not run! Use AliasInterface::runAliasAnalysis";
    return m_analysisContext->aliasInformationContainer()[&o].toString();
}

std::string AliasImplementation::toString(bool onlyNonEmpty)
{
    if (!m_wasAliasesBuilt) return "Alias analysis was not run! Use AliasInterface::runAliasAnalysis";
    return m_analysisContext->aliasInformationContainer().toString(onlyNonEmpty);
}

//возвращает возможные функциии, которые могут содержаться в данном указателе на функцию
//применяется при определении, какая функция вызывается в данном месте программы
//для вхождений из контейнера должен быть проведен анализ псевдонимов, иначе выскочит исключение
std::list<OPS::Reprise::SubroutineDeclaration*> AliasImplementation::getAllPossibleSubroutinesByPointer(OPS::Reprise::SubroutineCallExpression& e)
{
    if (!m_wasAliasesBuilt)
        throw OPS::RuntimeError("Функция getAllPossibleSubroutinesByPointer вызывается с непроведенным анализом псевдонимов!");
    //анализируем адрес какой функции возвращает выражение в нулевом аргументе
    OPS::Montego::BasicOccurrencePtr occur;
    OPS::Montego::MemoryCellContainer* memoryCellContainer = &m_analysisContext->memoryCellContainer();
    std::list<OPS::Reprise::SubroutineDeclaration*> sdeclarations;//их может быть много, если мы точно не узнаем
    if ( m_analysisContext->occurrenceContainer().getOccurBy(e.getCallExpression(), occur) )
    {
        //если здесь, то нулевой аргумент не является вхождением

        //проверяем, является ли вызовом определенной функции
        if (e.hasExplicitSubroutineDeclaration())
        {
            OPS::Reprise::SubroutineDeclaration& sdecl = e.getExplicitSubroutineDeclaration();
            //отсеиваем функции выделения памяти
            if (OPS::Montego::DynamicMemoryAllotmentSearchVisitor::isMemAllocFuncName(sdecl.getName()))
            {
                //данная функция - это функция выделения памяти. Возвращаем пустое множетсво.
                return sdeclarations;
            }
            SubroutineType& sType = sdecl.hasDefinition() ? sdecl.getDefinition().getType() : sdecl.getType();
            if (( sType.getParameterCount() != e.getArgumentCount() ) && (!sType.isVarArg()))
            {
                std::cout << "Wrong call of function " + sdecl.getName() + "! Param numbers of call and definition are not equal! May be function doesn't have definition. It will be exception in release.\n";
#if !OPS_BUILD_DEBUG
                throw OPS::RuntimeError("Некорректный вызов функции "+sdecl.getName()+" ! Количества параметров вызова и объявления не совпадают! Возможно функция не имеет определения?");
#endif
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

        OPS::Montego::SetAbstractMemoryCell funcSAMC(*memoryCellContainer);
        getOccurrenceContent(*occur,funcSAMC);
        //выбираем из funcSAMC все функции
        //TODO - здесь надо бы провести подробный анализ такой, который бы оставлял только одну функцию
        //TODO - одну функцию все равно оставить не получится. Пример: for... p[i](a,b,c) - массив
        //указателей на функции в цикле
        for (size_t it = 0; it < funcSAMC.size(); ++it)
        {
            OPS::Reprise::RepriseBase* memoryAllotment;
            if (m_analysisContext->options().recognizeStructFields)
                memoryAllotment = funcSAMC[it]->cast_to<MemoryCellOffset>().getCellPtr()->getMemoryAllotment();
            else
                memoryAllotment = funcSAMC[it]->cast_to<MemoryCell>().getMemoryAllotment();
            if ( memoryAllotment->is_a<OPS::Reprise::SubroutineDeclaration>() )
            {
                OPS::Reprise::SubroutineDeclaration& sdecl = memoryAllotment->cast_to<OPS::Reprise::SubroutineDeclaration>();
                //проверяем количество параметров (TODO: надо бы еще типы проверять)
                if ( sdecl.getType().getParameterCount() == e.getArgumentCount() )
                {
                    //добавляем функцию
                    sdeclarations.push_back(&sdecl);
                }
            }
        }
    }
    return sdeclarations;
}

///Только для тестирования анализатора псевдонимов: SAMC вхождения - множество ячеек памяти, которые соответствуют ему (см. описание в docs/Анализ альясов )
OccurrenceSAMC AliasImplementation::getOccurrenceSAMC(BasicOccurrence& o)
{
    return m_analysisContext->aliasInformationContainer()[&o];
}

//возвращает возможные ЗНАЧЕНИЯ данного вхождения по построенному OccurrenceSAMC
void AliasImplementation::getOccurrenceContent(BasicOccurrence& o, SetAbstractMemoryCell& result)
{
    result.clear();
    OccurrenceSAMC& occurrenceSAMC = m_analysisContext->aliasInformationContainer()[&o];
    SAMCForOneContext un = occurrenceSAMC.getUnion();
    result = un.m_returnValue;
}

//находит возможные ячейки памяти, в которых может храниться данное вхождение
//поиск производится по построенному OccurrenceSAMC
//возвращается контексно независимая информация
void AliasImplementation::getOccurrenceAddress(BasicOccurrence& o, SetAbstractMemoryCell& result)
{
    result.clear();
    if (!o.islValue())
        throw OPS::RuntimeError("You are trying get address of not l-value occurrence: " + o.toString());
    OccurrenceSAMC& occurrenceSAMC = m_analysisContext->aliasInformationContainer()[&o];
    SAMCForOneContext un = occurrenceSAMC.getUnion();
    result = un.m_containerCells;
}

//находит возможные ячейки памяти, отличные от content и address, от содержимого
//которых зависит содержимое данного вхождения
void AliasImplementation::getDependentCellsOfOccurrence(BasicOccurrence& o, SetAbstractMemoryCell& result)
{
    result.clear();
    OccurrenceSAMC& occurrenceSAMC = m_analysisContext->aliasInformationContainer()[&o];
    SAMCForOneContext un = occurrenceSAMC.getUnion();
    result = un.m_dependentCells;
}

//находит возможные ячейки памяти, в которых может храниться данное вхождение
//добавляет к ним все ячейки памяти, затронутые операцией [] при вычислении значения вхождения
//содержимое вхождения НЕ ВКЛЮЧАЕТСЯ!!!
//поиск производится по построенному OccurrenceSAMC
//возвращается контексно независимая информация
void AliasImplementation::getCellsConnectedWithOccur(BasicOccurrence& o, SetAbstractMemoryCell& result, bool onlyDependentCells)
{
    result.clear();
    if (!o.islValue())
        onlyDependentCells = true;
    OccurrenceSAMC& occurrenceSAMC = m_analysisContext->aliasInformationContainer()[&o];
    SAMCForOneContext un = occurrenceSAMC.getUnion();
    if (!onlyDependentCells) result = un.m_containerCells;
    result.unionWith(un.m_dependentCells);
}

//выясняет, живет ли ячейка памяти вне указанного блока
void AliasImplementation::isMemoryCellPrivate(const OPS::Reprise::BlockStatement& block,
                                         SetAbstractMemoryCell& memCells,
                                         valarray<bool>& result,
                                         SetAbstractMemoryCell& examinedCells)
{
    AliasAnalysisOptions options = m_analysisContext->options();
    if (options.debug) cout << "Start isMemoryCellPrivate for cells: " << memCells.toString() << endl;
    result.resize(memCells.size(), true);
    //строим множество вхождений вне рассматриваемого блока
    vector<BasicOccurrencePtr> allOccurs0 = m_analysisContext->occurrenceContainer().getAllBasicOccurrencesIn(block.findTranslationUnit());
    set<BasicOccurrencePtr> allOccurs(allOccurs0.begin(), allOccurs0.end());
    vector<BasicOccurrencePtr> blockOccurs0 = m_analysisContext->occurrenceContainer().getAllBasicOccurrencesIn(&block);
    set<BasicOccurrencePtr> blockOccurs(blockOccurs0.begin(), blockOccurs0.end());
    set<BasicOccurrencePtr>::iterator it = blockOccurs.begin();
    for (; it != blockOccurs.end(); ++it)
    {
        allOccurs.erase(allOccurs.find(*it));
    }
    examinedCells.unionWith(memCells);
    for (size_t i=0; i<memCells.size(); i++)
    {
        //для каждой ячейки памяти проверяем, встречается ли она вне блока
        //universal не учитываем!!!!!!
        SetAbstractMemoryCell memCell(m_analysisContext->memoryCellContainer());
        memCell.insert(memCells[i]);
        for (it = allOccurs.begin(); it != allOccurs.end(); ++it)
        {
            SetAbstractMemoryCell samc(m_analysisContext->memoryCellContainer());
            getCellsConnectedWithOccur(**it, samc);
            if ((!samc.isUniversal()) && samc.isIntersectWith(memCell, true))
            {
                result[i] = false;
                break;
            }
        }
        if (result[i] == false) continue;

        //проверяем, возможно ли использование данной ячейки памяти при повторном выполнении данного блока
        RepriseBase* ma;
        if (options.recognizeStructFields)
            ma = memCells[i]->cast_to<MemoryCellOffset>().getCellPtr()->getMemoryAllotment();
        else
            ma = memCells[i]->cast_to<MemoryCell>().getMemoryAllotment();
        VariableDeclaration* v = ma->cast_ptr<VariableDeclaration>();
        if (v != 0)
        {
            if (!v->hasDefinedBlock() || !Shared::contain(&block, &v->getDefinedBlock()))
                result[i] = false;
            continue;
        }
        //если это malloc
        //берем все ячейки памяти из блока, содержащие этот malloc
        SetAbstractMemoryCell memCellsRecur(m_analysisContext->memoryCellContainer());
        for (size_t j = 0; j < blockOccurs0.size(); j++)
        {
            OccurrenceSAMC osamc = getOccurrenceSAMC(*(blockOccurs0[j]));
            OccurrenceSAMC::iterator it;
            for (it = osamc.begin(); it != osamc.end(); ++it)
            {
                if (it->second.m_returnValue.isIntersectWith(memCell, true))
                    memCellsRecur.unionWith(it->second.m_containerCells);
                if (it->second.m_containerCells.isIntersectWith(memCell, true))
                    memCellsRecur.unionWith(it->second.m_dependentCells);
            }
        }
        if (options.debug) cout << "Cells " << memCellsRecur.toString() << " contain examined cell: "
                                  << memCells[i]->toString() << " and should be examined recursively" << endl;
        //запрещаем бесконечную рекурсию, выкидывая уже рассмотренные ячейки
		for (size_t j = 0; j < examinedCells.size(); j++)
        {
            int tmp = memCellsRecur.find(examinedCells[j]);
            if (tmp>=0) memCellsRecur.erase(tmp);
        }
        //if (examinedCells.isIntersectWith(memCellsRecur)) {result[i] = false; continue;}
        //запускаем для них рекурсивно эту же функцию
        valarray<bool> resRecur;
        isMemoryCellPrivate(block, memCellsRecur, resRecur, examinedCells);
        //если хотя бы одна не является приватной, то возвращаем false
        for (size_t j = 0; j < resRecur.size(); j++)
            if (resRecur[j] == false) {result[i] = false; break;}
    }
}

//выясняет, живет ли ячейка памяти, соответствующая вхождению, вне указанного блока
//отвечает на вопрос: может ли данное вхождение быть связанным с чем-то циклически порожденными дугами относительно указанного тела цикла
bool AliasImplementation::isOccurrencePrivate(const OPS::Reprise::BlockStatement& block, BasicOccurrencePtr& o)
{
    SetAbstractMemoryCell samc(m_analysisContext->memoryCellContainer()), examinedCells(m_analysisContext->memoryCellContainer());
    if (o->islValue())
        getOccurrenceAddress(*o, samc);
    else
        getCellsConnectedWithOccur(*o, samc, true);
    if (samc.isUniversal()) return false;
    bool res = true;
    valarray<bool> r;
    isMemoryCellPrivate(block, samc, r, examinedCells);
    for (size_t i=0; i<r.size(); i++)
        res = res && r[i];
    return res;
}

//для тестирования
std::map<OPS::Reprise::StatementBase*, int> AliasImplementation::getStmtVisitsCount()
{
    std::map<OPS::Reprise::StatementBase*, int> result;

    std::map<const ExpressionBase*, int>::iterator it = m_analysisContext->m_exprVisitsCount.begin();

    for(; it != m_analysisContext->m_exprVisitsCount.end(); ++it)
    {
        const ExpressionBase* expr = it->first;
        StatementBase& stmt = expr->getParent()->cast_to<StatementBase>();

        if (ForStatement* forStmt = stmt.cast_ptr<ForStatement>())
        {
            if (&forStmt->getFinalExpression() == expr)
                result[&stmt] = it->second;
        }
        else
            result[&stmt] = it->second;
    }

    return result;
}

AliasInterfaceMeta::AliasInterfaceMeta(OPS::Shared::ProgramContext &context)
{
    AliasAnalysisOptions defaultOpt;
    m_interface.reset(new AliasImplementation(context.getProgram(), *context.getMetaInformationSafe<OccurrenceContainerMeta>().getContainer(), defaultOpt));
}

std::tr1::shared_ptr<AliasInterface> AliasInterfaceMeta::getInterface() const
{
    return m_interface;
}

const char* AliasInterfaceMeta::getUniqueId() const
{
    return UNIQUE_ID;
}

std::vector<const char*> AliasInterfaceMeta::getDependencies() const
{
    std::vector<const char*> deps;
    deps.push_back(OccurrenceContainerMeta::UNIQUE_ID);
    return deps;
}

const char* const AliasInterfaceMeta::UNIQUE_ID = "AliasInterface";

OccurrencePtrWithGeneratorInformation::OccurrencePtrWithGeneratorInformation()
{
}

OccurrencePtrWithGeneratorInformation::OccurrencePtrWithGeneratorInformation(const BasicOccurrencePtrWithGeneratorInformation& o)
{
    occurrencePtr = o.occurrencePtr;
    isGenerator = o.isGenerator;
    isUsage = o.isUsage;
}

bool operator <(const BasicOccurrencePtrWithGeneratorInformation& o1, const BasicOccurrencePtrWithGeneratorInformation& o2)
{
    if (o1.occurrencePtr.get() < o2.occurrencePtr.get())
        return true;

    if (o1.occurrencePtr.get() == o2.occurrencePtr.get())
    {
        if (o1.isGenerator < o2.isGenerator)
            return true;
        if (o1.isGenerator == o2.isGenerator)
        {
            return o1.isUsage < o2.isUsage;
        }
    }
    return false;
}

bool operator ==(const BasicOccurrencePtrWithGeneratorInformation& o1, const BasicOccurrencePtrWithGeneratorInformation& o2)
{
    return o1.occurrencePtr.get() == o2.occurrencePtr.get() &&
        o1.isGenerator == o2.isGenerator &&
        o1.isUsage == o2.isUsage;
}

bool operator ==(const OccurrencePtrWithGeneratorInformation& o1, const OccurrencePtrWithGeneratorInformation& o2)
{
    return o1.occurrencePtr.get() == o2.occurrencePtr.get() &&
        o1.isGenerator == o2.isGenerator &&
        o1.isUsage == o2.isUsage;
}

bool isIntersect(set<SubroutineDeclaration*>* a, set<SubroutineDeclaration*>* b)
{
    if (a->empty() || b->empty()) return false;
    if (a->size() < b->size())
    {
        set<SubroutineDeclaration*>* p = a;
        a = b;
        b = p;
    }
    set<SubroutineDeclaration*>::iterator it = b->begin();
    for ( ; it != b->end(); ++it)
        if (a->find(*it) != a->end()) return true;

    return false;
}

void AliasImplementation::addFuncAliasesWithSameGlobalVariable(std::vector< std::vector<OccurrencePtrWithGeneratorInformation> >& aliases,
                                                          std::vector<ComplexOccurrencePtr>& complexOccurrences)
{
    size_t newAliasNum = m_analysisContext->options().funcSetsWithSameGlobalVariable.size();
    if (newAliasNum == 0) return;
    vector<list<ComplexOccurrencePtr> > newAliases(newAliasNum);
    for (size_t j = 0; j < complexOccurrences.size(); j++)
    {
        list<SubroutineDeclaration*> occurSubrs = getAllPossibleSubroutinesByPointer(*(complexOccurrences[j]->getSubroutineCall()));
        set<SubroutineDeclaration*> occurSubrsSet(occurSubrs.begin(), occurSubrs.end());
        for (size_t i = 0; i < newAliasNum; i++)
        {
            const set<SubroutineDeclaration*>& funcSet = m_analysisContext->options().funcSetsWithSameGlobalVariable[i];
            if (isIntersect((set<SubroutineDeclaration*>*) &funcSet, &occurSubrsSet))
                newAliases[i].push_back(complexOccurrences[j]);
        }
    }
    //add built aliases to result array
    for (size_t i = 0; i < newAliasNum; i++)
        if (!newAliases[i].empty())
        {
            std::vector<OccurrencePtrWithGeneratorInformation> newAlias(newAliases[i].size());
            list<ComplexOccurrencePtr>::iterator it = newAliases[i].begin();
            for (size_t k = 0; k < newAliases[i].size(); k++, ++it)
            {
                OccurrencePtrWithGeneratorInformation o;
                o.occurrencePtr = *it;
                o.isGenerator = true;
                o.isUsage = true;
                newAlias[k] = o;
            }
            aliases.push_back(newAlias);
        }
}

//проверяет, обошел ли анализатор все операторы программы
class AliasProcedureAnalyzerChecker : public OPS::Reprise::Service::DeepWalker
{
public:
    explicit AliasProcedureAnalyzerChecker(std::map<const OPS::Reprise::ExpressionBase*, int>& visitedExprs)
        :m_visitedExprs(visitedExprs)
    {
    }

    std::vector<ExpressionBase*> m_errorExprs;

    std::map<const OPS::Reprise::ExpressionBase*, int>& m_visitedExprs;

    void checkExpr(ExpressionBase& expr)
    {
        if (m_visitedExprs.find(&expr) == m_visitedExprs.end() &&
                !expr.is_a<EmptyExpression>())
        {
            m_errorExprs.push_back(&expr);
        }
    }

    void visit(ExpressionStatement& s)
    {
        checkExpr(s.get());
    }
    void visit(ForStatement& s)
    {
        checkExpr(s.getInitExpression());
        checkExpr(s.getFinalExpression());
        checkExpr(s.getStepExpression());
        DeepWalker::visit(s);
    }
    void visit(WhileStatement& s)
    {
        checkExpr(s.getCondition());
        DeepWalker::visit(s);
    }
    void visit(IfStatement& s)
    {
        checkExpr(s.getCondition());
        DeepWalker::visit(s);
    }
    void visit(PlainSwitchStatement& s)
    {
        checkExpr(s.getCondition());
        DeepWalker::visit(s);
    }
    void visit(ReturnStatement& s)
    {
        checkExpr(s.getReturnExpression());
    }
};



void AliasImplementation::checkVisitedStmts()
{
#if OPS_BUILD_DEBUG
    AliasProcedureAnalyzerChecker ch(m_analysisContext->m_exprVisitsCount);
    ProgramStateForProcs::KeyToHashCodes::const_iterator it = m_analysisContext->visitedProcs().Begin();
    AliasCheckerException exception;
    for ( ; it != m_analysisContext->visitedProcs().End(); ++it)
    {
        SubroutineDeclaration* subDeclaration = it->first;
        ch.m_errorExprs.clear();
        subDeclaration->getBodyBlock().accept(ch);
        if (!ch.m_errorExprs.empty())
        {
            for(size_t expr = 0; expr < ch.m_errorExprs.size(); ++expr)
            {
                exception.addNotVisitedExpr(std::make_pair(ch.m_errorExprs[expr], subDeclaration));
            }
        }
    }
    set<SubroutineDeclaration*>::iterator it2 = m_analysisContext->m_navigation.m_notVisitedSubroutines.begin();
    for ( ; it2 != m_analysisContext->m_navigation.m_notVisitedSubroutines.end(); ++it2)
    {
        exception.addNotVisitedSubr(*it2);
    }

    if (exception.isError())
    {
        std::cout << exception.getMessage();
        throw exception;
    }
#endif
}

AliasAnalysisOptions AliasImplementation::options()
{
    return m_analysisContext->options();
}

}//end of namespace

namespace Morant // Yet another bay in Jamaica for future new implementation
{
}//end of namespace

}//end of namespace
