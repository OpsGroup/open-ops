#pragma once
#include "OPS_Core/MemoryHelper.h"
#include "Analysis/Montego/Occurrence.h"
#include "Shared/ProgramContext.h"
#include <valarray>
#include <list>

//Здесь содержится описание всех функций, использующих анализ псевдонимов

namespace OPS
{
namespace Montego
{
class OccurrenceContainer;
class FunctionContext;

enum OccurRoleInDependency {
    GENERATOR_ROLE, USAGE_ROLE
};
typedef std::pair<OccurRoleInDependency, OccurRoleInDependency> OccurRolePair;

struct BasicOccurrencePtrWithGeneratorInformation
{
    BasicOccurrencePtr occurrencePtr;
    bool isGenerator, isUsage;//оба могут быть установлены в true
};

struct OccurrencePtrWithGeneratorInformation
{
    OccurrencePtrWithGeneratorInformation();
    OccurrencePtrWithGeneratorInformation(const BasicOccurrencePtrWithGeneratorInformation& o);
    OccurrencePtr occurrencePtr;
    bool isGenerator, isUsage;//оба могут быть установлены в true
};

bool operator <(const BasicOccurrencePtrWithGeneratorInformation& o1, const BasicOccurrencePtrWithGeneratorInformation& o2);
bool operator ==(const BasicOccurrencePtrWithGeneratorInformation& o1, const BasicOccurrencePtrWithGeneratorInformation& o2);
bool operator ==(const OccurrencePtrWithGeneratorInformation& o1, const OccurrencePtrWithGeneratorInformation& o2);

enum RecursionType
{
    RecursionByDecl,
    RecursionByCall
};

class AliasAnalysisOptions
{
public:
    //считать, что неизвестные функции меняют глобальные переменные и возвращают какие-то адресные данные
    bool treatUnknownFuncsAsUnsafe;

    // считать, что в программе не используются экзотические операции с указателями (умножения, деления, сдвиги)
    bool noPointerCrazyOperations;

    // максимальная глубина рекурсии в программе
    int maximumRecursionDepth;

    RecursionType recursionType;

    //если treatUnknownFuncsAsUnsafe == false, то можно указать список функций, которые меняют глобальные переменные и возвращают какие-то адресные данные
    std::set<Reprise::SubroutineDeclaration*> unsafeFuncs;

    //если treatUnknownFuncsAsUnsafe == true, то можно указать список функций, которые не меняют глобальные переменные и не возвращают адресные данные
    std::set<Reprise::SubroutineDeclaration*> safeFuncs;

    // массив множеств функций, меняющих общие невидимые в программе глобальные переменные (одна переменная - одно множество)
    std::vector<std::set<Reprise::SubroutineDeclaration*> > funcSetsWithSameGlobalVariable;

    bool recognizeStructFields;
    
    bool debug; //выводить ли отладочную информацию

    bool meshingIfBranches; // Объединять ли результаты анализа веток if в месте анализа?

    bool cacheProcedureAnalys; // Кэшировать ли результаты процедурного анализа

    AliasAnalysisOptions();
};

class AliasInterface
{
public:

    static AliasInterface* create(Reprise::ProgramUnit &programUnit, OccurrenceContainer &oc);//опции по умолчанию почему-то приводят к непонятным недетерминированным глюкам
    static AliasInterface* create(Reprise::ProgramUnit &programUnit, OccurrenceContainer &oc, AliasAnalysisOptions options);

    virtual bool wasBuilt() const = 0;

    //Проводит анализ альясов.
    //При этом контейнер вхождений автоматически расширяется до множетсва всех вхождений программы.
    //Возвращает 0 - все нормально, 1 - программа не приведена к канонической форме
    virtual int runAliasAnalysis() = 0;


    //возвращает true, если вхождения могут являться псевдонимами
    //false - если точно не являются псевдонимами
    //возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
    //где была вызвана функция, в теле которой находятся вхождения)
    virtual bool isAlias(BasicOccurrence& o1, BasicOccurrence& o2) = 0;

    //возвращает набор пар типов зависимостей между вхождениями для заданного контекста вызовов функций
    virtual std::list<OccurRolePair> getAliasTypes(Occurrence& o1, Occurrence& o2, FunctionContext* funcContext) = 0;

    //возвращает true, если вхождения могут являться псевдонимами
    //false - если точно не являются псевдонимами
    //возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
    //где была вызвана функция, в теле которой находятся вхождения)
    //TODO: исправить. Если брать только ячейки памяти вхождений, содержащих v1 и v2, то возможна ошибка,
    //когда ячейки содержат одинаковые адреса, они передаются в функцию и там есть псевдонимы. Внутри функции
    //имена переменных - другие, поэтому мы их не отлавливаем.
    //Правильный анализ такой: собираем все ячейки памяти, связанные с v1 и c v2. Добавляем к ним содержимое,
    //содержимое содержимого и т.п. Потом смотрим пересекаются ли полученные множества.
    virtual bool isAlias(OPS::Reprise::VariableDeclaration& v1, OPS::Reprise::VariableDeclaration& v2) = 0;

    //возвращает вектор альясов, состоящих из вхождений, принадлежащих данной
    //части кода программы (возвращаются вхождения только из текущей функции),
    //Одно вхождение может включаться в несколько множеств.
    //память необходимо отчистить после использования!
    virtual std::vector<std::vector<OccurrencePtrWithGeneratorInformation > > getOccursByAlias(
        OPS::Reprise::RepriseBase& code) = 0;

    //возвращает множество вхождений-псевдонимов данного вхождения внутри code
    //если code==0, то для всей программы
    //возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
    //где была вызвана функция, в теле которой находятся вхождения)
    //если в code содержатся вызовы функций, то псевдонимы возвращаются и из этих функций
    virtual std::list<BasicOccurrencePtr> getBasicOccurrencesAliasedWith(BasicOccurrence& o, OPS::Reprise::RepriseBase* code = 0) = 0;

    //возвращает множество вхождений-псевдонимов данного вхождения внутри code
    //если code==0, то для всей программы
    //возвращается контекстно НЕЗАВИСИМАЯ информация (т.е. не зависит от того места,
    //где была вызвана функция, в теле которой находятся вхождения)
    //если в code содержатся вызовы функций, то псевдонимы возвращаются и из этих функций
    virtual std::list<BasicOccurrencePtr> getBasicOccurrencesAliasedWith(Reprise::VariableDeclaration& v, OPS::Reprise::RepriseBase* code = 0) = 0;


    //возвращает возможные функции (кроме функций выделения памяти),
    //которые могут содержаться в данном указателе на функцию
    //применяется при определении, какая функция вызывается в данном месте программы
    //для вхождений из контейнера должен быть проведен анализ псевдонимов, иначе выскочит исключение
    virtual std::list<OPS::Reprise::SubroutineDeclaration*> getAllPossibleSubroutinesByPointer(OPS::Reprise::SubroutineCallExpression& e) = 0;

    //выясняет, живет ли ячейка памяти, соответствующая вхождению, вне указанного блока
    //отвечает на вопрос: может ли данное вхождение быть связанным с чем-то циклически порожденными дугами относительно указанного тела цикла
    virtual bool isOccurrencePrivate(const OPS::Reprise::BlockStatement& block, BasicOccurrencePtr& o) = 0;

    virtual ~AliasInterface() {}
};

class AliasInterfaceMeta : public OPS::Shared::IMetaInformation
{
public:
    AliasInterfaceMeta(OPS::Shared::ProgramContext& context);

    std::tr1::shared_ptr<AliasInterface> getInterface() const;

    const char* getUniqueId() const;
    std::vector<const char*> getDependencies() const;
    //AliasInterfaceMeta* clone(Reprise::ProgramUnit &cloneProgram);

    static const char* const UNIQUE_ID;

private:
    std::tr1::shared_ptr<AliasInterface> m_interface;
};


}//end of namespace
}//end of namespace
