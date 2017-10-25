#pragma once

#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"

namespace OPS
{
namespace Montego
{

class AliasProcedureAnalyzer;
class SetAbstractMemoryCell;
class MemoryCellContainer;
class AliasAnalysisContext;
class AliasInformationContainer;

class AliasImplementation : public AliasInterface
                          , public OPS::NonCopyableMix
{
public :
    explicit AliasImplementation(OPS::Reprise::ProgramUnit& programUnit, OccurrenceContainer& oc);
    explicit AliasImplementation(Reprise::ProgramUnit &programUnit, OccurrenceContainer &oc, AliasAnalysisOptions options);

    ~AliasImplementation();

    bool wasBuilt() const { return m_wasAliasesBuilt; }

    void clear();

    //Проводит анализ альясов. В programUnit ищется функция main и запускается анализатор.
    //При этом контейнер вхождений автоматически расширяется до множетсва всех вхождений программы.
    //Возвращает 0 - все нормально, 1 - программа не приведена к канонической форме
    int runAliasAnalysis();


    //возвращает true, если вхождения могут являться псевдонимами
    //false - если точно не являются псевдонимами
    //возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
    //где была вызвана функция, в теле которой находятся вхождения)
    bool isAlias(BasicOccurrence& o1, BasicOccurrence& o2);

    //возвращает набор пар типов зависимостей между вхождениями для заданного контекста вызовов функций
    std::list<OccurRolePair> getAliasTypes(Occurrence& o1, Occurrence& o2, FunctionContext* funcContext);

    //возвращает true, если вхождения могут являться псевдонимами
    //false - если точно не являются псевдонимами
    //возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
    //где была вызвана функция, в теле которой находятся вхождения)
    //TODO: исправить. Если брать только ячейки памяти вхождений, содержащих v1 и v2, то возможна ошибка,
    //когда ячейки содержат одинаковые адреса, они передаются в функцию и там есть псевдонимы. Внутри функции
    //имена переменных - другие, поэтому мы их не отлавливаем.
    //Правильный анализ такой: собираем все ячейки памяти, связанные с v1 и c v2. Добавляем к ним содержимое,
    //содержимое содержимого и т.п. Потом смотрим пересекаются ли полученные множества.
    bool isAlias(OPS::Reprise::VariableDeclaration& v1, OPS::Reprise::VariableDeclaration& v2);

    //возвращает вектор альясов, состоящих из вхождений, принадлежащих данной
    //части кода программы (возвращаются вхождения только из текущей функции),
    //Одно вхождение может включаться в несколько множеств.
    //память необходимо отчистить после использования!
    std::vector<std::vector<OccurrencePtrWithGeneratorInformation > > getOccursByAlias(
        OPS::Reprise::RepriseBase& code);

    //возвращает множество вхождений-псевдонимов данного вхождения внутри code
    //если code==0, то для всей программы
    //возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
    //где была вызвана функция, в теле которой находятся вхождения)
    //если в code содержатся вызовы функций, то псевдонимы возвращаются и из этих функций
    std::list<BasicOccurrencePtr> getBasicOccurrencesAliasedWith(BasicOccurrence& o, OPS::Reprise::RepriseBase* code = 0);

    //возвращает множество вхождений-псевдонимов данного вхождения внутри code
    //если code==0, то для всей программы
    //возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
    //где была вызвана функция, в теле которой находятся вхождения)
    //если в code содержатся вызовы функций, то псевдонимы возвращаются и из этих функций
    std::list<BasicOccurrencePtr> getBasicOccurrencesAliasedWith(Reprise::VariableDeclaration& v, OPS::Reprise::RepriseBase* code = 0);


    //возвращает возможные функции (кроме функций выделения памяти),
    //которые могут содержаться в данном указателе на функцию
    //применяется при определении, какая функция вызывается в данном месте программы
    //для вхождений из контейнера должен быть проведен анализ псевдонимов, иначе выскочит исключение
    std::list<OPS::Reprise::SubroutineDeclaration*> getAllPossibleSubroutinesByPointer(OPS::Reprise::SubroutineCallExpression& e);

    //выясняет, живет ли ячейка памяти, соответствующая вхождению, вне указанного блока
    //отвечает на вопрос: может ли данное вхождение быть связанным с чем-то циклически порожденными дугами относительно указанного тела цикла
    bool isOccurrencePrivate(const OPS::Reprise::BlockStatement& block, BasicOccurrencePtr& o);

    // Используется только при тестированиии анализа псевдонимов: возвращает SAMC всех вхождений для печати на экране или в файл
    std::string toString(bool onlyNonEmpty = false);
    // Используется только при тестированиии анализа псевдонимов: возвращает SAMC 1 вхождения для печати на экране или в файл
    std::string toString(BasicOccurrence& o);

    OccurrenceContainer* getOccurrenceContainer();

    MemoryCellContainer* getMemoryCellContainer();

    //находит возможные ЗНАЧЕНИЯ данного вхождения по построенному OccurrenceSAMC
    //возвращается контексно независимая информация
    void getOccurrenceContent(BasicOccurrence& o, SetAbstractMemoryCell& result);

    //находит возможные ячейки памяти, в которых может храниться данное вхождение
    //применяется только при тестировании анализатора альясов
    //поиск производится по построенному OccurrenceSAMC
    //возвращается контексно независимая информация
    void getOccurrenceAddress(BasicOccurrence& o, SetAbstractMemoryCell& result);

    //находит возможные ячейки памяти, отличные от content и address, от содержимого
    //которых зависит содержимое данного вхождения
    void getDependentCellsOfOccurrence(BasicOccurrence& o, SetAbstractMemoryCell& result);

    //для тестирования
    std::map<OPS::Reprise::StatementBase*, int> getStmtVisitsCount();

    AliasAnalysisOptions options();

    bool isVariableChanged(BasicOccurrence& o, OPS::Reprise::RepriseBase* code);

    bool isVariableChanged(Reprise::VariableDeclaration& v, OPS::Reprise::RepriseBase* code);

    bool isFortranAliases(Reprise::VariableDeclaration& v);

    bool isFortranAliases();

    ///Только для тестирования анализатора псевдонимов: SAMC вхождения - множество ячеек памяти, которые соответствуют ему (см. описание в docs/Анализ альясов )
    OccurrenceSAMC getOccurrenceSAMC(BasicOccurrence& o);

private:

    //находит возможные ячейки памяти, в которых может храниться данное вхождение
    //добавляет к ним все ячейки памяти, затронутые операцией [] при вычислении значения вхождения
    //содержимое вхождения НЕ ВКЛЮЧАЕТСЯ!!!
    //поиск производится по построенному OccurrenceSAMC
    //возвращается контексно независимая информация
    void getCellsConnectedWithOccur(BasicOccurrence& o, SetAbstractMemoryCell& result, bool onlyDependentCells = false);

    //возвращает множество вхождений-псевдонимов имеющих адрес из samc внутри code
    //если code==0, то для всей программы
    //возвращается контекстно НЕЗАВИСИМАЯ информация
    std::list<BasicOccurrencePtr> getOccurrencesAliasedWithHelper(SetAbstractMemoryCell& samc, OPS::Reprise::RepriseBase* code);

    // делает черновик draft разбиения вхождений на альясы
    // каждому альясу соответствует своя ячейка памяти, draft - и есть данное соответствие
    void makeDraftAliases(std::vector<BasicOccurrencePtr>& basicOccurrences,
        std::map <MemoryCellorOffset*, std::set<BasicOccurrencePtrWithGeneratorInformation> >& draft);

    // собирает все ячейки памяти в которых может храниться содержимое вхождений, включающих в себя variable
    SetAbstractMemoryCell getMemoryOfAllOccurences(const Reprise::VariableDeclaration& variable);

    //выясняет, живет ли ячейка памяти вне указанного блока
    void isMemoryCellPrivate(const OPS::Reprise::BlockStatement& block, SetAbstractMemoryCell& memCells, std::valarray<bool>& result, SetAbstractMemoryCell& examinedCells);

    void addFuncAliasesWithSameGlobalVariable(std::vector< std::vector<OccurrencePtrWithGeneratorInformation> >& aliases, std::vector<ComplexOccurrencePtr>& complexOccurrences);

    void checkVisitedStmts();

    bool m_wasAliasesBuilt;

    AliasProcedureAnalyzer *m_aliasProcedureAnalyzer;

    AliasAnalysisContext* m_analysisContext;

    friend class AliasAnalysisTester;
};


}
}
