#include "Analysis/Montego/DependenceGraph/IndexAnalysis.h"
#include "Shared/Checks.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Analysis/Montego/OccurrenceCoeffs.h"
#include "Reprise/Service/DeepWalker.h"
#include <iostream>
#include "Analysis/LatticeGraph/ElemLatticeGraph.h"
#include "Shared/LoopShared.h"
#include "Shared/StatementsShared.h"
#include "Reprise/ServiceFunctions.h"

#include "OPS_Core/msc_leakcheck.h"//контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{

// проверяет, применимы ли функции анализа индексных выражений к указанному фрагменту программы
// возвращает true - применимы, false - неприменимы
bool testApplicability(OPS::Reprise::StatementBase& program, OccurrenceContainer& cont, AliasInterface& ai)
{
    //проверяем наличие операторов отличных от for, if и ExpressionStatement
    OPS::Shared::Checks::CompositionCheckObjects acceptableObjects;
    acceptableObjects << OPS::Shared::Checks::CompositionCheckObjects::CCOT_BlockStatement 
                      << OPS::Shared::Checks::CompositionCheckObjects::CCOT_BreakStatement
                      << OPS::Shared::Checks::CompositionCheckObjects::CCOT_ContinueStatement
                      << OPS::Shared::Checks::CompositionCheckObjects::CCOT_EmptyStatement
                      << OPS::Shared::Checks::CompositionCheckObjects::CCOT_ExpressionStatement
                      << OPS::Shared::Checks::CompositionCheckObjects::CCOT_ForStatement
                      << OPS::Shared::Checks::CompositionCheckObjects::CCOT_IfStatement
                      << OPS::Shared::Checks::CompositionCheckObjects::CCOT_ReturnStatement;
    if (!OPS::Shared::Checks::makeCompositionCheck<StatementBase>(program, acceptableObjects))
        return false;
    
    //проверяем линейность выражений вхождений
    std::set<VariableDeclaration*> externalParams;
    std::vector<BasicOccurrencePtr> occurs = cont.getAllBasicOccurrencesIn(&program);
    for (size_t i = 0; i < occurs.size(); ++i)
    {
        OPS::Montego::OccurrenceCoeffs occurrenceCoeffs(*(occurs[i]));
        std::vector<OPS::Shared::CanonicalLinearExpression> externalParamCoefs;
        std::vector<LatticeGraph::SimpleLinearExpression> coeffs;
        if ( !occurrenceCoeffs.IsCoeffsLinear() ||
             !occurrenceCoeffs.getExternalParamAndLoopCounterCoefficients(coeffs, externalParamCoefs, &program))
            return false;
        for(size_t j = 0; j < externalParamCoefs.size(); j++)
        {
            std::list<VariableDeclaration*> vars = externalParamCoefs[j].getVariables();
            externalParams.insert(vars.begin(), vars.end());
        }
    }

    //проверяем каноничность циклов 
    //(шаг = 1, счетчик не меняется внутри тела цикла, начальное и конечное выражения - 
    // канонические (имеют вид: счетчик = лин; счетчик <(<=) лин), счетчик в обоих - тот же!!!
    // выражения - линейные
    //OPS_ASSERT(!"Еще не реализовано");


    //удаляем счетчики циклов из externalParams
    //OPS_ASSERT(!"Еще не реализовано");


    //для каждого внешнего параметра
    std::set<VariableDeclaration*>::iterator it = externalParams.begin();
    for ( ; it != externalParams.end(); ++it)
    {
        //получаем список его вхождений в рассматриваемом фрагменте программы
        std::list<BasicOccurrencePtr> param_aliases = 
            ai.getBasicOccurrencesAliasedWith(**it, &program);
        //проверяем, есть ли среди них генераторы
        std::list<BasicOccurrencePtr>::iterator it2;
        for (it2 = param_aliases.begin(); it2 != param_aliases.end(); ++it2)
        {
            if ((*it2)->isGenerator()) return false;
        }
    }   

    return true;
}

// на основе анализа индексных выражений вхождений и циклов, содержащих вхождение и находящихся внутри 
// фрагмента программы (для которого построен граф зависимостей),
// выясняет могут ли два вхождения обращаться к одной и той же ячейке памяти
// возвращает true - могут обращаться, false - не могут
// данная функция использует алгоритмы решетчатого графа. 
// Если flagUseFast = true, то она работает более чем в 5 раз быстрее, чем проверка решетчатым графом,
// но при этом тестируются все зависимости одновременно: т.е. проверяется могут ли вхождения обращаться к
// одной ячейке памяти
bool testDependenceWithLatticeGraph(const DependenceGraphAbstractArc& arc, const DependenceGraph& graph, bool flagUseFast)
{
    OccurrencePtr oo1 = arc.getStartVertex().getSourceOccurrence();
    OccurrencePtr oo2 = arc.getEndVertex().getSourceOccurrence();
    BasicOccurrence* o1 = oo1->cast_ptr<BasicOccurrence>();
    BasicOccurrence* o2 = oo2->cast_ptr<BasicOccurrence>();
    if ((o1 == 0) || (o2 == 0)) return true;
    std::tr1::shared_ptr<AliasInterface> ai = graph.getAliasInterface();
	OPS::Reprise::StatementBase* program = const_cast<StatementBase*>(&graph.getSourceStatement());
    if (flagUseFast)
    {
		LatticeGraph::Polyhedron p = buildSystemOfIndexEquationsAndInequalities(*o1, *o2, *program);
        return (p.IsFeasible() != 0);
    }
    else
    {
        LatticeGraph::ElemLatticeGraph elg(*program, *ai, *o1, *o2, arc.getDependenceType(), true);
        return !elg.isStatus(LatticeGraph::LG_EMPTY);
    }
}

// собирает все параметры в [] и границах счетчиков циклов (границы - канонические!)
// ни в коем случае нельзя вызывать для внутренности []
class GetAllParamsInIndexExpressionsVisitor : public Service::DeepWalker
{
public:
    explicit GetAllParamsInIndexExpressionsVisitor(StatementBase* program):m_weInBrackets(false) 
        {program->accept(*this);}
    std::set<VariableDeclaration*> getParams(){return m_params;}
    std::set<VariableDeclaration*> m_params;
    bool m_weInBrackets;
    void visit(BasicCallExpression& e)
    {
        if (e.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
        {
            m_weInBrackets = true;
            for (int i = 1; i < e.getChildCount(); ++i) e.getChild(i).accept(*this);
            m_weInBrackets = false;
        }
        else DeepWalker::visit(e);
    }
    void visit(ReferenceExpression& e) {if (m_weInBrackets) m_params.insert(&e.getReference());}
    void visit(ForStatement& e)
    {
        m_weInBrackets = true;
        e.getInitExpression().getChild(0).accept(*this);//счетчик тоже нужно добавить
        e.getInitExpression().getChild(1).accept(*this);
        e.getFinalExpression().getChild(1).accept(*this);
        m_weInBrackets = false;
        e.getBody().accept(*this);
    }
};

// получаем списки всех параметров в нужном порядке 
// для о1: (счетчики o1 + нули + внешние)
// для о2: (нули + счетчики o2 + внешние)
std::pair<std::vector<VariableDeclaration*>, std::vector<VariableDeclaration*> >
	getAllParamsInOrder(ExpressionBase *os1, ExpressionBase *os2, StatementBase *program)
{
	std::pair<std::vector<VariableDeclaration*>, std::vector<VariableDeclaration*> > result;

	result.first = OPS::Shared::getIndexVariables(os1, program);
	result.second = OPS::Shared::getIndexVariables(os2, program);

	const size_t firstIndexesSize = result.first.size();
	const size_t secondIndexesSize = result.second.size();

	// находим внешние параметры
	GetAllParamsInIndexExpressionsVisitor v(program);
	std::set<VariableDeclaration*> allParams = v.getParams();
	std::set<VariableDeclaration*>::iterator it;

	// удаляем из множества всех параметров счетчики
	for (size_t i = 0; i < firstIndexesSize; ++i)
		allParams.erase(result.first[i]);
	for (size_t i = 0; i < secondIndexesSize; ++i)
		allParams.erase(result.second[i]);

	// дополняем нолями
	result.first.insert(result.first.end(), secondIndexesSize, 0);
	result.second.insert(result.second.begin(), firstIndexesSize, 0);

	// добавляем внешние параметры
	result.first.insert(result.first.end(), allParams.begin(), allParams.end());
	result.second.insert(result.second.end(), allParams.begin(), allParams.end());

	return result;
}

// низкоуровневая функция, которую используют все остальные функции из этого файла
// она для двух вхождений возвращает полиэдр - набор неравенств для счетчиков циклов и
// набор равенств индексных выражений вхождений
// например для for (i=a; i<=b; i++) x[w1(i)] ... x[w2(i)] функция вернет полиэдр:
// a <= i <= b
// a <= i'<= b
// w1(i) = w2(i')
// равенства будут заменены на систему эквивалентных неравенств
// Все коэфициенты в наборах идут в следующем порядке: свободный член,
// коэффициенты при счетчиках циклов i вхождения o1,
// коэффициенты при счетчиках циклов i' вхождения o2 (включая счетчики общих циклов),
// коэффициенты при внешних параметрах
// массив count содержит 4 элемента: кол-во циклов o1, кол-во циклов o2, кол-во общих циклов, кол-во внеш. параметров
LatticeGraph::Polyhedron buildSystemOfIndexEquationsAndInequalities(BasicOccurrence &o1, BasicOccurrence &o2,
																	OPS::Reprise::StatementBase &program,
																	int *o1EmbracedLoopCount,
																	int *o2EmbracedLoopCount,
																	int *commonEmbracedLoopsCount,
																	int *externalParamsCount)
{
    OPS_ASSERT(o1.getName().m_varDecl == o2.getName().m_varDecl);
    OPS_ASSERT(o1.getBracketCount() == o2.getBracketCount());
    
    LatticeGraph::Polyhedron result;
    
    std::vector<BasicOccurrence*> os(2);
    os[0] = &o1; 
    os[1] = &o2;
    
    std::vector<int> loopCount(os.size());
	loopCount[0] = OPS::Shared::getEmbracedLoopsCount(os[0]->getSourceExpression(), &program);
	loopCount[1] = OPS::Shared::getEmbracedLoopsCount(os[1]->getSourceExpression(), &program);

	if (o1EmbracedLoopCount)
		*o1EmbracedLoopCount = loopCount[0];

	if (o2EmbracedLoopCount)
		*o2EmbracedLoopCount = loopCount[1];

	if (commonEmbracedLoopsCount)
		*commonEmbracedLoopsCount = OPS::Shared::getCommonUpperLoopsAmount(os[0]->getSourceExpression(), os[1]->getSourceExpression(), &program);

    // получаем списки всех параметров в нужном порядке 
    // для о1: (счетчики o1 + нули + внешние)
    // для о2: (нули + счетчики o2 + внешние)
	std::pair<std::vector<VariableDeclaration*>, std::vector<VariableDeclaration*> > allParamsInOrder
			= getAllParamsInOrder(os[0]->getSourceExpression(), os[1]->getSourceExpression(), &program);
    
	std::vector<VariableDeclaration*> externalParams(allParamsInOrder.first.size() - loopCount[0] - loopCount[1]);
	if (externalParamsCount)
		*externalParamsCount = (int)externalParams.size();

    for (size_t i = 0; i < externalParams.size(); ++i)
		externalParams[i] = allParamsInOrder.first[loopCount[0] + loopCount[1] + i];
    
    //границы изменения счетчиков циклов
	LatticeGraph::Polyhedron suppPolyhedron0(*(os[0]), &program, -1, &externalParams);
    //вставляем loopCount[1] нулей после счетчиков циклов
    LatticeGraph::Polyhedron::InequalityList::iterator it = suppPolyhedron0.m_ins.begin();
    for ( ; it != suppPolyhedron0.m_ins.end(); ++it)
        (*it)->insertNZerosBeforeNewParamCoefs(loopCount[1], loopCount[0]);
    result.AddInequalities(suppPolyhedron0);

	LatticeGraph::Polyhedron suppPolyhedron1(*(os[1]), &program, -1, &externalParams);
    //вставляем loopCount[0] нулей до счетчиков циклов
    for (it = suppPolyhedron1.m_ins.begin() ; it != suppPolyhedron1.m_ins.end(); ++it)
        (*it)->insertNZerosBeforeNewParamCoefs(loopCount[0], 0);
    result.AddInequalities(suppPolyhedron1);

    //коэффициенты индексных выражений
    std::vector<LatticeGraph::SimpleLinearExpression> coeffs1, coeffs2;
    OPS::Montego::OccurrenceCoeffs Coeffs1(*(os[0])), Coeffs2(*(os[1]));

	if (!Coeffs1.getAllVarsCoeffsInOrder(coeffs1, allParamsInOrder.first))
		throw OPS::RuntimeError("Non-linear expression encountered in buildSystemOfIndexEquationsAndInequalities()");

	if (!Coeffs2.getAllVarsCoeffsInOrder(coeffs2, allParamsInOrder.second))
		throw OPS::RuntimeError("Non-linear expression encountered in buildSystemOfIndexEquationsAndInequalities()");

    for (int i = 0; i < os[0]->getBracketCount(); ++i)
    {
        coeffs1[i].substract(coeffs2[i]);
        result.AddInequality(coeffs1[i]);
        result.AddInverseInequality(coeffs1[i]);
    }

    return result;
}

// функция, определяющая носители зависимости для пары вхождений
// возвращает массив, кол-во элементов которого равно количеству общих циклов вхождений внутри 
// фрагмента программы (для которого построен граф зависимостей)
// true - если данный цикл является носителем зависимости, false - не является
// если все элементы = false, значит дуга ГИС циклически независимая относительно циклов 
// в рассматриваемом фрагменте программы
// Определение носителя зависимости см. на 27 стр. в диссертации Шульженко
std::vector<bool> findDepSupp(const DependenceGraphAbstractArc& arc, const DependenceGraph& graph)
{
    OccurrencePtr oo1 = arc.getStartVertex().getSourceOccurrence();
    OccurrencePtr oo2 = arc.getEndVertex().getSourceOccurrence();
    BasicOccurrence* o1 = oo1->cast_ptr<BasicOccurrence>();
    BasicOccurrence* o2 = oo2->cast_ptr<BasicOccurrence>();
	StatementBase* program = const_cast<StatementBase*>(&graph.getSourceStatement());
	if ((o1 == 0) || (o2 == 0) ||
		(o1->getBracketCount() != o2->getBracketCount())
		|| (o1->getName().m_varDecl != o2->getName().m_varDecl))
	{
        int commonLoopNum = OPS::Shared::getCommonUpperLoopsAmount(oo1->getSourceExpression(), oo2->getSourceExpression(), program);
		return std::vector<bool>(commonLoopNum, true);
	}
    if (o1->getBracketCount() == 0)
    {
        //скаляры
		std::list<ForStatement*> commonLoops =
            OPS::Shared::getEmbracedLoopsNest(*OPS::Shared::getFirstCommonParent(*o1->getSourceExpression(), *o2->getSourceExpression()), program);
		std::vector<bool> res(commonLoops.size(), true);

		VariableDeclaration* v = o1->getName().m_varDecl;
        if (!v->hasDefinedBlock())  return res;
        
        int i = 0;
        for(std::list<ForStatement*>::iterator it = commonLoops.begin(); it != commonLoops.end(); ++it,++i)
        {
            ForStatement* f = *it;
            res[i] = !OPS::Shared::contain(*f, v->getDefinedBlock());
        }
        return res;
    }

	int commonLoopNum = 0, loopCount1 = 0, loopCount2 = 0, externalParamsNum = 0;
    LatticeGraph::Polyhedron p;

    try
    {
        p = buildSystemOfIndexEquationsAndInequalities(*o1, *o2, *program, &loopCount1, &loopCount2, &commonLoopNum, &externalParamsNum);
    }
    catch(OPS::RuntimeError&)
    {
        return std::vector<bool>(OPS::Shared::getCommonUpperLoopsAmount(oo1->getSourceExpression(), oo2->getSourceExpression(), program), true);
    }

    std::vector<bool> result(commonLoopNum);

    for (int i = 0; i < commonLoopNum; ++i)
    {
        LatticeGraph::Polyhedron p1 = p;
        //добавляем i равенств индексов
        for (int j = 0; j < i; j++)
        {
            LatticeGraph::SimpleLinearExpression s(1 + loopCount1 + loopCount2 + externalParamsNum);
            s.MakeZero();
            s[1+j] = 1;
            s[1+loopCount1+j] = -1;
            p1.AddInequality(s);
            p1.AddInverseInequality(s);
        }
        //добавляем строгое неравентсво для i-того индекса
        LatticeGraph::SimpleLinearExpression s(1 + loopCount1 + loopCount2 + externalParamsNum);
        s.MakeZero();
        s[0] = -1;//для строгости неравенства
        s[1+i] = -1;
        s[1+loopCount1+i] = 1;
        p1.AddInequality(s);
        result[i] = (p1.IsFeasible() != 0);
    }
    
    return result;
}


}//end of namespace
}//end of namespace
