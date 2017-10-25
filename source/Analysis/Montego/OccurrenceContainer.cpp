#include "Analysis/Montego/OccurrenceContainer.h"
#include "OccurrenceContainerBuilder.h"
#include "OccurSearchVisitor.h"

namespace OPS
{
namespace Montego
{

OccurrenceContainer::OccurrenceContainer() 
					:  m_wasComplexOccurrenceBuilt(false), m_programPart(0)
{
}

OccurrenceContainer::~OccurrenceContainer()
{
}

//строит множество вхождений программы, если нужно, парсит линейные выражения в скобках [] 
//и строит информацию об охватывающих вхождения циклах
OccurrenceContainer::OccurrenceContainer(OPS::Reprise::RepriseBase& program)
{
    OccurrenceContainerBuilder builder;
    program.accept(builder);
    m_basicOccurByReference = builder.getOccurByReferenceMap();
    m_programPart = &program;
    m_wasComplexOccurrenceBuilt = false;
}

//добавляет элементарные вхождения в контейнер из данной части кода программы (см. также комментарий у конструктора). 
//Если в контейнере уже существуют такие вхождения, то возникает ошибка 
//или они перезаписываются в зависимости от replaceOccurrences.
void OccurrenceContainer::addAllBasicOccurrencesIn(OPS::Reprise::RepriseBase& program, bool replaceOccurrences)
{
    OccurrenceContainerBuilder builder;
    builder.addAllBasicOccurrencesIn(program,m_basicOccurByReference,replaceOccurrences);
    m_programPart = &program;
}

//добавляет оставные вхождения в контейнер из данной части кода программы.
//Если в контейнере уже существуют такие вхождения, то они перезаписываются.
//Не заходит внутрь вызываемых функций!
void OccurrenceContainer::addAllComplexOccurrencesIn(OPS::Reprise::RepriseBase& program, AliasInterface& ai)
{
    OccurrenceContainerBuilder builder;
    builder.addAllComplexOccurrencesIn(program,m_complexOccurBySCall,*this,ai);
    m_wasComplexOccurrenceBuilt = true;
}


//находит вхождение по заданному ReferenceExpression. Возвращает 0 - найдено, 1 - не найдено
int OccurrenceContainer::getOccurBy(const OPS::Reprise::ReferenceExpression& ref, BasicOccurrencePtr& occur)
{
	BasicOccurrenceByReferenceMap::const_iterator it = m_basicOccurByReference.find(&ref);

    if (it != m_basicOccurByReference.end())
    {
		occur = it->second;
        return 0;
    }
    else
    {
        return 1;//not found
    }
}

//находит вхождение по заданному StructAccessExpression. Возвращает 0 - найдено, 1 - не найдено
int OccurrenceContainer::getOccurBy(const OPS::Reprise::StructAccessExpression& structAccess, BasicOccurrencePtr& occur)
{
	const OPS::Reprise::RepriseBase* child = &structAccess.getStructPointerExpression();
    while (true)
    {
		if (child->is_a<const OPS::Reprise::BasicCallExpression>())
        {
            //child - это обращение к элементу массива
			const OPS::Reprise::BasicCallExpression& basicCallExpr = child->cast_to<const OPS::Reprise::BasicCallExpression>();
            return getOccurBy(basicCallExpr, occur);
        }
        else
        {
			if (const StructAccessExpression* childStruct = child->cast_ptr<const OPS::Reprise::StructAccessExpression>())
            {
                //child - это обращение к полю структуры
				child = &childStruct->getStructPointerExpression();
                continue;
            }
			else if (const ReferenceExpression* ref = child->cast_ptr<const ReferenceExpression>())
            {
				return  getOccurBy(*ref,occur);
			}
			else
			{
				throw OPS::RuntimeError("Canonical transformations were not done!");
            }
        }
    }

}

//находит вхождение по заданному обращению к элементу массива или по операции & : BasicCallExpression. 
//Возвращает 0 - найдено, 1 - не найдено, 2 - выражение не является обращением к элементу массива или операцией &
//occur - адрес вхождения ВНУТРИ контейнера (эту память освобождать нельзя!)
int OccurrenceContainer::getOccurBy(const OPS::Reprise::BasicCallExpression& arrayAccess, BasicOccurrencePtr& occur)
{
    // если это операция &, то вызываем для внутренности соотв. функцию
    if (arrayAccess.getKind() == OPS::Reprise::BasicCallExpression::BCK_TAKE_ADDRESS)
		return getOccurBy(arrayAccess.getArgument(0),occur);

    if (arrayAccess.getKind() != OPS::Reprise::BasicCallExpression::BCK_ARRAY_ACCESS)
        return 2;
	const OPS::Reprise::RepriseBase& child = arrayAccess.getArgument(0);
	if (child.is_a<const OPS::Reprise::ReferenceExpression>())
    {
		const OPS::Reprise::ReferenceExpression& refExpr = child.cast_to<const OPS::Reprise::ReferenceExpression>();
        return getOccurBy(refExpr, occur);
    }
    else
    {
		if (child.is_a<const OPS::Reprise::StructAccessExpression>())
        {
			const OPS::Reprise::StructAccessExpression& structAccess = child.cast_to<const OPS::Reprise::StructAccessExpression>();
            return getOccurBy(structAccess, occur);
        }
        else
        {
            throw OPS::RuntimeError("Canonical transformations were not done!");
        }
    }
}

//находит составное вхождение по заданному вызову функции
//Возвращает 0 - найдено, 1 - не найдено, 2 - выражение не является обращением к элементу массива или операцией &
//occur - адрес вхождения ВНУТРИ контейнера (эту память освобождать нельзя!)
int OccurrenceContainer::getOccurBy(const OPS::Reprise::SubroutineCallExpression& scall, ComplexOccurrencePtr& occur)
{
	std::map<const OPS::Reprise::SubroutineCallExpression*, ComplexOccurrencePtr>::iterator it;
    it = m_complexOccurBySCall.find(&scall);
    if (it != m_complexOccurBySCall.end())
    {
        occur = it->second;
        return 0;
    }
    else
    {
        return 1;//not found
    }  
}


//вызывает одну из вышеперечисленных функций, если тип подходит, в противном случае возвращает 2
int OccurrenceContainer::getOccurBy(const OPS::Reprise::RepriseBase& smth, BasicOccurrencePtr& occur)
{
	const OPS::Reprise::ReferenceExpression* p = smth.cast_ptr<const OPS::Reprise::ReferenceExpression>();
    if (p)
    {
        return getOccurBy(*p, occur);
    }
    else
    {
		const OPS::Reprise::StructAccessExpression* q = smth.cast_ptr<const OPS::Reprise::StructAccessExpression>();
        if (q)
        {
            return getOccurBy(*q, occur);
        }
        else
        {
			const OPS::Reprise::BasicCallExpression* r = smth.cast_ptr<const OPS::Reprise::BasicCallExpression>();
            if (r)
            {
                return getOccurBy(*r, occur);
            }
            else
            {
                return 2;
            }
        }
    }
}

void OccurrenceContainer::getAllOccurrencesHelper(const OPS::Reprise::RepriseBase* code,
                                                  std::vector<BasicOccurrencePtr>& basicOccurrences, 
                                                  std::vector<ComplexOccurrencePtr>& complexOccurrences, 
                                                  bool flagEnterSubrouitineCalls, 
                                                  AliasInterface* ai, bool flagCollectComplexOccurs)
{
    if (flagCollectComplexOccurs && !m_wasComplexOccurrenceBuilt)
        throw OPS::RuntimeError("OccurrenceContainer::getAllOccurrencesHelper: complex occurrences were not built, but wanted!");
    BasicOccurrencePtr occur;
    ComplexOccurrencePtr c_occur;
    basicOccurrences.clear();
    complexOccurrences.clear();
    if (code != 0)
    {
        std::set<OPS::Reprise::ReferenceExpression*>::iterator it;
        std::set<OPS::Reprise::SubroutineCallExpression*>::iterator its;
        OccurSearchVisitor visitor(*this, flagEnterSubrouitineCalls, ai);
		const_cast<OPS::Reprise::RepriseBase*>(code)->accept(visitor);
        std::set<OPS::Reprise::ReferenceExpression*> refSet = visitor.getRefSet();
        std::set<OPS::Reprise::SubroutineCallExpression*> scallSet = visitor.getSCallSet();

        basicOccurrences.reserve(refSet.size());
        for (it = refSet.begin(); it!=refSet.end(); ++it)
        {
            if (!getOccurBy(**it,occur)) basicOccurrences.push_back(occur);
            else throw OPS::RuntimeError("OccurrenceContainer::getAllOccurrencesHelper: there is occurrence in code, wich is absent in container");
        }
        if (flagCollectComplexOccurs)
        {
            complexOccurrences.reserve(scallSet.size());
            for (its = scallSet.begin(); its!=scallSet.end(); ++its)
            {
                if (!getOccurBy(**its,c_occur))  complexOccurrences.push_back(c_occur);
                else throw OPS::RuntimeError("OccurrenceContainer::getAllOccurrencesHelper: there is occurrence in code, wich is absent in container");
            }
        }
    }
    else //возвращаются ВСЕ вхождения контейнера
    {
		std::map <const OPS::Reprise::ReferenceExpression*, BasicOccurrencePtr>::iterator it;
		std::map <const OPS::Reprise::SubroutineCallExpression*, ComplexOccurrencePtr>::iterator its;
        basicOccurrences.reserve(m_basicOccurByReference.size());
        for (it = m_basicOccurByReference.begin(); it != m_basicOccurByReference.end(); ++it)
            basicOccurrences.push_back(it->second);
        if (flagCollectComplexOccurs)
        {
            complexOccurrences.reserve(m_complexOccurBySCall.size());
            for (its = m_complexOccurBySCall.begin(); its != m_complexOccurBySCall.end(); ++its)
                complexOccurrences.push_back(its->second);
        }
    }
}

//возвращает массив всех вхождений контейнера, принадлежащих данной части кода программы
//внутрь вызываемых функций НЕ ВХОДИТ
//если code = 0, возвращаются ВСЕ вхождения контейнера
std::vector<OccurrencePtr> OccurrenceContainer::getAllOccurrencesIn(OPS::Reprise::RepriseBase* code)
{
    if (!m_wasComplexOccurrenceBuilt) throw OPS::RuntimeError("OccurrenceContainer::getAllOccurrencesIn: Complex occurrences were not built, but wanted!");
    std::vector<OccurrencePtr> res;
    std::vector<BasicOccurrencePtr> basicOccurrences;
    std::vector<ComplexOccurrencePtr> complexOccurrences;
    getAllOccurrencesHelper(code, basicOccurrences, complexOccurrences, false, NULL, true);
    res.reserve(basicOccurrences.size()+complexOccurrences.size());
    res.insert(res.end(),basicOccurrences.begin(),basicOccurrences.end());
    res.insert(res.end(),complexOccurrences.begin(),complexOccurrences.end());
    return res;
}

//возвращает массив всех вхождений контейнера, разделенный на две части: элементарные вхождения
//и составные вхождения-вызовы функций. Вхождения принадлежат заданной части кода программы
//внутрь вызываемых функций НЕ ВХОДИТ
//если code = 0, возвращаются ВСЕ вхождения контейнера
void OccurrenceContainer::getAllOccurrencesIn(OPS::Reprise::RepriseBase* code, 
                                              std::vector<BasicOccurrencePtr>& basicOccurrences, 
                                              std::vector<ComplexOccurrencePtr>& complexOccurrences)
{
    getAllOccurrencesHelper(code, basicOccurrences, complexOccurrences, false, NULL, true);
}

//возвращает массив всех элементарных вхождений контейнера
//Вхождения принадлежат заданной части кода программы
//внутрь вызываемых функций НЕ ВХОДИТ
//если code = 0, возвращаются ВСЕ вхождения контейнера
//Если в контейнере не нашлось имеющегося в коде вхождения, то выскакивает исключение.
std::vector<BasicOccurrencePtr> OccurrenceContainer::getAllBasicOccurrencesIn(const OPS::Reprise::RepriseBase* code)
{
    std::vector<BasicOccurrencePtr> basicOccurrences;
    std::vector<ComplexOccurrencePtr> complexOccurrences;
    getAllOccurrencesHelper(code, basicOccurrences, complexOccurrences, false, NULL, false);
    return basicOccurrences;
}


//возвращает массив всех вхождений контейнера, принадлежащих данной части кода программы
//В ТОМ ЧИСЛЕ И ВНУТРИ ВЫЗЫВАЕМЫХ ФУНКЦИЙ!!!!!!!!!!!!!!!!
//если code = 0, возвращаются ВСЕ вхождения контейнера
//Если ai=0 и в коде есть вызов функции через указатель, то выскочит исключение
//возвращаемые вхожения НЕ копируются, они продолжают принадлежать контейнеру
std::vector<BasicOccurrencePtr> OccurrenceContainer::getDeepAllBasicOccurrencesIn(OPS::Reprise::RepriseBase* code,  
                                                  AliasInterface* ai)
{
    std::vector<BasicOccurrencePtr> basicOccurrences;
    std::vector<ComplexOccurrencePtr> complexOccurrences;
    getAllOccurrencesHelper(code, basicOccurrences, complexOccurrences, true, ai, false);
    return basicOccurrences;
}

//возвращает список всех вхождений переменной v с любым количеством скобок и комбинаций полей структур
std::list<BasicOccurrencePtr> OccurrenceContainer::getAllBasicOccurrencesOf(const VariableDeclaration& v)
{
    std::list<BasicOccurrencePtr> result;
	std::map <const OPS::Reprise::ReferenceExpression*, BasicOccurrencePtr>::iterator it;
    for (it = m_basicOccurByReference.begin(); it != m_basicOccurByReference.end(); ++it)
    {
        if (&it->first->getReference() == &v)   result.push_back(it->second);
    }
    return result;
}


//проверка наличия вхождений в заданном участке кода
//возвращает 0 - нет вхождения, 1 - есть обычные вхождения и нет вызовов функций, 2 - есть вызовы функций
//внутрь вызываемых функций не входит
class ContainOccurrencesVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    ContainOccurrencesVisitor():result(0){}
    ~ContainOccurrencesVisitor(){}
    
	void visit(ReferenceExpression&){if (result == 0) result = 1;}
	void visit(SubroutineCallExpression&){if (result != 2) result = 2;}
    
    int result;
};


//проверка наличия вхождений в заданном участке кода
//возвращает 0 - нет вхождения, 1 - есть обычные вхождения и нет вызовов функций, 2 - есть вызовы функций
//внутрь вызываемых функций не входит
int OccurrenceContainer::containOccurrences(OPS::Reprise::RepriseBase& code)
{
    ContainOccurrencesVisitor v;
    code.accept(v);
    return v.result;
}


//возвращает часть программы, для которой был построен контейнер (или часть, вхождения из которой последний раз добавлялись в контейнер)
RepriseBase* OccurrenceContainer::getProgramPart()
{
    return m_programPart;
}

//собирает ReferenceExpr в порядке обхода кода DeepWalkr-ом
class ExprCollector : public Service::DeepWalker
{
public:
    ExprCollector(){}
    void visit(Reprise::ReferenceExpression& e){m_refExpr.push_back(&e);}
    std::list<Reprise::ReferenceExpression*> m_refExpr;
};

std::string OccurrenceContainer::toString()
{
    std::string res = "Occurrence container:\n";
    ExprCollector v;
    m_programPart->accept(v);
    std::list<Reprise::ReferenceExpression*>::iterator it = v.m_refExpr.begin();
    for(;it != v.m_refExpr.end(); ++it)
    {
        BasicOccurrencePtr po = m_basicOccurByReference[*it];
        res += po->toString();
        if (po->isUsage()) res += " USAGE";
        if (po->isGenerator()) res += " GENERATOR";
        res += "\n";
    }
    return res;
}

OccurrenceContainerMeta::OccurrenceContainerMeta(OPS::Shared::ProgramContext &context)
	:m_container(new OccurrenceContainer(context.getProgram()))
{
}

std::tr1::shared_ptr<OccurrenceContainer> OccurrenceContainerMeta::getContainer() const
{
	return m_container;
}

const char* OccurrenceContainerMeta::getUniqueId() const
{
	return UNIQUE_ID;
}

//OccurrenceContainerMeta* OccurrenceContainerMeta::clone(Reprise::ProgramUnit &cloneProgram)
//{
//	return new OccurrenceContainerMeta(cloneProgram);
//}

const char* const OccurrenceContainerMeta::UNIQUE_ID = "OccurrenceContainer";

}//end of namespace
}//end of namespace
