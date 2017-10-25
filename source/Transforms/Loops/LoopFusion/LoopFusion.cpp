/*Разрезание циклов*/
/*Вопросы: Олег 8-903-462-33-24*/
#include "Transforms/Loops/LoopDistribution/LoopDistribution.h"
#include "Transforms/Loops/LoopFusion/LoopFusion.h"

#include <cassert>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"


using namespace OPS;
using namespace std;
using namespace DepGraph;
using namespace Montego;
using namespace OPS::Reprise;


bool isLoopFusionPossible(ForStatement* loop1, ForStatement* loop2)
{
	//не пустой ли оператор передан?
	OPS_ASSERT(loop1 != 0);
	OPS_ASSERT(loop2 != 0);

	//заголовки циклов совпадают
	if(!(loop1->getInitExpression().isEqual(loop2->getInitExpression())))
		return false;//начальные условия не совпадают
	if(!(loop1->getFinalExpression().isEqual(loop2->getFinalExpression())))
		return false;//условия выхода не совпадают
	if(!(loop1->getStepExpression().isEqual(loop2->getStepExpression())))
		return false;//шаги итераторов не совпадают

	//в теле первого цикла пресутствуют только операторы присваивания?
	for (BlockStatement::Iterator iter = loop1->getBody().getFirst(); iter.isValid(); ++iter)
	{
		if(!iter->is_a<ExpressionStatement>())
			return false;//в теле цикла пресутствуют не только операторы присваивания
		if(!iter->cast_to<ExpressionStatement>().get().is_a<BasicCallExpression>())
			return false;//в теле цикла пресутствуют не только операторы присваивания
		if(iter->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getKind() != BasicCallExpression::BCK_ASSIGN)
			return false;//в теле цикла пресутствуют не только операторы присваивания
	}
	//в теле второго цикла пресутствуют только операторы присваивания?
	for (BlockStatement::Iterator iter = loop2->getBody().getFirst(); iter.isValid(); ++iter)
	{
		if(!iter->is_a<ExpressionStatement>())
			return false;//в теле цикла пресутствуют не только операторы присваивания
		if(!iter->cast_to<ExpressionStatement>().get().is_a<BasicCallExpression>())
			return false;//в теле цикла пресутствуют не только операторы присваивания
		if(iter->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getKind() != BasicCallExpression::BCK_ASSIGN)
			return false;//в теле цикла пресутствуют не только операторы присваивания
	}

	//циклы loop1 и loop2 следуют непосредственно друг за другом?
	BlockStatement::Iterator iter = loop1->getParentBlock().convertToIterator(loop1);
	OPS_ASSERT(iter.isValid());
	BlockStatement::Iterator iter2 = loop2->getParentBlock().convertToIterator(loop2);
	OPS_ASSERT(iter2.isValid());
	
	iter++;
	if(iter != iter2)
		return false;

	//не стоит ли метки на заголовке второго цикла?
	//????????????
	if(loop2->hasLabel())
		return false;

	return true;
}



namespace OPS
{
namespace Transforms
{
namespace Loops
{
//на вход преобразование подается два цикла for и параметры VarToArray и TempArrays значение которого влияет на то будут ли использоваться соответствующие преобразования
bool LoopFusion(ForStatement* loop1, ForStatement* loop2, bool VarToArray, bool TempArrays)
{
//////начальные проверки на входные данные
	//можно ли применить преобразование к данным циклам
	if(!isLoopFusionPossible(loop1, loop2))
		return false;

////////формируем результирующий код
	//создаем новый цикл в котором вставляем операторы второго цикла следом за операторами первого
	//ForStatement* loop = loop1->clone();
	//loop1->getParentBlock().addAfter(loop1->getParentBlock().convertToIterator(loop1), loop);


	//вставляем операторы второго цикла следом за операторами первого
	//если же проверка покажет что присутствуют зависимости которые мешают выполнению преобразования,
	//то просто деелаем return false; (в следствии чего все изменения будут анулированы)
	BlockStatement::Iterator iter = loop2->getBody().getFirst();
	BlockStatement::Iterator iter_half = loop1->getBody().addLast(&*iter->clone());
	iter++;
	for(; iter.isValid();++iter)
	{
		loop1->getBody().addLast(&*iter->clone());
	}

	//проверяем нет ли дуг идущих от тела второго оператора цикла к телу первого
	//проверяем не идут ли дуги из второго списка в первый
	Id::id a(loop1->getBody());
	DepGraph::LamportGraph Lamp;
	Lamp.Build(a);

	//DependenceGraph graph(loop1->getBody());
	//graph.refineAllArcsWithLatticeGraph();



	//// соберем все ссылки на переменные, имеющиеся в заданных операторах
	//Shared::NodesCollector<ReferenceExpression> occurenceCollector1, occurenceCollector2;

	//it1->first->accept(occurenceCollector1);
	//it2->first->accept(occurenceCollector2);




	for(BlockStatement::Iterator iter1 = loop1->getBody().getFirst(); iter1 != iter_half; ++iter1)
	{
		for(BlockStatement::Iterator iter2 = iter_half; iter2.isValid(); ++iter2)
		{

			//if(graph.)
			if ((Lamp.TestDep(ANTIDEP, &*iter2, &*iter1)) || (Lamp.TestDep(FLOWDEP, &*iter2, &*iter1)) || (Lamp.TestDep(OUTPUTDEP, &*iter2, &*iter1)))
			{
				return false;
				//удаляем исходный первый цикл
				//iter = loop1->getParentBlock().convertToIterator(loop);
				//loop1->getParentBlock().erase(iter);
			}
		}
	}

	//удаляем исходный первый цикл
	//iter = loop1->getParentBlock().convertToIterator(loop1);
	//loop1->getParentBlock().erase(iter);
	//удаляем исходный второй цикл
	iter = loop2->getParentBlock().convertToIterator(loop2);
	loop2->getParentBlock().erase(iter);



	{
//////////	StatementBase* temp1_b = &(*loop1->getBody().getFirst());
//////////	StatementBase* temp1_e = &(*loop1->getBody().getLast());
//////////	StatementBase* temp2_b = &(*loop2->getBody().getFirst());
//////////	StatementBase* temp2_e = &(*loop2->getBody().getLast());
//////////
//////////	//добавляем тело второго цикла в конец тела первого цикла
//////////	loop1->getBody().addLast(&loop2->getBody());
//////////	//удаляем исходный второй цикл
//////////	BlockStatement::Iterator iter = loop2->getParentBlock().convertToIterator(loop2);
//////////	loop2->getParentBlock().erase(iter);
//////////
////////////////проверяем нет ли дуг идущих от тела второго цикла к телу первого
//////////	//проверяем не идут ли дуги из второго списка в первый
//////////	Id::id a(loop1->getBody());
//////////	DepGraph::LamportGraph Lamp;
//////////	Lamp.Build(a);
//////////
//////////	BlockStatement::Iterator iter_loop1_begin = loop1->getBody().convertToIterator(temp1_b);
//////////	BlockStatement::Iterator iter_loop1_end = loop1->getBody().convertToIterator(temp1_e);
//////////	BlockStatement::Iterator iter_loop2_begin = loop1->getBody().convertToIterator(temp2_b);
//////////	BlockStatement::Iterator iter_loop2_end = loop1->getBody().convertToIterator(temp2_e);
//////////	iter_loop1_end++;
//////////	iter_loop2_end++;
//////////	for(BlockStatement::Iterator iter1 = iter_loop2_begin; iter1 != iter_loop2_end; ++iter1)
//////////	{
//////////		for(BlockStatement::Iterator iter2 = iter_loop1_begin; iter2 != iter_loop1_end; ++iter2)
//////////		{
//////////			if ((Lamp.TestDep(ANTIDEP, &*iter1, &*iter2)) || (Lamp.TestDep(FLOWDEP, &*iter1, &*iter2)) || (Lamp.TestDep(OUTPUTDEP, &*iter1, &*iter2)))
//////////			{
//////////				return false;
//////////			}
//////////		}
//////////	}
	}
return true;
}
}	// Loops
}	// Transforms
}	// OPS
