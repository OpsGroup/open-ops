/*Разрезание циклов*/
/*Вопросы: Олег 8-903-462-33-24*/
#include "Shared/Checks/CompositionCheck/CompositionCheckWalker.h"


#include "Transforms/Loops/LoopDistribution/LoopDistribution.h"
#include "Transforms/Loops/TempArrays/TempArrays.h"
#include "Transforms/Loops/Var2Array/Var2Array.h"
#include "Transforms/Loops/LoopNesting/LoopNesting.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Shared/NodesCollector.h"
#include "Shared/LinearExpressions.h"
#include "Shared/Checks.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <algorithm>
#include "Analysis/DepGraph/DepGraphEx/DepGraphEx.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include <list>


using namespace OPS;
using namespace std;
using namespace DepGraph;
using namespace Montego;
using namespace Reprise;
using namespace OPS::Shared::ExpressionHelpers;

/*Переносит определения переменных в другой блок*/
void newVarDecBlock(BlockStatement *sourceBlock)
{
	//проверки!!! блоки относятся к одной подпрограмме?Есть ли у сорсБлока парент блок

	//цикл проходит по всем переменным VariableList-а который требуется переместить
	Declarations *decls = &(sourceBlock->getDeclarations());
	//перебираем все переменные определенные в подаваемом блоке (sourceBlock)
	for(Declarations::VarIterator it1 = decls->getFirstVar(); it1.isValid(); it1++)
	{

		bool isNeedNewName = false;
		if(it1->hasDefinedBlock())
		{
			//если у переменной есть блок в котором она определена (???А если нет???)
			if (&(it1->getDefinedBlock()) == sourceBlock)
			{
				//перебираем все переменные определенные в родительском блоке подаваемого блока
				for(Declarations::ConstVarIterator it2 = it1; it2 != decls->getLastVar(); it2++)
				{
					//если у найденной переменной (второй) блок в котором она определена является родительским блоком подаваемого блока ипри этом ее имя совпадает с рассматриваемой переменной
					if(&(it2->getDefinedBlock()) == &(sourceBlock->getParent()->getParent()->cast_to<BlockStatement>()))
						if(it1->getName() != it2->getName())
						{isNeedNewName = true;}
				}
				//есть ли необходимость создать новое имя переменной
				if(isNeedNewName)
				{	//создаем новое имя для выносимой переменной
					//VariableDeclaration &new_var = Editing::createNewVariable(it1->getType(), sourceBlock->getParentBlock());
					//ReferenceExpression *name_of_new_var = new ReferenceExpression(new_var);
					//заменяем имеющиеся вхождения переменной it1 в блоке sourceBlock на вхождения созданной меременной
					//OPS::Transforms::Scalar::makeSubstitutionForward(sourceBlock, name_of_new_var, &it1,  true);
					//Удаляем переменную it1
					//BlockStatement::Iterator iter = it1;
					//decls->erase(iter);
				}
				else
					{it1->setDefinedBlock(sourceBlock->getParent()->getParent()->cast_to<BlockStatement>());}
			}
		}
	}
}
/*Перемножение матриц*/
/*Помещает результат во вторую матрицу*/
void MultiplyMatrix(const int N, bool **Matrix1, bool **Matrix2)
{
	int i, j, k;
	bool **MulMatrix=new bool*[N];
	for (i=0; i<N; i++)
		MulMatrix[i]=new bool[N];
	for (i=0; i<N; i++)
		for (j=0; j<N; j++)
			for (k=0; k<N; k++)
				if ((MulMatrix[i][j] = (Matrix1[i][k] && Matrix2[k][j]))) break;
	for (i=0; i<N; i++)
		for (j=0; j<N; j++)
			Matrix2[i][j]=MulMatrix[i][j];
	for (i=0; i<N; i++)
		delete[] MulMatrix[i];
	delete[] MulMatrix;
}
/*Вспомогательная функция для построение матрицы связей*/
//bool NeedFor_BuildMatrix(DependenceGraph &graph, map<StatementBase*, int>::iterator it1, map<StatementBase*, int>::iterator it2, const bool UchetAntiDep = true)
//{
//	DependenceGraph::ArcList arrow_list = graph.getAllArcsBetween(*(it1->first), *(it2->first));
//
//	for(DependenceGraph::ArcList::iterator arc_iter = arrow_list.begin(); arc_iter != arrow_list.end(); ++arc_iter)
//	{
//		if((UchetAntiDep) && (*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)
//			{return true;}
//		if((*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)
//			{return true;}
//		if((*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE)
//			{return true;}
//	}
//	BlockStatement *block_it1, *block_it2;
//	//if(it1 - содержит блок)
//	if(it1->first->is_a<ForStatement>())
//	{
//		block_it1 = it1->first->cast_to<ForStatement>().getBody(); 
//		//if(it2 - содержит блок)
//		{
//		//	block_it1 = 
//
//			for(StatementBase::iterator it3 = block_it1->getFirst(); it3 != ; ++it3)
//			{
//				for(StatementBase::iterator it4 = block_it2->getFirst(); it4 != ; ++it4)
//				{
//					DependenceGraph::ArcList arrow_list = graph.getAllArcsBetween(*(it3->first), *(it4->first));
//
//					return NeedFor_BuildMatrix(graph, it3, it4, UchetAntiDep);
//				}
//			}
//		}
//		//else
//		{
//			DependenceGraph::ArcList arrow_list = graph.getAllArcsBetween(*(it3->first), *(it4->first));
//
//			return NeedFor_BuildMatrix(graph, it3, it4, UchetAntiDep);
//		}
//	}
//}
/*Построение матрицы связей*/
void BuildMatrix(const BlockStatement *InBlock, bool **M, DependenceGraph &graph, map<StatementBase*, int> &St_and_countSt, const bool UchetAntiDep = true)
{
	//БУДЕТ РАБОТАТЬ ТОЛЬКО БЕЗ ВЫЗОВОВ ФУНКЦИЙ
	//не пустой ли блок передан?
	OPS_ASSERT(InBlock != 0);

	DependenceGraph::ArcList arrow_list = graph.getAllArcs();
	for( map<StatementBase*, int>::iterator it1 = St_and_countSt.begin(); it1 != St_and_countSt.end(); ++it1 )
	{

		map<StatementBase*, int>::iterator it2 = it1;
		it2++;
		for(; it2 != St_and_countSt.end(); ++it2 )
		{
			// соберем все ссылки на переменные, имеющиеся в заданных операторах
			Shared::NodesCollector<ReferenceExpression> occurenceCollector1, occurenceCollector2;

			it1->first->accept(occurenceCollector1);
			it2->first->accept(occurenceCollector2);

			for(DependenceGraph::ArcList::iterator arc_iter = arrow_list.begin(); arc_iter != arrow_list.end(); ++arc_iter)
			{
				//БУДЕТ РАБОТАТЬ ТОЛЬКО БЕЗ ВЫЗОВОВ ФУНКЦИЙ (надо переделать)
				if( (find(occurenceCollector1.getCollection().begin() ,occurenceCollector1.getCollection().end() ,(*arc_iter)->getStartVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getRefExpr()) != occurenceCollector1.getCollection().end())
					&& (find(occurenceCollector2.getCollection().begin() ,occurenceCollector2.getCollection().end() ,(*arc_iter)->getEndVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getRefExpr()) != occurenceCollector2.getCollection().end()) )
				{
					if(	(UchetAntiDep) && ((*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_ANTIDEPENDENCE) )
						{M[it1->second-1][it2->second-1] = true;}
					if(	(*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE )
						{M[it1->second-1][it2->second-1] = true;}
					if( (*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE )
						{M[it1->second-1][it2->second-1] = true;}
				}
				if( (find(occurenceCollector2.getCollection().begin() ,occurenceCollector2.getCollection().end() ,(*arc_iter)->getStartVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getRefExpr()) != occurenceCollector2.getCollection().end())
					&& (find(occurenceCollector1.getCollection().begin() ,occurenceCollector1.getCollection().end() ,(*arc_iter)->getEndVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getRefExpr()) != occurenceCollector1.getCollection().end()) )
				{
					if(	(UchetAntiDep) && ((*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_ANTIDEPENDENCE) )
						{M[it2->second-1][it1->second-1] = true;}
					if(	(*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE )
						{M[it2->second-1][it1->second-1] = true;}
					if( (*arc_iter)->getDependenceType() == DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE )
						{M[it2->second-1][it1->second-1] = true;}
				}
			}
		}
	}
}
/*Нахождение компонент сильной связности и расстановка их*/
/*Opers - список операторов, на выходе массив (ч. эл. = ч. операторов в блоке Opers)*/
int *FindComponents(BlockStatement *InBlock, DependenceGraph &graph, int& N, map<StatementBase*, int>& St_and_countSt, int& numb_of_comp, bool UchetAntiDep)
{
	OPS_ASSERT(InBlock != 0);

	N = 0;
	St_and_countSt.clear();
	for (BlockStatement::Iterator iter = InBlock->getFirst(); iter.isValid(); iter++)
	{
		N++;
		St_and_countSt.insert( make_pair( &(*iter ), N) );
	}

//	getNumStatementInBlock(InBlock, N, St_and_countSt);

	bool **Components = new bool*[N],  //Компоненты сильной связности
		 **InMatrix = new bool*[N],  //Начальная матрица
		 **MulMatrix = new bool*[N],
		 **SummMatrix = new bool*[N];
	for (int i = 0; i < N; i++)
	{
		Components[i] = new bool[i];
		InMatrix[i] = new bool[N];
		MulMatrix[i] = new bool[N];
		SummMatrix[i] = new bool[N];
		for (int j = 0; j < N; j++)
			InMatrix[i][j] = false;
	}
///////////////////////////////////////////////////////////////////////
	BuildMatrix(InBlock, InMatrix, graph, St_and_countSt, UchetAntiDep);

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			SummMatrix[i][j] = MulMatrix[i][j] = InMatrix[i][j];
	for (int k = 1; k < N; k++)
	{
		MultiplyMatrix(N, InMatrix, MulMatrix);
		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
				SummMatrix[i][j] = SummMatrix[i][j] || MulMatrix[i][j];
	}
	//Выделяем компоненты сильной в Components
	for (int i = 0; i < N; i++)
		for (int j = 0; j < i; j++)
			Components[i][j] = SummMatrix[i][j] && SummMatrix[j][i];

//Расставляем компоненты по порядку
//разбиение на компоненты
	int *Tmp = new int[N];
	for(int j = 0; j < N; ++j)
		{Tmp[j] = 0;}

	numb_of_comp = 0;
	for (int j = 0; j < N; j++)
	{
		if (!Tmp[j])
		{
			Tmp[j] = --numb_of_comp;
			for (int i = j+1; i < N; i++)
			{
				if (Components[i][j])
					{Tmp[i] = numb_of_comp;}
			}
		}
	}
//определение порядка следования компонент
	numb_of_comp = 0;
	int t;
	for (int j = 0; j < N; j++)
	{
		t = Tmp[j];
		if (t < 0)
		{
			bool b = false;
			for (int j2 = j; j2 < N; j2++)
			{
				if (Tmp[j2] == t)
				{
					for (int i = 0; i < N; i++)
					{
						b = b || (SummMatrix[i][j2] && Tmp[j2] != Tmp[i]);
					}
				}
			}
//перенумерация всей компоненты
			if (!b)
			{
				++numb_of_comp;
				for (int l = 0; l < N; l++)
				{
					if (Tmp[l] == t)
					{
						Tmp[l] = numb_of_comp;
						for (int m = 0; m < N; m++)
							{SummMatrix[l][m] = false;}
					}
				}
				j = -1;//???что это??? для чего это???
			}
		}
	}
	for (int i = 0; i < N; i++)
	{
		delete[] Components[i];
		delete[] InMatrix[i];
		delete[] MulMatrix[i];
		delete[] SummMatrix[i];
	}
	delete[] Components;
	delete[] InMatrix;
	delete[] MulMatrix;
	delete[] SummMatrix;
	return Tmp;
}
/*Ф-ия копирования цикла Loop*/
/*Копирует цикл Pos после самого себя и оставляеь в нём только операторы
из той же компоненты сильной связности*/
void CreateNewLoop(ForStatement* loop, int *Components, int numb_of_comp)
{
	int i = 0;

	ForStatement *newFor = loop->clone();
	loop->getParentBlock().addAfter(loop->getParentBlock().convertToIterator(loop), newFor);

	for(BlockStatement::Iterator iter = newFor->getBody().getFirst(); iter.isValid(); ++i)
	{
		if (Components[i] != numb_of_comp)
		{
			BlockStatement::Iterator tempIter = iter++;
			newFor->getBody().erase(tempIter);
		}
		else
			{iter++;}
	}
}
bool isLoopDistributionPossible(ForStatement* pFor)
{
	//не пустой ли оператор передан?
	OPS_ASSERT(pFor != 0);
	//есть ли тело у цикла?
//	OPS_ASSERT(pFor->getBody() != 0);
	//цикл канонического вида?
	if(!Reprise::Editing::forHeaderIsCanonized(*pFor))
		{return false;}

	////в теле цикла пресутствуют только операторы присваивания?
	//int t_count = 0;
	for (BlockStatement::Iterator iter = pFor->getBody().getFirst(); iter.isValid(); ++iter)
	{
		//if(iter->is_a<GotoStatement>())
		//	{return false;}
		//if(iter->is_a<ReturnStatement>())
		//	{return false;}
		//if(iter->is_a<Canto::HirBreakStatement>())
		//	{return false;}
		//if(iter->is_a<Canto::HirContinueStatement>())
		//	{return false;}
		if(iter->is_a<ExpressionStatement>())
		{
			if(!iter->cast_to<ExpressionStatement>().get().is_a<BasicCallExpression>())
				return false;//в теле цикла пресутствуют не только операторы присваивания
			if(iter->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getKind() != BasicCallExpression::BCK_ASSIGN)
				return false;//в теле цикла пресутствуют не только операторы присваивания
			//N++;
			//St_and_countSt.insert( make_pair( &(*iter ), N) );
			//iter++;
			continue;
		}
		return false;
	}
	return true;
}

namespace OPS
{
namespace Transforms
{
namespace Loops
{
//на вход преобразование подается цикл for и параметр Dynamic значение которого влияет на то будет ли.......
//bool LoopNestingAndDistribution(ForStatement* pFor, bool Var2Array, bool TempArrays)
bool LoopDistribution(ForStatement* pFor, bool Var2Array, bool TempArrays)
{
///////////////////////////удаляем пустые операторы
	for (BlockStatement::Iterator iter = pFor->getBody().getFirst(); iter.isValid(); )
	{
		if(iter->is_a<EmptyStatement>())
		{
			BlockStatement::Iterator tempIter = iter++;
			pFor->getBody().erase(tempIter);
			continue;
		}
		else
			{iter++;}
	}
///////////////////////////
//////начальные проверки на входные данные
	//можно ли применить преобразование к данному циклу
	if(!isLoopDistributionPossible(pFor))
		return false;
	int N = 0;
	map<StatementBase*,int> St_and_countSt;//в этом мапе будут храниться <ссылка на оператор, порядковый номер оператора в блоке(нумерация идет сквозная через все вложенные блоки)>
//	//в теле цикла меньше 2-х операторов присваивания
//	if(N < 2)
//	{return true;}
//////переносим определения переменных находящиеся в блоке цикла pFor в его родительский блок
	newVarDecBlock(&pFor->getBody());


//////создание графа и описание переменных
	DependenceGraph graph(*pFor);
	graph.removeCounterArcs();
	graph.refineAllArcsWithLatticeGraph();
//!!!временная заглушка////////////
	//DependenceGraph::ArcList temp_arrow_list = graph.getAllArcs();
	//for(DependenceGraph::ArcList::iterator iter = temp_arrow_list.begin(); iter != temp_arrow_list.end(); ++iter )
	//{
	//	if(((*iter)->getDependenceType() == DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE) && (!(((*iter)->getStartVertex() == (*iter)->getEndVertex()))))
	//		{TempArrays = false;}
	//}
///////////////////////////////////
	int *Components;
	int numb_of_comp = 0;
/////////////////////////////////////////////////

	if(TempArrays || Var2Array)
	{
		//Поиск и расстановка компонент сильной связности без учета дуг антизависимости
		int *Components2;
		Components2 = FindComponents(&pFor->getBody(), graph, N, St_and_countSt, numb_of_comp, false);//для каждого цикла будет строиться новый мап

		//////// Заполняем лист дугами антизависимости
		DependenceGraph::ArcList arrow_list = graph.getAllArcs();
		DependenceGraph::ArcList anti_dep_list;
		for(DependenceGraph::ArcList::iterator iter = arrow_list.begin(); iter != arrow_list.end(); ++iter )
		{
			// Нам нужны только дуги антизависимости
			if((*iter)->getDependenceType() == DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)
				{anti_dep_list.push_back(*iter);}
		}

		//Обрабатываем отобраные дуги (отсеиваем лишние)
		//оставляем только дуги антизависимости идущие из компаненты с.с. с большим номером в к.с.с. с меньшим номером
		//и находящиеся в одном блоке некоторого цикла for
		for(DependenceGraph::ArcList::iterator iter = anti_dep_list.begin(); iter != anti_dep_list.end();)
		{
			// Получение операторов, в которых имеются вхождения,соединенные дугой антизависимости
			StatementBase* pSrcStmt = (*iter)->getStartVertex().getParentStatement();
			StatementBase* pDepStmt = (*iter)->getEndVertex().getParentStatement();

			// Проверка на то, находятся ли зависимые операторы в одном блоке или нет
			//????????if( pDepStmt->getParentBlock() == pSrcStmt->getParentBlock() )
			{
				//используя мап <указатель на оператор, какой компаненте он принадлежит> узнаем идет ли дуга из компоненты с большим номером в компоненту с меньшим номером
//int tht = St_and_countSt[pSrcStmt]-1;
//int tht2 = St_and_countSt[pDepStmt]-1;

				if( Components2[St_and_countSt[pSrcStmt]-1] > Components2[St_and_countSt[pDepStmt]-1] )
				{
					iter++;
					continue;
				}
			}
			//удаляем дугу из списка дуг на удаление (дуга не подходит для применения преобразования удаляющего дугу)
			DependenceGraph::ArcList::iterator temp_iter = iter++;
			anti_dep_list.erase(temp_iter);
		}

////////////////////////
		//удаляем дуги анизависимости идущие от разных генераторов к одному и тому же использованию (кроме одной)
		//???нужно определить какую из них нужно оставить!!!
		for(DependenceGraph::ArcList::iterator arc_iter = anti_dep_list.begin(); arc_iter != anti_dep_list.end(); ++arc_iter)
		{
			//OPS::Montego::DependenceGraphVertex temp_vertex = (*arc_iter)->getStartVertex();
			DependenceGraph::ArcList::iterator arc_iter2 = arc_iter;
			arc_iter2++;
			for(; arc_iter2 != anti_dep_list.end();)
			{
				//проверяем то действительно ли разные (в этом списке нет одинаковых) дуги идут к одному оператору
				if( (*arc_iter)->getEndVertex() == (*arc_iter2)->getEndVertex() )
				{
					DependenceGraph::ArcList::iterator temp_iter = arc_iter2++;
					anti_dep_list.erase(temp_iter);
				}
				else
				{++arc_iter2;}
			}
		}
////////////////////////

		//удаляем все дуги имеющиеся в списке при помощи соответствующих преобразований
		//for( std::map<int, LampArrow*>::iterator it2 = aDeps.begin(); it2 != aDeps.end(); ++it2 )
		bool is_need_rebuild = false;
		for(DependenceGraph::ArcList::iterator arc_iter = anti_dep_list.begin(); arc_iter != anti_dep_list.end(); ++arc_iter)
		{
		//	LampArrow* currentDep( it2->second );
			if(TempArrays)
			{
				//пытаемся применить преобразование. Если получается, то перестраиваем граф зависимостей
				if(MakeTempArrayTransform(arc_iter, pFor))
				{
					//////перестроение графа и описание переменных
					is_need_rebuild = true;
				}
			}
			if(Var2Array)
			{
				//пытаемся применить преобразование. Если получается, то перестраиваем граф зависимостей
				if(MakeVar2ArrayTransform(arc_iter, pFor))
				{
					////перестроение графа и описание переменных
					is_need_rebuild = true;
				}
			}
		}
		if(is_need_rebuild == true)
		{
			DependenceGraph graph2(*pFor);
			graph2.removeCounterArcs();
			graph2.refineAllArcsWithLatticeGraph();
			Components = FindComponents(&pFor->getBody(), graph2, N, St_and_countSt, numb_of_comp, true);//для каждого цикла будет строиться новый мап
		}
		else
		{
			Components = FindComponents(&pFor->getBody(), graph, N, St_and_countSt, numb_of_comp, true);//для каждого цикла будет строиться новый мап
		}
	}
	else
	{
		Components = FindComponents(&pFor->getBody(), graph, N, St_and_countSt, numb_of_comp, true);//для каждого цикла будет строиться новый мап
	}

	if(numb_of_comp < 2)
		{return true;}
//////вставляем новые циклы (результат разрезания)
	//внутри i-го цикла операторы присваивания принадлежащие i-й компоненте сильной связности
	for (int i = numb_of_comp; i > 0; i--)
		{CreateNewLoop(pFor, Components, i);}

//////удаляем исходный цикл
	BlockStatement::Iterator it = pFor->getParentBlock().convertToIterator(pFor);
	pFor->getParentBlock().erase(it);
return true;
}

////bool LoopDistribution(ForStatement* pFor, BlockStatement::Iterator IterSt, bool VarToArray, bool TempArrays)
////{
//////////начальные проверки на входные данные
////	//можно ли применить преобразование к данному циклу
////	if(!isLoopDistributionPosible(pFor))
////		return false;
////	//дополнительная проверка на то что подаваемый итератор указывает на оператор принадлежащий телу подаваемого цикла
//////	if(&IterSt->getParentBlock() != pFor)
//////		return false;
////
//////////вспомогательные построения и преобразования
////	Id::id a(pFor->getBody());
////	DepGraph::LamportGraph Lamp;
////	Lamp.Build(a);
////
//////////проверяем нет ли дуг идущих из нижней части входного цикла в верхнюю
////	//получаем списки операторов присваивания находящихся внутри начальных циклов
////	for(BlockStatement::Iterator iter1 = IterSt; iter1.isValid(); ++iter1)
////	{
////		for(BlockStatement::Iterator iter2 = pFor->getBody().getFirst(); iter2 != IterSt; ++iter2)
////		{
////			if ((Lamp.TestDep(ANTIDEP, &*iter1, &*iter2)) && (Lamp.TestDep(FLOWDEP, &*iter1, &*iter2)) && (Lamp.TestDep(OUTPUTDEP, &*iter1, &*iter2)))
////				{return false;}
////		}
////	}
////
//////////удаляем исходный цикл
////	BlockStatement::Iterator it = pFor->getParentBlock().convertToIterator(pFor);
////	pFor->getParentBlock().erase(it);
////
////	return true;
////}

//bool LoopDistribution(ForStatement* pFor, bool Var2Array, bool TempArrays)
//{
//	return OPS::Transforms::Loops::IntToVector(pFor);
//}


bool LoopNestingAndDistribution(ForStatement* pFor, bool Var2Array, bool TempArrays)
//bool LoopDistribution(ForStatement* pFor, bool Var2Array, bool TempArrays)
{
//	OPS::Transforms::Loops::IntToVector(pFor);
////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////удаляем пустые операторы/////////////////////////////////////////////
	for (BlockStatement::Iterator iter = pFor->getBody().getFirst(); iter.isValid(); )
	{
		if(iter->is_a<EmptyStatement>())
		{
			BlockStatement::Iterator tempIter = iter++;
			pFor->getBody().erase(tempIter);
			continue;
		}
		else
			{iter++;}
	}
////////////////////////////////////////////////////////////////////////////////////////////////
//////начальные проверки на входные данные//////////////////////////////////////////////////////
	//можно ли применить преобразование к данному циклу
	if(!isLoopDistributionPossible(pFor))
		return false;
////////////////////////////////////////////////////////////////////////////////////////////////
//////переносим определения переменных находящиеся в блоке цикла pFor в его родительский блок///
	newVarDecBlock(&pFor->getBody());
////////////////////////////////////////////////////////////////////////////////////////////////
//////создание графа и описание переменных//////////////////////////////////////////////////////
	DependenceGraph graph(*pFor);
	graph.removeCounterArcs();
	graph.refineAllArcsWithLatticeGraph();
////////////////////////////////////////////////////////////////////////////////////////////////
//////проверяем какие переменные присутствуют в подаваемом цикле
//////если присутстуют только 16-ти битные переменные, то длина внутреннего цикла при гнездовании равна 8
//////если присутстуют 16-ти и 32-х битные переменные, то длина внутреннего цикла при гнездовании равна 4
//////если присутстуют еще какие-то другие переменные, то преобразование применить не получится. окончание работы функции
int flag = 1;
		////ДЛЯ КАЖДОЙ ПЕРЕМЕННОЙ ДЕЛАЕМ ПРОВЕРКУ НА ТО ЯВЛЯЕТСЯ ЛИ ОНА 16-ти бытной или 32-х битной
		//for(по всем переменным в цикле)
		//{
			//if(16-ти битная) {if(flag == 0) {flag = 1}; continue;}
			//if(32-х битная) {flag = 2; continue;}
			////попали сюда, значит не 16-ти и не 32-х битная, а значит
			//flag = 3; break;
		//}
		//MyDeepWalker walker;
		//walker.visit(pFor->getBody());
		//		for (BlockStatement::Iterator iter = pFor->getBody().getFirst(); iter.isValid();iter++)
		//{
		//	MyDeepWalker::visit(*iter);
		//}

//pFor1 - внешний цикл после гнездования, pFor2 -внутренний цикл, pFor3 - хвост
ForStatement /*pFor1,*/ *pFor2 = 0/*, *pFor3*/;
switch(flag)
{
	case 0: //переменных нет
			return false;
		break;
	case 1: //гнездование с внутренним циклом длины 8
			//OPS::Transforms::Loops::makeLoopNesting(*pFor, ReprisePtr<ExpressionBase>(StrictLiteralExpression::createInt16(8)),pFor1, pFor2, pFor3, false);
		break;
	case 2: //гнездование с внутренним циклом длины 4
			//OPS::Transforms::Loops::makeLoopNesting(*pFor, ReprisePtr<ExpressionBase>(StrictLiteralExpression::createInt16(static_cast<sdword>(4))), false);
		break;
	case 3: //преобразование применить не получится. окончание работы функции
			return false;
		break;
}
////////////////////////////////////////////////////////////////////////////////////////////////
//////разрезаем насколько возможно внутренний цикл//////////////////////////////////////////////
	LoopNestingAndDistribution(pFor2);
	//LoopDistribution(pFor2);
////////////////////////////////////////////////////////////////////////////////////////////////
	return true;
}

}	// Loops
}	// Transforms
}	// OPS






//DeepWalker для задачи проверки что внутри фрагмента программы
//присутствуют только 32-битные или 16-ти битные эллементы
/*class MyDeepWalker: public Reprise::Service::DeepWalker
{
	private: int flag;

	public:
	using OPS::Reprise::Service::DeepWalker::visit;

	//MyDeepWalker(){};
	//void visit(StrictLiteralExpression& m_sle);//константы
	void visit(StrictLiteralExpression& m_sle);//константы
	//void visit(BlockStatement& m_bs);//блок
	void visit(ReferenceExpression& m_re);//переменная
	void visit(BasicCallExpression& m_bce);// операции, массивы
	void visit(TypeBase& m_tb);
////узнать размер указателей

//	void visit(StructAccessExpression& m_sae);//структуры (оператор .) (не подходит)
//	void visit(EnumAccessExpression& m_eae);//енум (не подходит)
//	void visit(SubroutineReferenceExpression& m_sre);//обращение к подпрограмме (не подходит)
//	void visit(SubroutineCallExpression& m_sce);// вызов подпрограммы (не подходит)
//	void visit(CompoundLiteralExpression& m_cle);//список выражений в фигурных скобках (не подходит)

//	void visit(TypeCastExpression& m_tce);//приведение типов(x = a + b) (не проверять)
//	void visit(CallExpressionBase& m_ceb);//абстрактный (не проверять)
//	void visit(BasicLiteralExpression& m_ble);//(не проверять)
//	void visit(EmptyExpression& m_ee);//пустой оператор (не нужно проверять)
};

//MyDeepWalker()
//{
//}

void MyDeepWalker::visit(StrictLiteralExpression& m_sle)
{
	BasicType::BasicTypes m_type = m_sle.getLiteralType();
	//типы подходящие для преобразования BT_INT8, BT_INT16, BT_INT32, BT_UINT8, BT_UINT16, BT_UINT32, BT_FLOAT32, BT_BOOLEAN
	if((m_type != BasicType::BT_INT8) && (m_type != BasicType::BT_INT16) && (m_type != BasicType::BT_UINT8) && (m_type != BasicType::BT_UINT16)
		&& (m_type != BasicType::BT_BOOLEAN))
	{
		if( (m_type != BasicType::BT_INT32) && (m_type != BasicType::BT_UINT32) && (m_type != BasicType::BT_FLOAT32) )
		{
			flag = 3;
		 }
		else
		{
			flag = 2;
		 }
	}
	else
	{
		flag = 1;
	}
}
void MyDeepWalker::visit(ReferenceExpression& m_re)
{
	Reprise::VariableDeclaration& varDecl = m_re.getReference();
	//BasicType::BasicTypes m_type = m_re.getResultType()->cast_to<BasicType::BasicTypes>();
	
	visit(*(m_re.getResultType()));
}
void MyDeepWalker::visit(BasicCallExpression& m_bce)
{
	visit(*(m_bce.getResultType()));
}
void MyDeepWalker::visit(TypeBase& m_tb)
{
	if(m_tb.is_a<OPS::Reprise::EnumType>())
	{
		flag = 3;
	}
	if(m_tb.is_a<OPS::Reprise::PtrType>())
	{
		flag = 3;
	}
	if(m_tb.is_a<OPS::Reprise::StructType>())
	{
		flag = 3;
	}
	if(m_tb.is_a<OPS::Reprise::SubroutineType>())
	{
		flag = 3;
	}
	if(m_tb.is_a<OPS::Reprise::TypedefType>())
	{
		flag = 3;
	}
	if(m_tb.is_a<OPS::Reprise::ArrayType>())
	{
		flag = 3;
	}
	if(m_tb.is_a<OPS::Reprise::BasicTypeBase>())
	{
		flag = 3;
	}
	if(m_tb.is_a<OPS::Reprise::DeclaredType>())
	{
		flag = 3;
	}

	//if(m_tb.isConst())
	//{
	//	
	//}
	//if(m_tb.isVolatile())
	//{

	//}
	//OPS::Reprise::EnumType;
	//OPS::Reprise::PtrType;
	//OPS::Reprise::StructType;
	//OPS::Reprise::SubroutineType;
	//OPS::Reprise::TypedefType;
	//OPS::Reprise::ArrayType;
	//OPS::Reprise::BasicTypeBase;
		//OPS::Reprise::BasicType;
		//OPS::Reprise::Canto::HirCBasicType;
		//OPS::Reprise::Canto::HirFArrayType;
	//OPS::Reprise::DeclaredType;

	//bool isConst(void) const;
	//bool isVolatile(void) const;
	//virtual bool isFullType(void) const;

}*/
