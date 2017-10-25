#pragma once

//Здесь содержится описание типов вхождений: Occurrence, BasicOccurrence и ComplexOccurrence
//и связанных с ними классов и функций.
//
//Определение. Элементарным вхождением (BasicOccurrence), как обычно, будем называть 
//всякое появление переменной в тексте программы.
//Если около вхождения стоит &, то будем считать это неотъемлемой частью вхождения, а не унарной функцией. 
//Вхождения структур трактуем как многомерные массивы. Например, a[10].b.c[2] - двумерный массив с
//индексными выражениями: 10, 2 и составным именем "a.b.c"
//
//Составным вхождением будем называть некоторое множество элементарных вхождений, 
//связанных с вызовом функции. Это НЕ ВСЕ вхождения внутри функции, а быть может только их часть.
//Одному вызову функции может соответствовать несколько составных вхождений.
//
//Благодаря преобразованиям 1.1-1.3 (см. docs/Montego/преобразования...) все чтения из памяти 
//или записи в нее имеют стандартный 
//вид элементарного вхождения-переменной или поля структуры!

#include "Reprise/Expressions.h"
#include "Analysis/Montego/SafeStatus.h"
#include "OPS_Core/MemoryHelper.h"

namespace OPS
{
namespace Montego
{

class BasicOccurrence;
class ComplexOccurrence;
class Occurrence;
class OccurrenceContainer;

typedef std::tr1::shared_ptr<BasicOccurrence> BasicOccurrencePtr;
typedef std::tr1::shared_ptr<Occurrence> OccurrencePtr;
typedef std::tr1::shared_ptr<ComplexOccurrence> ComplexOccurrencePtr;

class Occurrence: public TypeConvertibleMix
{
public:
	virtual ~Occurrence()
	{
	}

public:
    virtual Reprise::StatementBase* getParentStatement() const = 0;
	virtual Reprise::ExpressionBase* getSourceExpression() const = 0;
    virtual bool isGenerator() const = 0 ;
    virtual bool isEqual(Occurrence& other) const = 0;
};

class ComplexOccurrence : public Occurrence
{
public:
	// Occurrence
	Reprise::StatementBase* getParentStatement() const;
	Reprise::ExpressionBase* getSourceExpression() const;
	bool isGenerator() const;
	bool isEqual(Occurrence& other) const;
    
public:
 
	//ocont - контейнер, откуда беруться адреса вхождений
    ComplexOccurrence(Reprise::SubroutineCallExpression& scall, std::vector<BasicOccurrencePtr>& basicOccurrences);

    Reprise::SubroutineCallExpression* getSubroutineCall() const;

    std::vector<BasicOccurrencePtr> getBasicOccurrences() const;

private:

    Reprise::SubroutineCallExpression* m_subroutineCall;

    bool m_isGenerator;

    //ComplexOccurrence не владеет элементарными вхождениями внутри вызова функции,
    //ими владеет контейнер вхождений
    std::vector<BasicOccurrencePtr> m_basicOccurrences;
};

/// Набор флагов, которые используются для описания вхождения (структура Occurrence)
enum EN_OccurrenceStatus
{
    BOUNDS_UNKNOWN = 2, //известны ли границы массива
    GENERATOR = 4, //является ли генератором
    USAGE = 8, //является ли использованием
    LOOPDESC_BUILT = 16 //проанализированы ли охватывающие циклы
};

// Имя вхождения (VariableDeclaration). Для структуры - указывается дополнительная информация
struct BasicOccurrenceName
{
    BasicOccurrenceName();

    //заполняет все поля, анализируя внутреннее представление около ReferenceExpression
    explicit BasicOccurrenceName(OPS::Reprise::ReferenceExpression& refExpr);

    //указатель на определение переменной; для структуры указатель на определение всей структуры
    OPS::Reprise::VariableDeclaration* m_varDecl;

    //число скобок у переменной или у имени структуры (для s[2][1].a[3].b[0] будет 2)
    int m_refExprBracketCount; 

    std::vector<OPS::Reprise::ExpressionBase*> m_bracketContent;//указатели на содержимое скобок

    //поля структуры после ее имени и количества скобок у них. Например для s.a[10].b.c[1][2] 
    //будет (a,1) (b,0) (c,2)
    //Не содержит элементов если это не составное имя или если это пустой класс и m_varDecl=0
    std::vector< std::pair<OPS::Reprise::StructMemberDescriptor*,int> > m_fields;

    //возвращает суммарное число скобок
    int getAllBracketCount();

	bool operator==(const BasicOccurrenceName& other) const;
	bool operator!=(const BasicOccurrenceName& other) const;
};

//элементарное вхождение
class BasicOccurrence : public Occurrence 
    //возможные статусы см. в EN_OccurDescStatus
{
public:
	// Occurrence
	Reprise::StatementBase* getParentStatement() const;
	Reprise::ExpressionBase* getSourceExpression() const;
	bool isGenerator() const;
    bool isUsage() const;
	bool isEqual(Occurrence& other) const;

public:

	//создает пустое вхождение
    BasicOccurrence();
    //создает вхождение по заданному ReferenceExpression
    BasicOccurrence(OPS::Reprise::ReferenceExpression& refExpr);

    // указатель на переменную вхождения в выражении 
    //для массива - указатель на имя, для поля структуры - указатель на всю структуру
	OPS::Reprise::ReferenceExpression* getRefExpr() const;

    // Составное имя вхождения - VariableDeclaration или
    // для структуры - VariableDeclaration всей структуры + набор StructMemberDescriptor и др. информация
	BasicOccurrenceName getName() const;

    /// количество операций [], использованных в данном вхождении, раньше это неправильно называлось размерностью m_dim
    /// для структур - суммарное число []
	int getBracketCount() const;

    /// присутсвует ли перед всем вхождением & пример: &a[i] (заметим, что такого: (&a)[i] быть не может (см. преобразование 2))
	bool isAddress() const;

    //является ли данное вхождение l-value (является ли оно ячейкой памяти, которая имеет адрес, например,
    //для int c[10][10]; вхождения c; &c, c[2]; &c[2]; &c[1][1]; не являются l-value, а c[3][2] является)
    bool islValue() const;

    Reprise::ReprisePtr<Reprise::StructType> isReturnStruct() const;

    /// возвращает родительский узел данного вхождения (не входит в состав вхождения)
	OPS::Reprise::RepriseBase* getParent() const;

    /// возвращает самый верхний узел данного вхождения (входит в состав вхождения)
    /// например для a[10].b[2] вернет операцию (...)[2]
    OPS::Reprise::ExpressionBase* getHeadNode() const;

    // возвращает вхождение для печати на экране или в файл
	std::string toString() const;

    //статусы
    bool isStatus(SafeStatus::status_t s) const;
    void addStatus(SafeStatus::status_t s);

private: //обязательно нужно, т.к. некоторые данные достраиваются автоматически в процессе обращения к вхождению

    //заполняет m_isAddress
    void determineIsAddress();

    //устанавливает флаг, если данное вхождение является генератором. Заметим, что в Си генератор может
    //одновременно быть использованием. Пример: c = (b = a)
    void determineIsGenerator();

    //заполняет m_islValue
    void determineIslValue();

    //статусы
    SafeStatus m_status;

    // указатель на переменную вхождения в выражении 
    //для массива - указатель на имя, для поля структуры - указатель на всю структуру
    OPS::Reprise::ReferenceExpression* m_refExpr;		

    // Составное имя вхождения - VariableDeclaration или
    // для структуры - VariableDeclaration всей структуры + набор StructMemberDescriptor и др. информация
    BasicOccurrenceName m_name;

    /// количество операций [], использованных в данном вхождении, раньше это неправильно называлось размерностью m_dim
    /// для структур - суммарное число []
    int m_bracketCount;

    /// присутсвует ли перед всем вхождением & пример: &a[i] (заметим, что такого: (&a)[i] быть не может (см. преобразование 2))
    bool m_isAddress;

    //является ли данное вхождение l-value (является ли оно ячейкой памяти, которая имеет адрес, например,
    //для int c[10][10]; вхождения c; &c, c[2]; &c[2]; &c[1][1]; не являются l-value, а c[3][2] является)
    bool m_islValue;

    friend class OccurrenceContainer;
    friend class GeneratorAnalyser;

};


}//end of namespace
}//end of namespace
