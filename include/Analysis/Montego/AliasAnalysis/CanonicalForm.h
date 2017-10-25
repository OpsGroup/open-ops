#pragma once
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Reprise.h"
#include "OPS_Stage/Passes.h"
#include <string>

/*
здесь описаны классы преобразований программ к канонической форме, с которой могут работать
анализатор альясов и депграф Montego
см. docs/Montego/преобразования запутанных программ
*/

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{
class OccurrenceContainer;

void apply4CanonicalTransforms(RepriseBase& code);

//возвращает true для нулевого аргумента операций [] и ".", если он не является элементарным вхождением
bool predicateFor2Transform(ExpressionBase& e);

//возвращает true для аргументов вызовов функции, которые содержат вхождения,
//И для имени функции, если оно не является вхождением
bool predicateFor3Transform(ExpressionBase& e);

//возвращает true если это выражение ? :
bool predicateForConditionalExpressions(ExpressionBase& e);

//заменяет *() на ()[0]
void convertDereferencesToArrayAccess(RepriseBase& code);

//Заменяет все подвыражения, удовлетворяющие предикату на ReferenceExpression новой переменной,
//предварительно определив эту переменную и присвоив ей подвыражение 
//Если подвыражение внутри ExpressionStatement, то присвоения добавляем операторами перед ним.
//Если выражение внутри условия while, for то добавляем в текущее выражение через запятую
//Делает преобразование в два этапа: сначала составляет список подвыражений, которые нужно 
//сделать переменными (подвыражения могут получаться вложенными друг в друга). Порядок
//в списке очень важен - сначала внутренние, потом внешние.
//Второй этап - вводим новые переменные и выносим.
void convertSubexpressionsToReferences(RepriseBase& code, bool (*predicate)(Reprise::ExpressionBase&));

//заменяет все подвыражения ?: на их аргументы
void convertConditionalExpressions(RepriseBase& code);

class SubexpressionCollector : public OPS::Reprise::Service::DeepWalker
{
public:
    SubexpressionCollector(bool (*predicate)(Reprise::ExpressionBase&));
    
    ~SubexpressionCollector();
    
    std::list<ExpressionBase*> m_exToConvert; //преобразуемые подвыражения

    bool (*m_predicate)(Reprise::ExpressionBase&); //предикат, определяющий нужно ли заменять подвыражение переменной

    //здесь должны быть все Expression!!!!!
    void visit(BasicLiteralExpression&);
    void visit(StrictLiteralExpression&);
    void visit(CompoundLiteralExpression&);
    void visit(ReferenceExpression&);
    void visit(SubroutineReferenceExpression&);
    void visit(StructAccessExpression&);
    void visit(EnumAccessExpression&);
    void visit(TypeCastExpression&);
    void visit(BasicCallExpression&);
    void visit(SubroutineCallExpression&);
    void visit(EmptyExpression&);
    void visit(Canto::HirCCallExpression&);

    //for нужно обходить в другом порядке: сначала init, потом step и потом final, чтобы добавлять в 
    //конец тела цикла операторы в нужном порядке
    void visit(ForStatement&);
};



class CanonicalFormChecker : public OPS::Reprise::Service::DeepWalker
{
public:
    CanonicalFormChecker(OccurrenceContainer& occurrenceContainer);
    ~CanonicalFormChecker();
    
    void clear();
    
	bool isFragmentCanonical(OPS::Reprise::RepriseBase& fragment);

	std::string getReason() const;

    void visit(OPS::Reprise::BasicCallExpression& e);

    void visit(OPS::Reprise::SubroutineCallExpression& e);

private:
    bool m_flagIsProgramCanonical;
    OccurrenceContainer* m_occurrenceContainer;
    std::string m_reason;
};

}//end of namespace

namespace Stage
{

class AliasCanonicalForm {};

class CanonicalFormPass : public PassBase
{
public:
    CanonicalFormPass();
    bool run();
    AnalysisUsage getAnalysisUsage() const;
};

}
}//end of namespace
