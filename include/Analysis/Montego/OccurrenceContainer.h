#pragma once

//Здесь содержится описание контейнера вхождений Occurrence
//Для быстрого поиска вхождения внутри контейнера храняться в map <ReferenceExpression*, Occurrence>

#include <list>
#include "Reprise/Expressions.h"
#include "Analysis/Montego/Occurrence.h"
#include "Shared/ProgramContext.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{

class AliasInterface;

class OccurrenceContainer 
{
public:
    OccurrenceContainer();
    ~OccurrenceContainer();

    //строит множество вхождений программы, не заходит внутрь вызываемых функций!
    //если вы впоследствии собираетесь использовать анализатор альясов или депграф, то наиболее оптимально
    //построить вхождения для всей программы (ProgramUnit). 
    explicit OccurrenceContainer(OPS::Reprise::RepriseBase& program);

    //добавляет элементарные вхождения в контейнер из данной части кода программы 
    //(см. также комментарий у конструктора). 
    //Если в контейнере уже существуют такие вхождения, то возникает ошибка 
    //или они перезаписываются в зависимости от replaceOccurrences.
    //не заходит внутрь вызываемых функций!
    void addAllBasicOccurrencesIn(OPS::Reprise::RepriseBase& program, bool replaceOccurrences = true);

    //добавляет оставные вхождения в контейнер из данной части кода программы.
    //Если в контейнере уже существуют такие вхождения, то они перезаписываются.
    //Не заходит внутрь вызываемых функций!
    void addAllComplexOccurrencesIn(OPS::Reprise::RepriseBase& program, AliasInterface& ai);

    //находит вхождение по заданному ReferenceExpression. Возвращает 0 - найдено, 1 - не найдено
    //occur - адрес вхождения ВНУТРИ контейнера (эту память освобождать нельзя!)
	int getOccurBy(const OPS::Reprise::ReferenceExpression& ref, BasicOccurrencePtr& occur);

    //находит вхождение по заданному StructAccessExpression. Возвращает 0 - найдено, 1 - не найдено
    //occur - адрес вхождения ВНУТРИ контейнера (эту память освобождать нельзя!)
	int getOccurBy(const OPS::Reprise::StructAccessExpression& structAccess, BasicOccurrencePtr& occur);

    //находит вхождение по заданному обращению к элементу массива или по операции & : BasicCallExpression. 
    //Возвращает 0 - найдено, 1 - не найдено, 2 - выражение не является обращением к элементу массива или операцией &
    //occur - адрес вхождения ВНУТРИ контейнера (эту память освобождать нельзя!)
	int getOccurBy(const OPS::Reprise::BasicCallExpression& arrayAccess, BasicOccurrencePtr& occur);

    //находит составное вхождение по заданному вызову функции
    //Возвращает 0 - найдено, 1 - не найдено, 2 - выражение не является обращением к элементу массива или операцией &
    //occur - адрес вхождения ВНУТРИ контейнера (эту память освобождать нельзя!)
	int getOccurBy(const OPS::Reprise::SubroutineCallExpression& scall, ComplexOccurrencePtr& occur);

    //находит элементарное вхождение по одному заданному его элементу: referenceExpr, [] или ".".
    //вызов функции не ищется!!!!!!!!!!!!!!
    //Возвращает 0 - найдено, 1 - не найдено
    //Если тип smth не подходит возвращает 2
    //occur - адрес вхождения ВНУТРИ контейнера (эту память освобождать нельзя!)
	int getOccurBy(const OPS::Reprise::RepriseBase& smth, BasicOccurrencePtr& occur);
    
    //возвращает массив всех вхождений, принадлежащих данной части кода программы
    //внутрь вызываемых функций НЕ ВХОДИТ
    //если code = 0, возвращаются ВСЕ вхождения контейнера
    //Если в контейнере не нашлось имеющегося в коде вхождения, то выскакивает исключение.
    std::vector<OccurrencePtr> getAllOccurrencesIn(OPS::Reprise::RepriseBase* code);

    //возвращает массив всех вхождений контейнера, разделенный на две части: элементарные вхождения
    //и составные вхождения-вызовы функций. Вхождения принадлежат заданной части кода программы
    //внутрь вызываемых функций НЕ ВХОДИТ
    //если code = 0, возвращаются ВСЕ вхождения контейнера
    //Если в контейнере не нашлось имеющегося в коде вхождения, то выскакивает исключение.
    void getAllOccurrencesIn(OPS::Reprise::RepriseBase* code, std::vector<BasicOccurrencePtr>& basicOccurrences, std::vector<ComplexOccurrencePtr>& complexOccurrences);

    //возвращает массив всех элементарных вхождений контейнера
    //Вхождения принадлежат заданной части кода программы
    //внутрь вызываемых функций НЕ ВХОДИТ
    //если code = 0, возвращаются ВСЕ вхождения контейнера
    //Если в контейнере не нашлось имеющегося в коде вхождения, то выскакивает исключение.
	std::vector<BasicOccurrencePtr> getAllBasicOccurrencesIn(const OPS::Reprise::RepriseBase* code);

    //возвращает массив всех элементарных вхождений контейнера, принадлежащих данной части кода программы
    //В ТОМ ЧИСЛЕ И ВНУТРИ ВЫЗЫВАЕМЫХ ФУНКЦИЙ!!!!!!!!!!!!!!!!
    //если code = 0, возвращаются ВСЕ вхождения контейнера
    //Если ai=0 и в коде есть вызов функции через указатель, то выскочит исключение
    //возвращаемые вхожения НЕ копируются, они продолжают принадлежать контейнеру
    //эта функция используется при построении составных вхождений
    //Если в контейнере не нашлось имеющегося в коде вхождения, то выскакивает исключение.
    std::vector<BasicOccurrencePtr> getDeepAllBasicOccurrencesIn(OPS::Reprise::RepriseBase* code,  AliasInterface* ai);

    //возвращает список всех вхождений переменной v с любым количеством скобок и комбинаций полей структур
    std::list<BasicOccurrencePtr> getAllBasicOccurrencesOf(const VariableDeclaration& v);

    //проверка наличия вхождений в заданном участке кода
    //возвращает 0 - нет вхождения, 1 - есть обычные вхождения и нет вызовов функций, 2 - есть вызовы функций
    //внутрь вызываемых функций не входит
    static int containOccurrences(OPS::Reprise::RepriseBase& code);

    //возвращает часть программы, для которой был построен контейнер (или часть, вхождения из которой последний раз добавлялись в контейнер)
    RepriseBase* getProgramPart();

    //печатает вхождения в порядке обхода кода DeepWalker-ом
    std::string toString();

private:

    void getAllOccurrencesHelper(const OPS::Reprise::RepriseBase* code, std::vector<BasicOccurrencePtr>& basicOccurrences, std::vector<ComplexOccurrencePtr>& complexOccurrences, bool flagEnterSubrouitineCalls, AliasInterface* ia, bool flagCollectComplexOccurs);

	typedef std::map <const OPS::Reprise::ReferenceExpression*, BasicOccurrencePtr> BasicOccurrenceByReferenceMap;
	BasicOccurrenceByReferenceMap m_basicOccurByReference;

	std::map <const OPS::Reprise::SubroutineCallExpression*, ComplexOccurrencePtr> m_complexOccurBySCall;

    bool m_wasComplexOccurrenceBuilt;

    //для какой части программы был построен контейнер вхождений 
    //или последняя часть программы, из которой добавлялись вхождения
    RepriseBase* m_programPart; 

};

class OccurrenceContainerMeta : public OPS::Shared::IMetaInformation
{
public:
	OccurrenceContainerMeta(OPS::Shared::ProgramContext& context);

	std::tr1::shared_ptr<OccurrenceContainer> getContainer() const;

	const char* getUniqueId() const;
	//OccurenceContainerMeta* clone(Reprise::ProgramUnit &cloneProgram);

	static const char* const UNIQUE_ID;

private:
	std::tr1::shared_ptr<OccurrenceContainer> m_container;
};

}//end of namespace
}//end of namespace
