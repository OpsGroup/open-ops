#pragma once

/*
Здесь содержатся функции, анализирующие индексные выражения вхождений и границы изменения
счетчиков охватывающих циклов, в том числе функции уточнения завиcимостей, определения 
носителя зависимости и т.п.
Перед применением указанных функций фрагмент программы нужно проверить функцией testApplicability.
Если фрагмент программы не является допустимым, указанные функции будут возвращать неверные результаты.
*/

#include "Analysis/Montego/Occurrence.h"
#include "Reprise/Reprise.h"
#include "Analysis/LatticeGraph/LinearLib.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraphArc.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

namespace OPS
{
namespace Montego
{

// проверяет, применимы ли функции анализа индексных выражений к указанному фрагменту программы
// возвращает true - применимы, false - неприменимы
bool testApplicability(OPS::Reprise::StatementBase& program, OccurrenceContainer& cont, AliasInterface& ai);

// на основе анализа индексных выражений вхождений и циклов, содержащих вхождение и находящихся внутри 
// фрагмента программы (для которого построен граф зависимостей),
// выясняет могут ли два вхождения обращаться к одной и той же ячейке памяти
// возвращает true - могут обращаться, false - не могут
// данная функция использует алгоритмы решетчатого графа. 
// Если flagUseFast = true, то она работает более чем в 5 раз быстрее, чем проверка решетчатым графом,
// но при этом тестируются все зависимости одновременно: т.е. проверяется могут ли вхождения обращаться к
// одной ячейке памяти
bool testDependenceWithLatticeGraph(const DependenceGraphAbstractArc& arc, const DependenceGraph& graph, bool flagUseFast);

// получаем списки всех параметров в нужном порядке
// для о1: (счетчики o1 + нули + внешние)
// для о2: (нули + счетчики o2 + внешние)
std::pair<std::vector<OPS::Reprise::VariableDeclaration*>, std::vector<OPS::Reprise::VariableDeclaration*> >
	getAllParamsInOrder(Reprise::ExpressionBase* os1, Reprise::ExpressionBase* os2, OPS::Reprise::StatementBase* program);

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
LatticeGraph::Polyhedron buildSystemOfIndexEquationsAndInequalities(
	BasicOccurrence& o1,
	BasicOccurrence& o2,
	OPS::Reprise::StatementBase& program,
	int* o1EmbracedLoopCount = 0,
	int* o2EmbracedLoopCount = 0,
	int* commonEmbracedLoopsCount = 0,
	int* externalParamsCount = 0);

// функция, определяющая носители зависимости для пары вхождений
// возвращает массив, кол-во элементов которого равно количеству общих циклов вхождений внутри 
// фрагмента программы (для которого построен граф зависимостей)
// true - если данный цикл является носителем зависимости, false - не является
// если все элементы = false, значит дуга ГИС циклически независимая относительно циклов 
// в рассматриваемом фрагменте программы
// Определение носителя зависимости см. на 27 стр. в диссертации Шульженко
std::vector<bool> findDepSupp(const DependenceGraphAbstractArc& arc, const DependenceGraph& graph);



}//end of namespace
}//end of namespace
