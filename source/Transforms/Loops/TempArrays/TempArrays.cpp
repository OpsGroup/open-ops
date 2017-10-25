//*Введение временных массивов*/
//*Вопросы: Олег 8-903-462-33-24*/
#include <algorithm>
#include <map>
#include <iostream>
#include "Reprise/Reprise.h"
#include "Reprise/Expressions.h"
#include "Reprise/ServiceFunctions.h"
#include "Transforms/Loops/TempArrays/TempArrays.h"
#include "Analysis/DepGraph/DepGraphEx/DepGraphEx.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

using namespace OPS;
using namespace std;
using namespace OPS::Reprise;
using namespace DepGraph;
using namespace Montego;

namespace OPS
{
namespace Transforms
{
namespace Loops
{
//bool MakeTempArrayTrunsform(LampArrow* Arrow, IndOccurContainer& occurList, ForStatement* pFor)
//bool MakeTempArrayTrunsform(DependenceGraph::ArcList::iterator Arrow)
bool MakeTempArrayTransform(DependenceGraph::ArcList::iterator Arrow, ForStatement* pFor)
{
//	OPS_ASSERT(Arrow);

	// Подходит только дуга антизависимости
	if((*Arrow)->getDependenceType() != DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)
		{return false;}

	//!!! Нужно доделать! не обрабатываются структуры и вообще требуется проверять не на массив ли это, а на зависимость относительно счетчика цикла
	// Если зависимые вхождения - вхождения массива (иначе надо применять растягивание скаляров)
	if( (*Arrow)->getStartVertex().getSourceOccurrence()->is_a<BasicOccurrence>())
	{
		if((*Arrow)->getStartVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getBracketCount() == 0)
			{return false;}
	}
	else
		{return false;}

	//string str = (*Arrow)->getStartVertex().getParentStatement()->dumpState();
//	cout << str;
	//string str2 = (*Arrow)->getEndVertex().getParentStatement()->dumpState();
//	cout << str;
	//!!! Нужно доделать! Пока подходит только дуга антизависимости соединяющая вхождения принадлежащие операторам одного блока
	//if( !((*Arrow)->getStartVertex().getParentStatement()->getParentBlock() == (*Arrow)->getEndVertex().getParentStatement()->getParentBlock()) )
//		{return false;}



	std::unique_ptr <BasicCallExpression> left_part;
//	string str = (*Arrow)->getStartVertex().getSourceOccurrence()->getSourceExpression()->dumpState();//->getParent()->cast_to<BasicCallExpression>());//получаем X[...]
//	cout << str;
	BasicCallExpression *temp_bce = &((*Arrow)->getStartVertex().getSourceOccurrence()->getSourceExpression()->cast_to<BasicCallExpression>());//получаем X[...]

//	//случай многомерного массива
//	while((temp_bce->getParent() != NULL) && (temp_bce->getParent()->is_a<BasicCallExpression>()))
//		if(temp_bce->getParent()->cast_to<BasicCallExpression>().getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
//			temp_bce = &(temp_bce->getParent()->cast_to<BasicCallExpression>());//получаем X[...][...]...[...], если дуга соединяет вхождения многомерного цикла
//		else
//			break;



	// Имя сгенерированной переменной
	VariableDeclaration& newVarName = Editing::createNewVariable((*Arrow)->getStartVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getRefExpr()->getReference().getType(), pFor->getParentBlock());
	// Ссылка на сгенерированную переменную
	ReferenceExpression* tempVarRef = new ReferenceExpression (newVarName);

	//имя счетчика цикла
//	VariableDeclaration& VDIterName = Editing::getBasicForCounter(*pFor).clone();//*(occurList[Arrow->depOccurNumb]->loops[occurList[Arrow->depOccurNumb]->loopNumb-1].counterIter);
	//Ссылка на счетчик цикла
//	ReferenceExpression* pFor_counter = new ReferenceExpression (VDIterName);
	ReferenceExpression* pFor_counter = &Editing::getBasicForCounter(*pFor);
	 
	// Левая часть вставляемого оператора присваивания
//	newFor_initExpression = new BasicCallExpression(op(new_counter) R_AS c_IH(0));
	left_part.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, tempVarRef->clone(), pFor_counter->clone()));//получили X[i]

	ReprisePtr<ExpressionBase> newPtr(left_part.release());
	ReprisePtr<ExpressionBase> newPtr2(Editing::replaceExpression(*temp_bce, newPtr));

	// Добавляем оператор присваивания
	left_part.reset(new BasicCallExpression(newPtr.get()->cast_to<BasicCallExpression>()));

	ExpressionBase *temp_assign = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, left_part->clone() ,temp_bce);
	StatementBase *temp_sb = new ExpressionStatement(temp_assign);

	//не нравится как выглядит... наверное надо переделать
	(*Arrow)->getStartVertex().getParentStatement()->getParentBlock().addBefore((*Arrow)->getStartVertex().getParentStatement()->getParentBlock().convertToIterator((*Arrow)->getStartVertex().getParentStatement()), temp_sb);

	return true;
}
}	// Loops
}	// Transforms
}	// OPS
