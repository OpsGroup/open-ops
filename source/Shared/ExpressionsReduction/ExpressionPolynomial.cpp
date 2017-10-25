#include "Shared/ExpressionPolynomial.h"
#include "Shared/DataShared.h"
#include "../DataShared/DataSharedDeepWalkers.h"

#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS::Reprise;
using namespace OPS::Shared::Literals;
using namespace std;

namespace OPS
{
namespace Shared
{

class PriorityMonomialConstructor: public OPS::Reprise::Service::WalkerBase
{
public:
    PriorityMonomialConstructor(): OPS::Reprise::Service::WalkerBase() {}

    ~PriorityMonomialConstructor() { m_ResultStack.clear();	}

    void visit(Reprise::BasicCallExpression& Node);
    void visit(Reprise::ReferenceExpression& Node);
	void visit(Reprise::StructAccessExpression& Node);
    void visit(Reprise::StrictLiteralExpression& Node);
    void visit(Reprise::EnumAccessExpression& Node);

    PriorityMonomial* getPriorityMonomial()
    {
        return m_ResultStack.front().release();
    }

private:
    std::vector<Reprise::ReprisePtr<PriorityMonomial> > m_ResultStack;
};

// ExpressionOperators.h implementation
EN_Operator_Precedence_Level getOperatorPrecedency(BasicCallExpression::BuiltinCallKind OperatorKind)
{
	switch (OperatorKind)
	{
	case BasicCallExpression::BCK_ARRAY_ACCESS:		return OPL_POSTFIX;
	case BasicCallExpression::BCK_ASSIGN:			return OPL_OUT_OF_CONSIDERATION;
	case BasicCallExpression::BCK_BINARY_MINUS:		return OPL_ADDITIVE;
	case BasicCallExpression::BCK_BINARY_PLUS:		return OPL_ADDITIVE;
	case BasicCallExpression::BCK_BITWISE_AND:		return OPL_BITWISE_AND;
	case BasicCallExpression::BCK_BITWISE_NOT:		return OPL_UNARY;
	case BasicCallExpression::BCK_BITWISE_OR:		return OPL_BITWISE_OR;
	case BasicCallExpression::BCK_BITWISE_XOR:		return OPL_BITWISE_XOR;
	case BasicCallExpression::BCK_COMMA:			return OPL_COMMA;
	case BasicCallExpression::BCK_CONDITIONAL:		return OPL_CONDITIONAL;
	case BasicCallExpression::BCK_DE_REFERENCE:		return OPL_PREFIX;
	case BasicCallExpression::BCK_DIVISION:			return OPL_MULTIPLICATIVE;
	case BasicCallExpression::BCK_EQUAL:			return OPL_EQUALITY;
	case BasicCallExpression::BCK_GREATER:			return OPL_RELATIONAL;
	case BasicCallExpression::BCK_GREATER_EQUAL:	return OPL_RELATIONAL;
	case BasicCallExpression::BCK_INTEGER_DIVISION:	return OPL_MULTIPLICATIVE;
	case BasicCallExpression::BCK_INTEGER_MOD:		return OPL_MULTIPLICATIVE;
	case BasicCallExpression::BCK_LEFT_SHIFT:		return OPL_SHIFTS;
	case BasicCallExpression::BCK_LESS:				return OPL_RELATIONAL;
	case BasicCallExpression::BCK_LESS_EQUAL:		return OPL_RELATIONAL;
	case BasicCallExpression::BCK_LOGICAL_AND:		return OPL_AND;
	case BasicCallExpression::BCK_LOGICAL_NOT:		return OPL_UNARY;
	case BasicCallExpression::BCK_LOGICAL_OR:		return OPL_OR;
	case BasicCallExpression::BCK_MULTIPLY:			return OPL_MULTIPLICATIVE;
	case BasicCallExpression::BCK_NOT_EQUAL:		return OPL_EQUALITY;
	case BasicCallExpression::BCK_RIGHT_SHIFT:		return OPL_SHIFTS;
	case BasicCallExpression::BCK_SIZE_OF:			return OPL_PREFIX;
	case BasicCallExpression::BCK_TAKE_ADDRESS:		return OPL_PREFIX;
	case BasicCallExpression::BCK_UNARY_MINUS:		return OPL_UNARY;
	case BasicCallExpression::BCK_UNARY_PLUS:		return OPL_UNARY;
	
	default: 
		{
			throw RepriseError("PriorityMonomial.getOperatorPrecedency: Undefined BasicCallExpression kind!");
			//return OPL_UNDEFINED;
		}
	}
}

bool isPrecedent(BasicCallExpression::BuiltinCallKind OperatorKind1, 
								   BasicCallExpression::BuiltinCallKind OperatorKind2)
{
	EN_Operator_Precedence_Level p1 = getOperatorPrecedency(OperatorKind1);
	EN_Operator_Precedence_Level p2 = getOperatorPrecedency(OperatorKind2);
	if ((p1 == OPL_PREFIX) && (p2 == OPL_UNARY))
		return false;
	else
		return (p1 < p2);
}


bool allowDistributiveLaw(BasicCallExpression::BuiltinCallKind OuterOperatorKind, 
								BasicCallExpression::BuiltinCallKind InnerOperatorKind)
{
	switch (OuterOperatorKind) 
	{
	case BasicCallExpression::BCK_MULTIPLY:
		switch (InnerOperatorKind)
		{
		case BasicCallExpression::BCK_BINARY_PLUS:
		case BasicCallExpression::BCK_BINARY_MINUS:
		case BasicCallExpression::BCK_LOGICAL_OR:
			return true;
		default:
			return false;
		}
	case BasicCallExpression::BCK_BITWISE_AND:
		if ((InnerOperatorKind == BasicCallExpression::BCK_BITWISE_OR) ||
			(InnerOperatorKind == BasicCallExpression::BCK_LOGICAL_OR))
			return true;
		else 
			return false;
	case BasicCallExpression::BCK_LOGICAL_AND:
		if (InnerOperatorKind == BasicCallExpression::BCK_LOGICAL_OR)
			return true;
		else 
			return false;
	default:
		return false;
	}
}

unsigned getOperatorArity(BasicCallExpression::BuiltinCallKind OperatorKind)
{
	EN_Operator_Precedence_Level lvl = getOperatorPrecedency(OperatorKind);
	if ((lvl == OPL_UNDEFINED) || (lvl == OPL_OUT_OF_CONSIDERATION))
		return 0;
	if (getOperatorPrecedency(OperatorKind) < OPL_MULTIPLICATIVE)
		return 1;
	if (getOperatorPrecedency(OperatorKind) < OPL_CONDITIONAL)
		return 2;
	if (getOperatorPrecedency(OperatorKind) == OPL_CONDITIONAL)
		return 3;
	if (getOperatorPrecedency(OperatorKind) == OPL_COMMA)
		return 4; // т.е. бесконечность ))

	return 0;
}

bool isOperatorAssociative(BasicCallExpression::BuiltinCallKind OperatorKind)
{
	return ((OperatorKind == BasicCallExpression::BCK_MULTIPLY) 
			|| (OperatorKind == BasicCallExpression::BCK_BINARY_PLUS)
			|| (OperatorKind == BasicCallExpression::BCK_BITWISE_AND)
			|| (OperatorKind == BasicCallExpression::BCK_BITWISE_OR)
			|| (OperatorKind == BasicCallExpression::BCK_BITWISE_XOR)
			|| (OperatorKind == BasicCallExpression::BCK_LOGICAL_AND)
			|| (OperatorKind == BasicCallExpression::BCK_LOGICAL_OR));
}

// SymbolicDescription class implementation
SymbolicDescription* SymbolicDescription::createSymbolicDescription(Reprise::ExpressionBase& Prototype)
{
	SymbolicDescription* res = 0;
	if (Prototype.is_a<ReferenceExpression>())
		res = new SymbolicScalar(&Prototype.cast_to<ReferenceExpression>().getReference());
	
	if (Prototype.is_a<BasicCallExpression>())
	{
		BasicCallExpression& bckPrototype = Prototype.cast_to<BasicCallExpression>();
		switch(bckPrototype.getKind())
		{
		case BasicCallExpression::BCK_ARRAY_ACCESS:
			{
				SymbolicDescription* head = createSymbolicDescription(bckPrototype.getArgument(0));
				PriorityMonomial* tail = PriorityMonomial::createPriorityMonomial(bckPrototype.getArgument(1));
				if ((head) && (tail))
					res = new SymbolicArray(head, tail);
				break;
			}
		case BasicCallExpression::BCK_TAKE_ADDRESS:
		case BasicCallExpression::BCK_DE_REFERENCE:
			{
				SymbolicDescription* tail = createSymbolicDescription(bckPrototype.getArgument(0));
				if (tail)
					res = new SymbolicUnaryOperator(bckPrototype.getKind(), tail);
				break;
			}
		default:
			res = 0;
		}
	}

	if (Prototype.is_a<StructAccessExpression>())
	{
		StructAccessExpression& saePrototype = Prototype.cast_to<StructAccessExpression>();
		SymbolicDescription* head = createSymbolicDescription(saePrototype.getStructPointerExpression());
		if (head)
			res = new SymbolicStruct(head, &saePrototype.getMember());
	}
	
	return res;
}

// SymbolicScalar class implementation
bool SymbolicScalar::isEqual(const MonomialOperand* SecondArg) const
{
	if (!SecondArg->is_a<SymbolicScalar>())
		return false;

	return (SecondArg->cast_to<SymbolicScalar>().m_DeclarationName == m_DeclarationName);
}

Reprise::ExpressionBase* SymbolicScalar::convert2RepriseExpression() const
{
	return new ReferenceExpression(*m_DeclarationName);
}

// SymbolicArray class implementation
bool SymbolicArray::isEqual(const MonomialOperand* SecondArg) const
{
	if (!SecondArg->is_a<SymbolicArray>())
		return false;

	const SymbolicArray& secondArg = SecondArg->cast_to<SymbolicArray>();
	return (m_Head->isEqual(secondArg.m_Head.get()) && m_SubExpression->isEqual(secondArg.m_SubExpression.get()));
}

Reprise::ExpressionBase* SymbolicArray::convert2RepriseExpression() const
{
	return new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, 
			m_Head->convert2RepriseExpression(), m_SubExpression->convert2RepriseExpression());
}


// SymbolicStruct class implementation
bool SymbolicStruct::isEqual(const MonomialOperand* SecondArg) const
{
	if (!SecondArg->is_a<SymbolicStruct>())
		return false;

	const SymbolicStruct& secondArg = SecondArg->cast_to<SymbolicStruct>();
	return (m_Head->isEqual(secondArg.m_Head.get()) && (m_SubExpression == secondArg.m_SubExpression));
}

Reprise::ExpressionBase* SymbolicStruct::convert2RepriseExpression() const
{
	return new StructAccessExpression(*m_Head->convert2RepriseExpression(), *m_SubExpression);
}

// SymbolicUnaryOperator class implementation
SymbolicUnaryOperator::SymbolicUnaryOperator(BasicCallExpression::BuiltinCallKind Kind, SymbolicDescription* SubExpression):
		m_Kind(Kind), m_SubExpression(SubExpression) 
{
	if (!isCorrect())
	{
		m_SubExpression.release();
		OPS::getOutputConsole("ExpressionPolynomial").log(OPS::Console::LEVEL_ERROR, "Unexprected operator in SymbolicUnaryOperator constructor.");
	}
}

bool SymbolicUnaryOperator::isEqual(const MonomialOperand* SecondArg) const
{
	if (!SecondArg->is_a<SymbolicUnaryOperator>())
		return false;

	const SymbolicUnaryOperator& secondArg = SecondArg->cast_to<SymbolicUnaryOperator>();
	return (m_Kind == secondArg.m_Kind) && (m_SubExpression->isEqual(secondArg.m_SubExpression.get()));
}

Reprise::ExpressionBase* SymbolicUnaryOperator::convert2RepriseExpression() const
{
	if (isCorrect())
		return new BasicCallExpression(m_Kind, m_SubExpression->convert2RepriseExpression());
	else
		return 0;
}

bool SymbolicUnaryOperator::isCorrect() const
{
	switch(m_Kind)
	{
	case BasicCallExpression::BCK_TAKE_ADDRESS:
	case BasicCallExpression::BCK_DE_REFERENCE:
			return true;
	default:
			return false;
	}
}

// PriorityMonomial implementation
PriorityMonomial::PriorityMonomial(const MonomialOperand& Operand) 
{
	if (Operand.is_a<PriorityMonomial>())
	{
		PriorityMonomial* pm = Operand.cast_ptr<PriorityMonomial>()->clone();
		m_Operands.insert(m_Operands.end(), pm->m_Operands.begin(), pm->m_Operands.end());
		m_Operators.insert(m_Operators.end(), pm->m_Operators.begin(), pm->m_Operators.end());
		m_Coefficient.reset(pm->m_Coefficient->clone());
		delete pm;
	}
	else
	{
		ReprisePtr<MonomialOperand> operand(Operand.clone());
		m_Operands.push_back(operand);
		m_Coefficient = ReprisePtr<StrictLiteralExpression>();
	}
}

PriorityMonomial* PriorityMonomial::createPriorityMonomial(const ExpressionBase& OriginalExpression)
{
	PriorityMonomialConstructor constructor;
	try
	{
        // TODO: remove cloning
        ReprisePtr<ExpressionBase> copy(OriginalExpression.clone());
		copy->accept(constructor);
		return constructor.getPriorityMonomial();
	}
    catch(const RepriseError& err)
	{
		OPS::getOutputConsole("ExpressionPolynomial").log(OPS::Console::LEVEL_ERROR, err.getMessage());
		return 0;
	}
}

bool PriorityMonomial::applyOperator(BasicCallExpression::BuiltinCallKind OperatorKind, 
				const PriorityMonomial* SecondOperand, const PriorityMonomial* ThirdOperand)
{
	// Проверим операцию
	EN_Operator_Precedence_Level OpLevel = getOperatorPrecedency(OperatorKind);
	if (OpLevel == OPL_OUT_OF_CONSIDERATION)
		return false;

	if ((OpLevel == OPL_PREFIX) || (OpLevel == OPL_POSTFIX))
		return false; // TODO: должна быть работа с пре- и постфиксными операциями

	// Проверим аргументы
	if ((OPL_UNARY < OpLevel) && (OpLevel < OPL_CONDITIONAL) && (SecondOperand == 0))
	{
		// передана бинарная операция, но не передан второй аргумент
		throw RepriseError("PriorityMonomial.applyOperator: Not enough arguments!");
	}
	if ((OPL_OR < OpLevel) && (OpLevel < OPL_CONDITIONAL) && 
		((SecondOperand == 0) || (ThirdOperand == 0)))
	{
		// передана тернарная операция, но не передан второй  или третий аргумент 
		throw RepriseError("PriorityMonomial.applyOperator: Not enough arguments!");
	}

	switch (getOperatorArity(OperatorKind))
	{
	case 1: 
		{
			// Операция точно унарная, т.к. пре- и постфиксные были проанализированы выше.
			applyUnaryOperator(OperatorKind); 
			return true;
		}
	case 2:
		{
			// 1. Если оба аргумента не PriorityMonomial, т.е. константы/переменные
			if ((getPrecedencyLevel() == OPL_UNDEFINED) && (SecondOperand->getPrecedencyLevel() == OPL_UNDEFINED))
			{
				// 1.1 
				m_Operators.push_back(OperatorKind);
				if (SecondOperand->m_Operands.size() != 1)
					throw RepriseError("PriorityMonomial.applyOperator: Wrong number of operands!");
				m_Operands.push_back(SecondOperand->m_Operands.front());
				return true;
			}

			// 2. Пытаемся применить дистрибутивности
			// Проверяем возможность применить дистрибутивность справа
			if (getPrecedencyLevel() > OpLevel)
			{
				// очевидно, getPrecedencyLevel() != OPL_UNDEFINED), 
				// значит можно сущ-вует m_Operators.front()
				if (areOperatorsTheSame()) 
					if (allowDistributiveLaw(OperatorKind, m_Operators.front()))
					{
						distributeRight(OperatorKind, SecondOperand);
						return true; 
					}
			}

			// Проверяем возможность применить дистрибутивность слева
			if (SecondOperand->getPrecedencyLevel() > OpLevel)
			{
				// очевидно, (SecondOperand->getPrecedencyLevel() != OPL_UNDEFINED), 
				// значит можно сущ-вует SecondOperand->m_Operators.front()
				if (SecondOperand->areOperatorsTheSame()) 
					if (allowDistributiveLaw(OperatorKind, SecondOperand->m_Operators.front()))
					{
						distributeLeft(OperatorKind, SecondOperand);
						return true; // надо рассмотреть 
					}
			}

			// 3. Если первый аргумент, не одночлен, а константа или переменная,
			// соответственно второй - одночлен, иначе отработал бы п1.
			if (getPrecedencyLevel() == OPL_UNDEFINED)
			{
				// Решаем вопрос о константе
				if ((m_Operands.front()->is_a<MonomialConstant>()) 
					&& isOperatorAssociative(OperatorKind))
				{
					StrictLiteralExpression* _const = m_Operands.front().release()->cast_to<MonomialConstant>().getStrictLiteralClone();
					m_Operands.erase(m_Operands.begin());
					m_Coefficient.reset(_const);
				}

				// 3.1 Действуем без дистрибутивности
				// Если совпадают приоритеты операций ...
				if ((SecondOperand->getPrecedencyLevel() == OpLevel) 
					&& isOperatorAssociative(OperatorKind))
				{
					// Константа
					if (SecondOperand->m_Coefficient.get() != 0)
					{
						if (m_Coefficient.get() != 0) 
						{
							BasicCallExpression exprToCalc(OperatorKind, m_Coefficient.release(),
								SecondOperand->m_Coefficient->clone());
                            CalculatorDeepWalker* calc = new CalculatorDeepWalker();
                            calc->visit(exprToCalc);
                            m_Coefficient.reset(calc->getResult());
						}
						else
							m_Coefficient.reset(SecondOperand->m_Coefficient->clone());
					}

					// Добавляем операцию
					m_Operators.push_back(OperatorKind);

					// Копируем операции из второго аргумента
					m_Operators.insert(m_Operators.end(), SecondOperand->m_Operators.begin(), 
						SecondOperand->m_Operators.end());
					// Копируем операнды из второго аргумента
					m_Operands.insert(m_Operands.end(), SecondOperand->m_Operands.begin(), 
						SecondOperand->m_Operands.end());
					return true;
				}

				// 3.2 Никакие упрощения не сработали
				// Добавляем операцию
				m_Operators.push_back(OperatorKind);
				// Добавляем аргумент как операнд
				ReprisePtr<MonomialOperand> newOperand(SecondOperand->clone());
				m_Operands.push_back(newOperand);
				
				return true;
			}
			
			// 4. Первый аргумент PriorityMonomial, проверяем второй
			if (SecondOperand->getPrecedencyLevel() == OPL_UNDEFINED)
			{
				// 4.1 Если совпадают приоритеты операций ...
				if ((getPrecedencyLevel() == OpLevel) 
					&& isOperatorAssociative(OperatorKind))
				{
					if (SecondOperand->m_Operands.front()->is_a<MonomialConstant>())
					{
						StrictLiteralExpression* _const = SecondOperand->m_Operands.front()->cast_to<MonomialConstant>().getStrictLiteralClone();
						// Это константа
						if (m_Coefficient.get() != 0) 
						{
							BasicCallExpression exprToCalc(OperatorKind, m_Coefficient.release(), _const);
							CalculatorDeepWalker* calc = new CalculatorDeepWalker();
							calc->visit(exprToCalc);
							m_Coefficient.reset(calc->getResult()); 
						}
						else
						{
							m_Operators.insert(m_Operators.begin(), OperatorKind);
							m_Coefficient.reset(_const);
						}
					}
					else
					{
						m_Operators.push_back(OperatorKind);
						m_Operands.push_back(SecondOperand->m_Operands.front());
					}
					return true;
				}
				// Если к константе/переменной упрощения не применимы, то идем дальше ...
			}

			// 5. Если приоритет операций первого аргумента (точно одночлен!) и
			// операции OperartorKind совпали
			if (getPrecedencyLevel() == OpLevel)
			{
				// Проверяем совпадение операций со вторым одночленом для возможности "добавления операнда"
				if ((SecondOperand->getPrecedencyLevel() == OpLevel) 
					&& isOperatorAssociative(OperatorKind))
				{
					// Преобразуем константы
					if (SecondOperand->m_Coefficient.get() != 0)
					{
						if (m_Coefficient.get() != 0) 
						{
							BasicCallExpression exprToCalc(OperatorKind, m_Coefficient.release(),
								SecondOperand->m_Coefficient->clone());
							CalculatorDeepWalker* calc = new CalculatorDeepWalker();
							calc->visit(exprToCalc);
							m_Coefficient.reset(calc->getResult()); 
						}
						else
						{
							m_Operators.insert(m_Operators.begin(), OperatorKind);
							m_Coefficient.reset(SecondOperand->m_Coefficient->clone());
						}
					}

					// Копируем операции
					m_Operators.insert(m_Operators.end(), SecondOperand->m_Operators.begin(), 
						SecondOperand->m_Operators.end());
					// Копируем аргументы
					m_Operands.insert(m_Operands.end(), SecondOperand->m_Operands.begin(), 
						SecondOperand->m_Operands.end());
				}
				else
				{
					// Добавляем операцию
					m_Operators.push_back(OperatorKind);
					// Добавляем операнд
					if (SecondOperand->getPrecedencyLevel() == OPL_UNDEFINED)
					{
						ReprisePtr<MonomialOperand> newOperand(SecondOperand->m_Operands.front()->clone());
						m_Operands.push_back(newOperand);
					}
					else
					{
						ReprisePtr<MonomialOperand> newOperand(SecondOperand->clone());
						m_Operands.push_back(newOperand);
					}
				}
				return true;
			}

			// 5. Если приоритет операций второго аргумента (возможно константа/переменная!) и
			// операции OperartorKind совпали
			ReprisePtr<PriorityMonomial> oldThis (this->clone());
			clear();
			if ((SecondOperand->getPrecedencyLevel() == OpLevel) 
				&& (getPrecedencyLevel() == OPL_MULTIPLICATIVE)
				&& (getPrecedencyLevel() == OPL_ADDITIVE))
			{
				// очевидно, (SecondOperand->getPrecedencyLevel() != OPL_UNDEFINED), значит
				// здесь оба аргумента одночлены

				// Первая операция
				m_Operators.push_back(OperatorKind);
				// Копируем операции
				m_Operators.insert(m_Operators.end(), SecondOperand->m_Operators.begin(), 
					SecondOperand->m_Operators.end());

				// Первый аргумент
				m_Operands.push_back(oldThis);
				// Копируем аргументы
				m_Operands.insert(m_Operands.end(), SecondOperand->m_Operands.begin(), 
					SecondOperand->m_Operands.end());

				// Копируем константу
				m_Coefficient.reset(SecondOperand->m_Coefficient->clone()); 
				return true;
			}

			// 7. Никакие упрощения не применимы
			// Добавляем операцию
			m_Operators.push_back(OperatorKind);
			// Добавляем операнды
			m_Operands.push_back(oldThis);
			ReprisePtr<MonomialOperand> newOperand(SecondOperand->clone());
			m_Operands.push_back(newOperand);

			return true;
		}
	case 3:
		{
			// Добавляем операцию
			m_Operators.push_back(OperatorKind);
			// Добавляем операнд
			ReprisePtr<MonomialOperand> newOperand(SecondOperand->clone());
			m_Operands.push_back(newOperand);
			newOperand.reset(ThirdOperand->clone());
			m_Operands.push_back(newOperand);
			return true;
		}
	case 0:
	case 4:
	default:
		throw RepriseError("PriorityMonomial.applyOperator: Operator is not supported!");
	}
}

EN_Operator_Precedence_Level PriorityMonomial::getPrecedencyLevel() const
{
	if (m_Operators.size() > 0)
		return getOperatorPrecedency(m_Operators.front());
	else
		return OPL_UNDEFINED; // нет операций
}

bool PriorityMonomial::areOperatorsTheSame() const
{
	if (OPL_EQUALITY < getPrecedencyLevel())
		// операции на последних уровнях иерархии приоритета единственны на своем уровне
		return true; 

	if (m_Operators.size() > 0)
	{
		vector<BasicCallExpression::BuiltinCallKind>::const_iterator it = m_Operators.begin();
		for (++it; it!=m_Operators.end(); ++it)
			if (*it != m_Operators.front())
				return false;
		// расхождения не найдены
		return true;
	}
	else
		return true;
}

bool PriorityMonomial::isEqual(const MonomialOperand* SecondArg) const
{
	if (!SecondArg->is_a<PriorityMonomial>())
		return false;
	
    const PriorityMonomial* pmSecondArg = SecondArg->cast_ptr<PriorityMonomial>();
	EN_Operator_Precedence_Level thisPrecedencyLevel = getPrecedencyLevel();
	if (thisPrecedencyLevel != pmSecondArg->getPrecedencyLevel())
		return false;

	if (thisPrecedencyLevel == OPL_OUT_OF_CONSIDERATION)
		return false;

	if ((areOperatorsTheSame()) && (thisPrecedencyLevel != OPL_UNDEFINED))
	{
		if ((m_Operators.front() == BasicCallExpression::BCK_MULTIPLY) ||
			(m_Operators.front() == BasicCallExpression::BCK_BINARY_PLUS) ||
			(m_Operators.front() == BasicCallExpression::BCK_BITWISE_AND) ||
			(m_Operators.front() == BasicCallExpression::BCK_BITWISE_OR) ||
			(m_Operators.front() == BasicCallExpression::BCK_BITWISE_XOR) ||
			(m_Operators.front() == BasicCallExpression::BCK_LOGICAL_AND) ||
			(m_Operators.front() == BasicCallExpression::BCK_LOGICAL_OR))
		{
			if (m_Operands.size() != pmSecondArg->m_Operands.size())
				return false;

			int size = m_Operands.size();
            vector<bool> hasMatch(size, false);
			vector<ReprisePtr<MonomialOperand> >::const_iterator it = m_Operands.begin();
			for(; it != m_Operands.end(); ++it)
			{
				bool MatchFound = false;
				for(int i=0; i<size; ++i)
					if (!hasMatch[i])
						if ((*it)->isEqual(m_Operands[i].get()))
						{
							hasMatch[i] = MatchFound = true;
							break;
						}
				if (!MatchFound)
					return false;
			}
			return true; // соответствия найдены для всех операндов
		}
	}

	// Далее операции и операнды имеет смысл проверять только 
	// в порядке определенном векторами m_Operators и m_Operands
	if (m_Operators.size() != pmSecondArg->m_Operators.size())
		return false;

	if (m_Operands.size() != pmSecondArg->m_Operands.size())
		return false;
	
    for(size_t i = 0; i<m_Operators.size(); ++i)
		if (m_Operators[i] != pmSecondArg->m_Operators[i])
			return false;

    for(size_t i = 0; i<m_Operands.size(); ++i)
		if (! m_Operands[i]->isEqual(pmSecondArg->m_Operands[i].get()))
			return false;
	return true;
}

ExpressionBase* PriorityMonomial::convert2RepriseExpression() const 
{
	if (m_Operands.empty())
		throw RepriseError("PriorityMonomial was created incorrectly!");

	if (m_Operators.empty())
		return m_Operands.front()->convert2RepriseExpression();

	vector<BasicCallExpression::BuiltinCallKind>::const_iterator itOperators = m_Operators.begin();
	vector<ReprisePtr<MonomialOperand> >::const_iterator  itOperands = m_Operands.begin();
	
	// Создаем первую операцию
	BasicCallExpression* result = new BasicCallExpression(m_Operators.front());
	itOperators++;

	switch (getOperatorArity(result->getKind()))
	{
	case 1:
		{
			if (m_Operands.size() != 1)
				throw RepriseError("PriorityMonomial was created incorrectly!");
			result->addArgument(m_Operands.front()->convert2RepriseExpression());
			return result;
		}
	case 2:
		{
			// Добавляем первой операции первый операнд 
			if (m_Coefficient.get() != 0)
				result->addArgument(m_Coefficient->clone());
			else
			{
				result->addArgument(m_Operands.front()->convert2RepriseExpression());
				itOperands++;
			}

			// Проверяем наличие второго операнда
			if (itOperands == m_Operands.end())
				throw RepriseError("PriorityMonomial was created incorrectly!");

			// Добавляем второй операнд
			result->addArgument((*itOperands)->convert2RepriseExpression());
			itOperands++;

			// Добавляем остальные операции и операнды
			BasicCallExpression* argument0 = result;
			for( ; itOperators != m_Operators.end(); ++itOperators, ++itOperands)
			{
				if (itOperands == m_Operands.end())
					throw RepriseError("PriorityMonomial was created incorrectly!");
				
				result = new BasicCallExpression(*itOperators);
				result->addArgument(argument0);
				result->addArgument((*itOperands)->convert2RepriseExpression());
				argument0 = result;
			}
			return result;
		}
	case 3:
		{
			if (m_Operands.size() != 3)
				throw RepriseError("PriorityMonomial was created incorrectly!");
			result->addArgument(m_Operands[0]->convert2RepriseExpression());
			result->addArgument(m_Operands[1]->convert2RepriseExpression());
			result->addArgument(m_Operands[2]->convert2RepriseExpression());
			return result;
		}
	case 0:
	case 4:
	default:
		throw RepriseError("PriorityMonomial was created incorrectly!");
	}
	return 0; 
}

void PriorityMonomial::applyUnaryOperator(BasicCallExpression::BuiltinCallKind OperatorKind)
{
	if (getOperatorArity(OperatorKind) != 1)
		return;

	if (getPrecedencyLevel() == OPL_UNDEFINED) // Константа или переменная?
	{
		m_Operators.push_back(OperatorKind);
		return;
	}

	switch (OperatorKind)
	{
	case BasicCallExpression::BCK_UNARY_PLUS: break;
	case BasicCallExpression::BCK_UNARY_MINUS:
		{
			switch (getPrecedencyLevel())
			{
			case OPL_UNARY:
				{
					if (m_Operators.front() == BasicCallExpression::BCK_UNARY_MINUS)
					{	// Закон двойного отрицания, причем аргумент не переменная, а именно другой одночлен,
						// т.к. все унарные операции должны быть протянуты вовнутрь
						applyDoubleNegationLaw();
						return;
					}
				}
			case OPL_ADDITIVE:
				{
					vector<Reprise::ReprisePtr<MonomialOperand> >::iterator it = m_Operands.begin();
					for(; it != m_Operands.end(); ++it)
						it->get()->applyUnaryOperator(OperatorKind);
					return;
				}
			case OPL_MULTIPLICATIVE:
			case OPL_SHIFTS:
				{
					m_Operands.front()->applyUnaryOperator(OperatorKind);
					return;
				}
			default: break;
			//default do nothing
			}
			break;
		}
	case BasicCallExpression::BCK_BITWISE_NOT:
		{
			if (m_Operators.front() == BasicCallExpression::BCK_BITWISE_NOT)
			{	// Закон двойного отрицания, причем аргумент не переменная, а именно другой одночлен,
				// т.к. все унарные операции должны быть протянуты вовнутрь
				applyDoubleNegationLaw();
				return;
			}
			if ((m_Operators.front() == BasicCallExpression::BCK_BITWISE_AND) || (m_Operators.front() == BasicCallExpression::BCK_BITWISE_OR))
			{
				applyDeMorgansLaw(OperatorKind);
				return;
			}
			break;
		}
	case BasicCallExpression::BCK_LOGICAL_NOT:
		{
			if (m_Operators.front() == BasicCallExpression::BCK_LOGICAL_NOT)
			{	// Закон двойного отрицания, причем аргумент не переменная, а именно другой одночлен,
				// т.к. все унарные операции должны быть протянуты вовнутрь
				applyDoubleNegationLaw();
				return;
			}
			if ((m_Operators.front() == BasicCallExpression::BCK_LOGICAL_AND) || (m_Operators.front() == BasicCallExpression::BCK_LOGICAL_OR))
			{
				applyDeMorgansLaw(OperatorKind);
				return;
			}
			break;
		}
	default:
		throw RepriseError("PriorityMonomial.applyUnaryOperator: Unexpected unary operator!");
	}
	
	// Ни один из законов оказался не применим. Перестраиваем одночлен.
	PriorityMonomial* oldThis = this->clone();
	ReprisePtr<PriorityMonomial> newOperand(oldThis);
	
	// Преобразуем операции
	m_Operators.clear();
	m_Operators.push_back(OperatorKind);

	// Преобразуем операнды
	m_Operands.clear();
	m_Operands.push_back(newOperand);

}

void PriorityMonomial::distributeRight(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind, const PriorityMonomial* RightOperand)
{
	if (m_Operators.size() == 0)
		return; // нет операций => нет дистрибутивности

	// Преобразуем операнды, а операции не меняем
	vector<ReprisePtr<MonomialOperand> >::iterator it = m_Operands.begin();
	for (; it != m_Operands.end(); ++it)
	{
		if ((*it)->is_a<SymbolicDescription>())
		{
			// Создаем одночлен состоящий только из этой переменной
			PriorityMonomial* temp = new PriorityMonomial((*it)->cast_to<SymbolicDescription>());

			// Применяем операцию
			temp->applyOperator(OperatorKind, RightOperand);

			// Заменяем операнд в текущем многочлене на новый
			it->reset(temp);
		}
		
		if ((*it)->is_a<PriorityMonomial>())
		{
			// Применяем операцию 
			(*it)->cast_to<PriorityMonomial>().applyOperator(OperatorKind, RightOperand);
		}
		else
			throw RepriseError("PriorityMonomial.distributeRight: Unexpected MonomialOperand!");
	}
}

void PriorityMonomial::distributeLeft(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind, const PriorityMonomial* RightOperand)
{
	if (RightOperand->m_Operators.size() == 0)
		return; // нет операций => нет дистрибутивности

	// Преобразуем операнды
	PriorityMonomial* oldThis = this->clone();
	clear();

	vector<ReprisePtr<MonomialOperand> >::const_iterator it = RightOperand->m_Operands.begin();
	vector<BasicCallExpression::BuiltinCallKind>::const_iterator it_operators = RightOperand->m_Operators.begin();
	for (; it != RightOperand->m_Operands.end(); ++it)
	{
		bool ok = false;

		// Создаем копию первого (левого) аргумента
		PriorityMonomial* arg1 = oldThis->clone();

		if ((*it)->is_a<SymbolicDescription>())
		{
			ok = true;
			// Создаем одночлен состоящий только из этой переменной
			PriorityMonomial* arg2 = new PriorityMonomial((*it)->cast_to<SymbolicDescription>());

			// Применяем операцию 
			arg1->applyOperator(OperatorKind, arg2);
		}
		
		if ((*it)->is_a<PriorityMonomial>())
		{
			ok = true;
			// Применяем операцию 
			arg1->applyOperator(OperatorKind, (*it)->cast_ptr<PriorityMonomial>());
		}

		if (ok)
		{
			// Вносим результат в текущий одночлен
			ReprisePtr<MonomialOperand> operand(arg1);
			m_Operands.push_back(operand);

			// Преобразуем операции
			if (it_operators != RightOperand->m_Operators.end())
			{
				m_Operators.push_back(*it_operators);
				it_operators++;
			}

		}
		else
			throw RepriseError("PriorityMonomial.distributeLeft: Unexpected MonomialOperand!");
	}
	delete oldThis;
}

void PriorityMonomial::applyDoubleNegationLaw()
{
	if (m_Operands.front().get()->is_a<PriorityMonomial>())
	{
		PriorityMonomial* temp = m_Operands.front().get()->cast_ptr<PriorityMonomial>();

		// Копируем операции
		m_Operators.clear();
		m_Operators.insert(m_Operators.begin(), temp->m_Operators.begin(), temp->m_Operators.end());

		// Копируем операнды
		m_Operands.insert(m_Operands.begin()+1, temp->m_Operands.begin(), temp->m_Operands.end());

		// Удаляем тот первый аргумент, который относился к старой унарной операции
		m_Operands.erase(m_Operands.begin());
	}
	else
		if (m_Operands.front()->is_a<SymbolicDescription>())
			m_Operators.clear();
		else
			throw RepriseError("PriorityMonomial.applyUnaryOperator: Unexpected MonomialOperand!"); 

}

void PriorityMonomial::applyDeMorgansLaw(BasicCallExpression::BuiltinCallKind NegationOperator)
{
	if (m_Operators.size() == 0)
		return;

	BasicCallExpression::BuiltinCallKind oppositeOperator;
	switch (m_Operators.front())
	{
	case BasicCallExpression::BCK_BITWISE_XOR:
		if (NegationOperator != BasicCallExpression::BCK_BITWISE_NOT) 
			return;
		//TODO: ~(a^b)=(a&b)|(~a & ~b)
		return;
	case BasicCallExpression::BCK_BITWISE_AND:	
		if (NegationOperator != BasicCallExpression::BCK_BITWISE_NOT) 
			return;
		oppositeOperator = BasicCallExpression::BCK_BITWISE_OR;
		break;
	case BasicCallExpression::BCK_BITWISE_OR:
		if (NegationOperator != BasicCallExpression::BCK_BITWISE_NOT) 
			return;
		oppositeOperator = BasicCallExpression::BCK_BITWISE_AND;
		break;
	case BasicCallExpression::BCK_LOGICAL_OR:
		if (NegationOperator != BasicCallExpression::BCK_LOGICAL_NOT) 
			return;
		oppositeOperator = BasicCallExpression::BCK_LOGICAL_AND;
		break;
	case BasicCallExpression::BCK_LOGICAL_AND:
		if (NegationOperator != BasicCallExpression::BCK_LOGICAL_NOT) 
			return;
		oppositeOperator = BasicCallExpression::BCK_LOGICAL_OR;
		break;
	default:
		return;
	}

	// Преобразуем операции
	vector<BasicCallExpression::BuiltinCallKind>::iterator it1 = m_Operators.begin();
	for(; it1 != m_Operators.end(); ++it1)
		*it1 = oppositeOperator;

	// Преобразуем операнды
	vector<ReprisePtr<MonomialOperand> >::iterator it2 = m_Operands.begin();
	for(; it2 != m_Operands.end(); ++it2)
		it2->get()->applyUnaryOperator(NegationOperator);
}

// MonomialConstant implementation
void MonomialConstant::applyUnaryOperator(BasicCallExpression::BuiltinCallKind OperatorKind)
{
	BasicCallExpression* exprToCalc = new BasicCallExpression(OperatorKind, this->m_Constant.get());
	CalculatorDeepWalker calc;
	calc.visit(*exprToCalc);
	m_Constant.reset(calc.getResult());
	delete exprToCalc;
}

bool MonomialConstant::isEqual(const MonomialOperand* SecondArg) const
{
	if (!SecondArg->is_a<MonomialConstant>())
		return false;

	return m_Constant->isEqual(*SecondArg->cast_to<MonomialConstant>().m_Constant.get());
}

// PriorityMonomialConstructor class implementation
void PriorityMonomialConstructor::visit(BasicCallExpression& Node)
{
	switch(getOperatorArity(Node.getKind()))
	{
	case 1:
		{
			if (Node.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
			{
				ReprisePtr<SymbolicDescription> p(SymbolicDescription::createSymbolicDescription(Node));
				if (!p.get())
					throw RepriseError("PriorityMonomialConstructor: Unexpected BasicCallExpression: "+Node.dumpState());
				PriorityMonomial* pm = new PriorityMonomial(*p);
				m_ResultStack.push_back(ReprisePtr<PriorityMonomial>(pm));
			}
			else
			{
				Node.getArgument(0).accept(*this);
				if (m_ResultStack.empty())
					throw RepriseError("PriorityMonomialConstructor: not enough operands for operator!");

				m_ResultStack.back()->applyUnaryOperator(Node.getKind());
			}
			return;
		}
	case 2:
		{
			Node.getArgument(0).accept(*this);
			Node.getArgument(1).accept(*this);
			if (m_ResultStack.size() <= 1)
				throw RepriseError("PriorityMonomialConstructor: not enough operands for operator!");
            ReprisePtr<PriorityMonomial> arg2 = m_ResultStack.back();
			m_ResultStack.pop_back();
            m_ResultStack.back()->applyOperator(Node.getKind(), arg2.get());
			return;
		}
	case 3:
		{
			Node.getArgument(0).accept(*this);
			Node.getArgument(1).accept(*this);
			Node.getArgument(2).accept(*this);
			if (m_ResultStack.size() <= 2)
				throw RepriseError("PriorityMonomialConstructor: not enough operands for operator!");
			PriorityMonomial* arg3 = m_ResultStack.back().release();
			m_ResultStack.pop_back();
			PriorityMonomial* arg2 = m_ResultStack.back().release();
			m_ResultStack.pop_back();
			m_ResultStack.back()->applyOperator(Node.getKind(), arg2, arg3);
			return;
		}
	case 4:
	case 0:
	default:
		throw RepriseError("PriorityMonomialConstructor: Unexpected expression arity obtained for PriorityMonomial construction!");
	}
}

void PriorityMonomialConstructor::visit(ReferenceExpression& Node)
{
    ReprisePtr<SymbolicDescription> p(SymbolicDescription::createSymbolicDescription(Node));
	if (!p.get())
		throw RepriseError("PriorityMonomialConstructor: Unexpected ReferenceExpression: "+Node.dumpState());
    PriorityMonomial* pm = new PriorityMonomial(*p);
	m_ResultStack.push_back(ReprisePtr<PriorityMonomial>(pm));
}

void PriorityMonomialConstructor::visit(Reprise::StructAccessExpression& Node)
{
	ReprisePtr<SymbolicDescription> p(SymbolicDescription::createSymbolicDescription(Node));
	if (!p.get())
		throw RepriseError("PriorityMonomialConstructor: Unexpected StructAccessExpression: "+Node.dumpState());
	PriorityMonomial* pm = new PriorityMonomial(*p);
	m_ResultStack.push_back(ReprisePtr<PriorityMonomial>(pm));
}

void PriorityMonomialConstructor::visit(StrictLiteralExpression& Node)
{
	MonomialConstant* _const = new MonomialConstant(Node);
	PriorityMonomial* pm = new PriorityMonomial(*_const);
	m_ResultStack.push_back(ReprisePtr<PriorityMonomial>(pm));
	delete _const;
}

void PriorityMonomialConstructor::visit(EnumAccessExpression &Node)
{
    ReprisePtr<StrictLiteralExpression> strictLiteral(new StrictLiteralExpression(BasicType::BT_INT32));
    strictLiteral->setInt32(Node.getMember().getValue());
    visit(*strictLiteral);
}

}
}
