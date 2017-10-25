//*Растягивание скаляров*/
//*Вопросы: Олег 8-903-462-33-24*/
#include <algorithm>
#include <map>
#include <iostream>
#include "Reprise/Reprise.h"
#include "Reprise/Expressions.h"
#include "Reprise/ServiceFunctions.h"
#include "Transforms/Loops/Var2Array/Var2Array.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

using namespace OPS;
using namespace std;
using namespace OPS::Reprise;
using namespace Montego;

namespace OPS
{
namespace Transforms
{
namespace Loops
{
bool MakeVar2ArrayTransform(DependenceGraph::ArcList::iterator Arrow, ForStatement* pFor)
{
//	OPS_ASSERT(Arrow);
//	OPS_ASSERT(pFor);
//
//for (i=0; i<100; i=i+1)
//{
//  B[i]=D[i]+C;
//  C=A[i+30]+D[i];
//}
//цикл преобразуется в:
//TypeOfC new_c[101];
//new_c[0] = C;
//for (i=0; i<100; i=i+1)
//{
//  B[i] = D[i] + new_c[i];
//  new_c[i+1] = A[i+30] + D[i];
//}
//C = new_c[100];
//

	//Проверка на то что подаваемая дуга является дугой антизависимости
	if((*Arrow)->getDependenceType() != DependenceGraphAbstractArc::DT_ANTIDEPENDENCE)
		return false;
	// Если зависимые вхождения - вхождения массива (иначе надо применять растягивание скаляров)
	if( !((*Arrow)->getStartVertex().getSourceOccurrence()->getSourceExpression()->is_a<ReferenceExpression>()) )
		return false;

	//std::unique_ptr <BasicCallExpression> left_part;
	//ReferenceExpression *temp_bce = &((*Arrow)->getStartVertex().getSourceOccurrence()->getSourceExpression()->cast_to<ReferenceExpression>());//получаем X

	// Имя сгенерированной переменной
	//OPS::Reprise::TypeBase *newType = new TypeBase((*Arrow)->getStartVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getRefExpr()->getReference().getType());
	
	//OPS::Reprise::ArrayType *newArrayType = new ArrayType(1001, (*Arrow)->getStartVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getRefExpr()->getReference().getType().clone());
	OPS::Reprise::ArrayType *newArrayType = new ArrayType(pFor->getFinalExpression().cast_to<BasicCallExpression>().getArgument(1).cast_to<StrictLiteralExpression>().getInt32() + 1, (*Arrow)->getStartVertex().getSourceOccurrence()->cast_ptr<BasicOccurrence>()->getRefExpr()->getReference().getType().clone());
	//string aaa = newArrayType->dumpState();
	VariableDeclaration& newVarName = Editing::createNewVariable(*newArrayType, pFor->getParentBlock());

	//aaa = newVarName.dumpState();
	// Ссылка на сгенерированную переменную
	ReferenceExpression* tempVarRef = new ReferenceExpression (newVarName);
	//ReferenceExpression* tempVarRef = pFor->getParentBlock().getFirst()->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>().getArgument(0).cast_ptr<ReferenceExpression>();
				// Имя сгенерированной переменной!!!надо сделать массив, а не переменную!!!
				//	VariableDeclaration& newVarName = Editing::createNewVariable(Arrow->pSrcOccur->getReference().getType(), pFor->getParentBlock());

				//это зачем????
				//	if(!pFor->getInitExpression().is_a<BasicCallExpression>())
				//		return false;
				//	if(pFor->getInitExpression().cast_to<BasicCallExpression>().getKind() != BasicCallExpression::BCK_ASSIGN)
				//		return false;



	ReferenceExpression* pFor_counter = &Editing::getBasicForCounter(*pFor);// получаем i счетчик цикла порождающего дугу.

	// Левая часть вставляемого оператора присваивания!!!!!!!!!!!!здесь надо переделать на случай не канонизированного цикла
	std::unique_ptr <BasicCallExpression> temp_bce, left_part, left_part2, bce_left, bce_right;

	//!!!будет правильно только в случае когда цикл канонического вида
	left_part.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, tempVarRef->clone(), &(pFor->getInitExpression().cast_to<BasicCallExpression>().getArgument(1))));//получили new_c[InitExpression]

	ExpressionBase *temp_assign = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, left_part->clone() ,(*Arrow)->getStartVertex().getSourceOccurrence()->getSourceExpression()->clone());//X[i] = C 

	StatementBase *temp_sb = new ExpressionStatement(temp_assign);
	pFor->getParentBlock().addBefore(pFor->getParentBlock().convertToIterator(pFor) ,temp_sb);
	//(*Arrow)->getStartVertex().getParentStatement()->getParentBlock().addBefore((*Arrow)->getStartVertex().getParentStatement()->getParentBlock().convertToIterator((*Arrow)->getStartVertex().getParentStatement()), temp_sb);

	//!!!будет правильно только в случае когда в условии цикла стоит знак меньше!!! в частности если цикл канонического вида
	left_part.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, tempVarRef->clone(), &(pFor->getFinalExpression().cast_to<BasicCallExpression>().getArgument(1))));//получили X[FinalExpression]
	//string aaa = left_part->dumpState();
	//aaa = (*Arrow)->getEndVertex().getSourceOccurrence()->getSourceExpression()->dumpState();

	ExpressionBase *temp_assign2 = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, (*Arrow)->getEndVertex().getSourceOccurrence()->getSourceExpression()->clone(), left_part->clone());
	StatementBase *temp_sb2 = new ExpressionStatement(temp_assign2);
	pFor->getParentBlock().addAfter(pFor->getParentBlock().convertToIterator(pFor) ,temp_sb2);
	
	//for()
	{
		bce_left.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, tempVarRef->clone(), pFor_counter->clone()));//temp_c[i]
		left_part2.reset(bce_left->clone());
		ReprisePtr<ExpressionBase> newPtr(left_part2.release());
		ReprisePtr<ExpressionBase> newPtr2(Editing::replaceExpression(*(*Arrow)->getStartVertex().getSourceOccurrence()->getSourceExpression(), newPtr));

		StrictLiteralExpression *temp_sle = StrictLiteralExpression::createInt32(1);//получили 1
		temp_bce.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS, pFor_counter->clone(), temp_sle));//получили i+1
		bce_left.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, tempVarRef->clone(), temp_bce->clone()));//temp_c[i+1]
		ReprisePtr<ExpressionBase> newPtr3(bce_left.release());
		ReprisePtr<ExpressionBase> newPtr4(Editing::replaceExpression(*(*Arrow)->getEndVertex().getSourceOccurrence()->getSourceExpression(), newPtr3));
	}

	return true;
}
}	// Loops
}	// Transforms
}	// OPS
