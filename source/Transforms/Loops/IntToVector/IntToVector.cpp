/**/
/*Вопросы: Олег 8-903-462-33-24*/
//нужно что-то делать со случаем нахождения массива во вложенном цикле
//for(){
//for(){
//a[i]}}

#include "Analysis/ConsistCheck/Conditions.h"
#include "Analysis/ConsistCheck/IConsistCheckService.h"
#include "Shared/Checks/CompositionCheck/CompositionCheckWalker.h"
#include "Shared/ExpressionHelpers.h"


//#include "Transforms/Loops/LoopDistribution/LoopDistribution.h"
//#include "Transforms/Loops/TempArrays/TempArrays.h"
//#include "Transforms/Loops/Var2Array/Var2Array.h"
//#include "Transforms/Loops/LoopNesting/LoopNesting.h"
//#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
//#include "Shared/NodesCollector.h"
//#include "Shared/LinearExpressions.h"
//#include "Shared/Checkes/Checks.h"

//#include <cassert>
//#include <vector>
//#include <list>
//#include <iostream>
//#include <algorithm>
//#include "Analysis/DepGraph/DepGraphEx/DepGraphEx.h"
//#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

using namespace OPS::Analysis;
using namespace OPS;
using namespace std;
using namespace Reprise;
using namespace OPS::Shared::ExpressionHelpers;

typedef std::list<BasicCallExpression*> BasicCallExpressions;
typedef std::list<ReferenceExpression*> ReferenceExpressions;

class IntToVectorWalker: public Reprise::Service::DeepWalker
{
	private:
		BasicCallExpressions array_list;
		ReferenceExpressions reference_list;
	public:

	void visit(BasicCallExpression& m_bce);//[]
	void visit(StrictLiteralExpression& m_sle);//[]

	BasicCallExpressions& GetArrayList();
	ReferenceExpressions& GetReferenceList();
};


void IntToVectorWalker::visit(BasicCallExpression &m_bce)
{
	if(m_bce.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)//рассматриваем вхождение []
	{
		if(m_bce.getParent()->is_a<BasicCallExpression>())//учитываем, то что нам не подходит случай многомерного массива
		{
			if(m_bce.getParent()->cast_to<BasicCallExpression>().getKind() != BasicCallExpression::BCK_ARRAY_ACCESS)
			{
				array_list.push_back(&m_bce);
				reference_list.push_back(&(m_bce.getArgument(0).cast_to<ReferenceExpression>()));
			}
		}
		else
		{
			array_list.push_back(&m_bce);
			reference_list.push_back(&(m_bce.getArgument(0).cast_to<ReferenceExpression>()));
		}
	}
	DeepWalker::visit(m_bce);
}
void IntToVectorWalker::visit(StrictLiteralExpression &m_sle)
{

}

BasicCallExpressions& IntToVectorWalker::GetArrayList()
{
	return array_list;
}

ReferenceExpressions& IntToVectorWalker::GetReferenceList()
{
	return reference_list;
}

void replaceSingle(BasicCallExpression* array_to_replace)
{
		//для данного эллемента [] создать выражение на которое он должен быть заменен
		
		BasicCallExpression *temp_bce, *temp_bce2;
		//создаем *
		temp_bce = new BasicCallExpression(BasicCallExpression::BCK_DE_REFERENCE);

		//создаем a+i
		temp_bce2 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS, (array_to_replace)->getArgument(0).clone(),(array_to_replace)->getArgument(1).clone());

		//делаем ей потомка RepriseBase::Type_cast* (создать описание v4si*)
		VectorType* temp_vt = new VectorType(4, BasicType::basicType(BasicType::BT_INT32));
		PtrType* cast_to = new PtrType(temp_vt);
		
		TypeCastExpression *temp_tce = new TypeCastExpression(cast_to, temp_bce2);

		//*(v4si*)(a+i)
		temp_bce->addArgument(temp_tce);

		//ExpressionBase *temp_eb = new BasicCallExpression(&array_to_replace);
		OPS::Reprise::ReprisePtr<BasicCallExpression> temp(temp_bce);
		Reprise::Editing::replaceExpression(*array_to_replace, temp);

//		array_to_replace->getParent
//		VariableDeclaration& newVarName = Editing::createNewVariable(*temp_vt, ->getParentBlock());
}
void replaceAll(BasicCallExpressions& array_list)
{
	for( BasicCallExpressions::iterator iter = array_list.begin(); iter != array_list.end(); ++iter )
	{
		replaceSingle(*iter);
	}
}

bool isIntToVectorPossible(ForStatement* pFor)
{
	//не пустой ли оператор передан?
	OPS_ASSERT(pFor != 0);
	//есть ли тело у цикла?
//	OPS_ASSERT(pFor->getBody() != 0);
	//цикл канонического вида?
	if(!Reprise::Editing::forHeaderIsCanonized(*pFor))
		{return false;}

	//проверка на то что длина цикла кратна 4
//	if(Editing::getBasicForFinalExpression(*pFor)/4 != 0)
//		{return false;}



	
	////в теле цикла пресутствуют только операторы присваивания?
	for (BlockStatement::Iterator iter = pFor->getBody().getFirst(); iter.isValid(); ++iter)
	{
		if(iter->is_a<ExpressionStatement>())
		{
			if(!iter->cast_to<ExpressionStatement>().get().is_a<BasicCallExpression>())
				return false;//в теле цикла пресутствуют не только операторы присваивания
			if(iter->cast_to<ExpressionStatement>().get().cast_to<BasicCallExpression>().getKind() != BasicCallExpression::BCK_ASSIGN)
				return false;//в теле цикла пресутствуют не только операторы присваивания
			continue;
		}
		return false;
	}


	ConditionList conditions;
	addCondition(conditions, new StatementCondition<EmptyStatement>());
	addCondition(conditions, new StatementCondition<ExpressionStatement>());
	addCondition(conditions, new RepriseCondition<EmptyExpression>());
	//addCondition(conditions, new RepriseCondition<StrictLiteralExpression>());
	addCondition(conditions, new RepriseCondition<BasicCallExpression>());
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_INT32));
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_INT32));
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_INT32));
	//break, continue


	bool result = getConsistCheckService().matchTo(pFor->getBody(),
			conditions);
	if(!result)
		return false;

	return true;
}


namespace OPS
{
namespace Transforms
{
namespace Loops
{
bool IntToVector(ForStatement* pFor)
{
	if(!isIntToVectorPossible(pFor))
		return false;
		
	IntegerHelper c(BasicType::BT_INT32);
	//берем счетчик цикла
	ReferenceExpression* pFor_counter = Editing::getBasicForCounter(*pFor).clone();
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
//создается цикл содержащий остаток итераций исходного не попадающий под преобразование
	//if((Editing::getBasicForFinalExpression(*pFor)).is_a<StrictLiteralExpression>())//нужно ли?????
	//{
	//	ExpressionBase &pFor_finalExpression = Editing::getBasicForFinalExpression(*pFor) - Editing::getBasicForFinalExpression(*pFor) % c(4);
	//	pFor->setFinalExpression(&pFor_finalExpression);

	//}
	//else
	{
		ForStatement *pFor_teil = pFor->clone();
		pFor->getParentBlock().addAfter(pFor->getParentBlock().convertToIterator(pFor) ,pFor_teil);
		//ReferenceExpression* pFor_counter = Editing::getBasicForCounter(*pFor_teil).clone();
		ExpressionBase &pFor_teil_finalExpression = Editing::getBasicForFinalExpression(*pFor_teil) % c(4);
		pFor_teil->setFinalExpression(&pFor_teil_finalExpression);

		//(после отбрасывания хвоста)меняется првая граница исходного цикла
		ExpressionBase &pFor_finalExpression = Editing::getBasicForFinalExpression(*pFor) - Editing::getBasicForFinalExpression(*pFor) % c(4);
		pFor->setFinalExpression(&pFor_finalExpression);
	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////

	IntToVectorWalker walker;
	//создается список указателей на []
	pFor->getBody().accept(walker);
	//a[i] во внутреннем представлении заменяются на *(*v4si)(a+i)
	replaceAll(walker.GetArrayList());
	
	
	
	//меняем шаг цикла
	ExpressionBase& stepExpression = op(pFor_counter) R_AS op(pFor_counter) + c(4);
	pFor->setStepExpression(&stepExpression);

	return true;
}
}	// Loops
}	// Transforms
}	// OPS










/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////



//class IsPosible_Walker: public Reprise::Service::DeepWalker
//{
//	private:
//		bool is_posible = 1;
//	public:
//
//	void visit(BasicCallExpression& m_bce);//[]
//	void visit(ExpressionBase& m_eb);//
//
//	bool isPosible();
//	
//
//};
//
//IsPosible_Walker::isPosible()
//{
//	return is_posible;
//}
//
//
//
////проверка должна показать что внутри блока присутствуют только допустимые эллементы (конструкции???)
////а именно: могут присутствовать блоки, операторы присваивания, скалярные переменные, массивы, 
//void IsPosible_Walker::visit(OPS::Reprise::ExpressionBase &m_eb)
//{
////////////////////////////////////////////////затем это нужно будет перенести в другое место
////эти вызовы нужно кинуть после того как проверки скажут что в коде присутствуют только допустимые конструкции
////вызовы применяются для левой и правой части оператора присваивания
//	IsPosible_Walker walker;
//	for (BlockStatement::Iterator iter = pFor->getBody().getFirst(); iter.isValid();iter++)
//	{
//		walker.is_posible = 1;
//
//		//далее, здесь нужна проверка на то что это оператор присваивания
//		visit(m_bce.getArgument(0));
//		if(walker.is_posible)
//			{visit(m_bce.getArgument(1));}
//		
//		//к этому моменту известно что все переменные в операторе присваивания могут быть(не могут быть)
//		//преобразованы к векторному типу. Соответственно преобразование всех переменных к векторному типу
//		if(walker.is_posible)
//		{
//			
//		}
//	}
////////////////////////////////////////////////
//}
//
//
//void IsPosible_Walker::visit(OPS::Reprise::BasicCallExpression &m_bce)
//{
//	//рассматривается только случай когда m_bce - массив. Другие не интересны.
//	if(m_bce.is_a<BCK_ARRAY_ACCESS>())
//	{
//		if(m_bce.getArgument(0).getResultType()->is_a<BasicType>() == true)
//		{
//			if(m_bce.getArgument(0).getResultType()->cast_to<BasicType>().getKind() == BasicType::BasicTypes::BT_INT32)
//			{
//				m_bce.getArgument(1).
//			}
//			else
//				{is_posible = 0;}
//		}
//	}
//}
//
//**/
//void IntToVector(BlockStatement *sourceBlock)
//{
//	//проверка на то что внутри подаваемого блока присутствуют только операторы присваивания,
//	//константы, ReferenceExpression
//	//OPS::Shared::Checks::CompositionCheckObjects m_cco;
//	//a << OPS::Shared::Checks::CompositionCheckObjects::CCOT_ExpressionStatement << OPS::Shared::Checks::CompositionCheckObjects::;
//
//
//	//IntToVectorWalker walker(1);
////		walker.
//	//if()
//	//{
//	//}
//}