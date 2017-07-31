#include <map>
#include <set>
#include "FrontTransforms/ExpressionSimplifier.h"
#include "BasicTypesHelper.h"
#include "Operators.h"
#include "Exceptions.h"

using namespace std;
using namespace OPS::Reprise;
using namespace OPS::ExpressionSimplifier::Helpers;
using namespace OPS::ExpressionSimplifier::Calculation;

namespace OPS
{
namespace ExpressionSimplifier
{

Simplifier::Simplifier()
{
	operators[BasicCallExpression::BCK_BINARY_PLUS] = new BinaryPlusOperator();
	operators[BasicCallExpression::BCK_MULTIPLY] = new MultiplyOperator();
	operators[BasicCallExpression::BCK_BINARY_MINUS] = new BinaryMinusOperator();

	associativeOperators.insert(BasicCallExpression::BCK_BITWISE_AND);
	associativeOperators.insert(BasicCallExpression::BCK_BITWISE_OR);
	associativeOperators.insert(BasicCallExpression::BCK_BITWISE_XOR);
	associativeOperators.insert(BasicCallExpression::BCK_LOGICAL_OR);
	associativeOperators.insert(BasicCallExpression::BCK_LOGICAL_AND);
	associativeOperators.insert(BasicCallExpression::BCK_BINARY_PLUS);
	associativeOperators.insert(BasicCallExpression::BCK_MULTIPLY);

	commutativeOperators.insert(BasicCallExpression::BCK_BITWISE_AND);
	commutativeOperators.insert(BasicCallExpression::BCK_BITWISE_OR);
	commutativeOperators.insert(BasicCallExpression::BCK_BITWISE_XOR);
	commutativeOperators.insert(BasicCallExpression::BCK_LOGICAL_OR);
	commutativeOperators.insert(BasicCallExpression::BCK_LOGICAL_AND);
	commutativeOperators.insert(BasicCallExpression::BCK_BINARY_PLUS);
	commutativeOperators.insert(BasicCallExpression::BCK_MULTIPLY);
}

Simplifier::~Simplifier()
{
	OperatorsMap::iterator it = operators.begin();
	for(; it != operators.end(); ++it)
	{
		delete it->second;
	}
}

// Функция меняет поддеревья местами. 
// Attention: Нет проверки на эквивалентность!
static void swapTrees(ExpressionBase *arg0, ExpressionBase *arg1)
{
	ReprisePtr<ExpressionBase> rightArg(arg1);
	
	CallExpressionBase *expr0 = dynamic_cast<CallExpressionBase*>(arg0->getParent());
	CallExpressionBase *expr1 = dynamic_cast<CallExpressionBase*>(arg1->getParent());
	
	expr1->replaceArgument(*arg1, ReprisePtr<ExpressionBase>(arg0));
	expr0->replaceArgument(*arg0, rightArg);	
}

ExpressionBase* Simplifier::simplify(const ExpressionBase* const inputExpression)
{	
	if (const ReferenceExpression *newExpression = dynamic_cast<const ReferenceExpression*>(inputExpression))
	{
		const ExpressionBase& initExpr = newExpression->getReference().getInitExpression();
		const EmptyExpression *bbb = dynamic_cast<const EmptyExpression*>(&initExpr);
		if (newExpression->getReference().getType().isConst()  && bbb == 0)
			return simplify(&initExpr);
		else
			return inputExpression->clone();				
	} else
    if (const BasicCallExpression *newExpression = dynamic_cast<const BasicCallExpression*>(inputExpression)) 
		return simplify(newExpression);
    else 
	if (const TypeCastExpression *newExpression = dynamic_cast<const TypeCastExpression*>(inputExpression)) 
		return simplify(newExpression);
	else return inputExpression->clone();	
}

ExpressionBase* Simplifier::private_SimplifyBinaryAssociativeAndCommutativeOperation(BasicCallExpression*& inputExpression)
{
	OPS_ASSERT(inputExpression);
	BasicCallExpression::BuiltinCallKind callKindOfInputExpression = inputExpression->getKind();

	OPS_ASSERT(inputExpression->getArgumentCount() == 2 && 
		associativeOperators.count(callKindOfInputExpression) &&
		commutativeOperators.count(callKindOfInputExpression));

	StrictLiteralExpression *leftArgument = dynamic_cast<StrictLiteralExpression*>(&inputExpression->getArgument(0));
	if (! leftArgument) 
	{
		swapTrees(&inputExpression->getArgument(0),&inputExpression->getArgument(1));
		leftArgument = dynamic_cast<StrictLiteralExpression*>(&inputExpression->getArgument(0));
	}

	BasicCallExpression *rightExpression = dynamic_cast<BasicCallExpression*>(&inputExpression->getArgument(1));
	if (!rightExpression || rightExpression->getKind() != callKindOfInputExpression) return 0;

	StrictLiteralExpression *leftArgumentOfRightExpression = dynamic_cast<StrictLiteralExpression*>(&rightExpression->getArgument(0));
	if (! leftArgumentOfRightExpression) 
	{
		swapTrees(&rightExpression->getArgument(0), &rightExpression->getArgument(1));
		leftArgumentOfRightExpression = dynamic_cast<StrictLiteralExpression*>(&rightExpression->getArgument(0));
	}

	if (leftArgumentOfRightExpression)
	{
		if (leftArgument)
		{
			// literal0 + (literal1 + expr0) -> literal3 + expr0
			if (BasicTypesHelper::isNumeric(leftArgument) && BasicTypesHelper::isNumeric(leftArgumentOfRightExpression))
			{
				BasicCallExpression* bufOp = new BasicCallExpression(inputExpression->getKind());					
				bufOp->addArgument(leftArgument->clone());
				bufOp->addArgument(leftArgumentOfRightExpression->clone());
				
				OperatorsMap::iterator it = operators.find(callKindOfInputExpression);				
				ExpressionBase* result = 0;
				if (ExpressionBase *bufSimplified = (*it).second->calculate(bufOp))
				{
					rightExpression->replaceArgument( *leftArgumentOfRightExpression, ReprisePtr<ExpressionBase>(bufSimplified) );
					result = rightExpression->clone();											
				}
				delete bufOp;
				return result;			
			}
		}
		else
		{
			// expr0 + (literal0 + expr1) -> literal0 + (expr0 + expr1) 	
			swapTrees(&inputExpression->getArgument(0), leftArgumentOfRightExpression);
			return inputExpression->clone();
		}
	}
	return 0;
}

ExpressionBase* Simplifier::simplify(const BasicCallExpression* const inputExpression)
{	
	// Сначала упрощением аргументы inputExpression
	vector<ExpressionBase*> inputExpressionArguments(inputExpression->getArgumentCount());
	for (int i = 0; i < inputExpression->getArgumentCount(); ++i)
		inputExpressionArguments[i] = simplify(&inputExpression->getArgument(i));
	
	// Создаем новый узел, который будет подвергнут упрощениям, 
	// и, в конечном итоге, он будет составлять результат применения оптимизаций!
	BasicCallExpression *resultExpression = new BasicCallExpression(inputExpression->getKind());
	for (size_t i = 0; i < inputExpressionArguments.size(); ++i)
		resultExpression->addArgument(inputExpressionArguments[i]);

	BasicCallExpression::BuiltinCallKind opCallKind = resultExpression->getKind();
	OperatorsMap::iterator it = operators.find(opCallKind);
	if (it != operators.end()) // найден оператор, который может упростить выражение
	{
		//Попытка упростить выражение, используя правила выбранного оператора 			
		if (ExpressionBase *newResultExpression = (*it).second->calculate(resultExpression))
		{
			delete resultExpression;
			return newResultExpression;	
		}
		if (resultExpression->getArgumentCount() == 2 && 
				associativeOperators.count(opCallKind) &&
				commutativeOperators.count(opCallKind))
		{
			if (ExpressionBase *newResultExpression = private_SimplifyBinaryAssociativeAndCommutativeOperation(resultExpression))
			{
				delete resultExpression;
				return newResultExpression;
			}
			return resultExpression;			
		}					
	}
	return resultExpression;
}

ExpressionBase* Simplifier::simplify(const TypeCastExpression* const castExpression)
{
	ExpressionBase *expr = simplify(&castExpression->getCastArgument());

	const BasicType *castType = dynamic_cast<const BasicType*>(&castExpression->getCastType());	
	TypeCastExpression *resultExpression = new TypeCastExpression(BasicType::basicType(castType->getKind()),
		expr, castExpression->isImplicit());
	
	if (ExpressionBase *newExpression = TypeCastOperator().calculate(resultExpression))
	{
		delete resultExpression;
		return newExpression;
	}
	return resultExpression;
}

void Simplifier::simplifyAndReplace(OPS::Reprise::ExpressionStatement& exprStatement)
{
	exprStatement.set( simplify(&exprStatement.get()) );
}

}
}
