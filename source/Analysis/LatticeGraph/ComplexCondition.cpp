#include <Analysis/LatticeGraph/ComplexCondition.h>

#include "OPS_Core/msc_leakcheck.h"

namespace OPS
{
namespace LatticeGraph 
{

ComplexCondition::ComplexCondition() : m_operation(COMPARE), m_inequality(0), m_left(0), m_right(0), m_father(0)
{
}

ComplexCondition::ComplexCondition(Operation op, ComplexCondition* left, ComplexCondition* right) 
	: m_operation(op), m_inequality(0), m_left(left), m_right(right), m_father(0)
{
    if (op == NOT)    
	{
        if (right != NULL)   
		{
            throw OPS::RuntimeError("You call the constructor of ComplexCondition with operation NOT and TWO(!) arguments");
        }
    }
}

ComplexCondition::ComplexCondition(Inequality* inequality)  
	: m_operation(COMPARE), m_inequality(inequality), m_left(0), m_right(0), m_father(0)
{
}

//удаляет поддерево с вершиной в данном узле
ComplexCondition::~ComplexCondition()
{
    //удаляем рекурсивно
    switch (m_operation) 
	{
	case AND:
	case OR: 
		if (m_left != 0) 
		{
			delete m_left; 
			m_left = 0;
		}
        if (m_right != 0) 
		{
			delete m_right; 
			m_right = 0;
		}
        break;
    case NOT:
        if (m_left != 0) 
		{
			delete m_left; 
			m_left = 0;
		}
        break;
    case COMPARE:
        if (m_inequality != 0) 
		{
			delete m_inequality; 
			m_inequality = 0;
		}
        break;
	OPS_DEFAULT_CASE_LABEL
    }
    m_operation = COMPARE;
}

ComplexCondition* ComplexCondition::clone()
{
    //обходим рекурсивно дерево выражения и снизу вверх (с листев до корня) копируем его
    ComplexCondition* current = new ComplexCondition();
    current->setOperation(m_operation);
    switch (m_operation) 
	{
    case AND:
    case OR: 
		{
			ComplexCondition* left = m_left->clone();  
			ComplexCondition* right = m_right->clone();
			current->setLeft(left);  
			current->setRight(right);  
			left->setFather(current);
			right->setFather(current);
			break;
        }
    case NOT:
		{
			ComplexCondition* left = m_left->clone();  
			current->setLeft(left);  
			left->setFather(current);
			break;
		}
    case COMPARE:
		current->m_inequality = new Inequality(*m_inequality);
        break;
	OPS_DEFAULT_CASE_LABEL
    }
	return current;
}

std::string ComplexCondition::toString(std::vector<std::string>& paramNames)
{
    std::string s;
    switch (m_operation)
    {
    case AND: 
        s="( "+m_left->toString(paramNames)+" ) and ( "+m_right->toString(paramNames)+" )";
        break;
    case OR: 
        s="( "+m_left->toString(paramNames)+" ) or ( "+m_right->toString(paramNames)+" )";   
        break;
    case NOT:
        s="not( "+m_left->toString(paramNames)+" )"; 
        break;
    case COMPARE:  
        s=m_inequality->toString(paramNames)+" >= 0"; 
        break; 
	OPS_DEFAULT_CASE_LABEL
    }
    return s;
}

ComplexCondition::Operation ComplexCondition::getOperation()
{
	return m_operation;
}

Inequality* ComplexCondition::getInequality()
{
	return m_inequality;
}

ComplexCondition* ComplexCondition::getLeft()
{
	return m_left;
}

ComplexCondition* ComplexCondition::getRight()
{
	return m_right;
}

ComplexCondition* ComplexCondition::getFather()
{
	return m_father;
}

void ComplexCondition::setLeft(ComplexCondition* left)
{
    delete m_left;
	m_left = left; 
	left->setFather(this);
}

void ComplexCondition::setRight(ComplexCondition* right)
{
    delete m_right;
	m_right = right; 
	right->setFather(this);
}

void ComplexCondition::setInequality(Inequality* inequality)
{
    delete m_inequality;
    m_inequality = inequality;
}

//можно вызывать только после установки детей у father, иначе она выбросит исключение!!!!!
void ComplexCondition::setFather(ComplexCondition* father)
{
    OPS_ASSERT( ( (father->getLeft()==this)&& (father->getRight()!=this) ) || ( (father->getLeft()!=this)&& (father->getRight()==this) ) );
    if ( ( (father->getLeft()==this)&& (father->getRight()!=this) ) || ( (father->getLeft()!=this)&& (father->getRight()==this) ) )
        m_father=father;
    else
        throw OPS::RuntimeError("ComplexCondition::setFather:  Exactly one child of father must have property m_father initialized by father.");
}

void ComplexCondition::setOperation(Operation operation)
{
	m_operation = operation;
}

bool ComplexCondition::evaluate(std::vector<int>& argument)//вычисляет выражение
{
    bool result = false;
    switch (m_operation)
    {
    case AND: 
        result = m_left->evaluate(argument) && m_right->evaluate(argument);
        break;
    case OR: 
        result = m_left->evaluate(argument) || m_right->evaluate(argument);
        break;
    case NOT:
        result = !(m_left->evaluate(argument)); 
        break;
    case COMPARE:  
        result = (m_inequality->evaluate(argument) >= 0);
        break; 
	OPS_DEFAULT_CASE_LABEL
    }
    return result;
}

//Применить трансформационную матрицу ко всем линейным выражениям
void ComplexCondition::transform(int** matrix, int dimi, int dimj)
{
    switch (m_operation) 
	{
	case AND:
	case OR: 
		m_left->transform(matrix, dimi, dimj);
		m_right->transform(matrix, dimi, dimj);
		break;
    case NOT:
		m_left->transform(matrix, dimi, dimj);
		break;
	case COMPARE:
        m_inequality->transform(matrix, dimi, dimj);
        break;
	OPS_DEFAULT_CASE_LABEL
    }
}

//строит ДНФ, возвращает вектор коньюнкций
ComplexCondition::TDNF ComplexCondition::buidDNF()
{
    //избавляемся от операций отрицания
    ComplexCondition* condWithoutNot = clone();
    condWithoutNot->removeNot();
    const TDNF& result = condWithoutNot->buidDNFHelper();
    delete condWithoutNot;
    return result;
}

//строит ДНФ, возвращает вектор коньюнкций
ComplexCondition::TDNF ComplexCondition::buidDNFHelper()
{
    TDNF result;
    //строим ДНФ, вынося все или наружу
    switch (m_operation) 
	{
    case AND:
        {
            const TDNF& leftDNF = m_left->buidDNF();
            const TDNF& rightDNF = m_left->buidDNF();
            //раскрываем скобки: получается дизъюнкция всевозможных пар конъюнкций: 
            //одна в паре из левой, другая из правой
            result.resize(leftDNF.size() * rightDNF.size());
            for (size_t i = 0; i < leftDNF.size(); i++)
			{
                for (size_t j = 0; j < rightDNF.size(); j++)  
				{
                    std::vector<Inequality> pair(leftDNF[i]);
                    pair.insert(pair.end(), rightDNF[j].begin(), rightDNF[j].end());
                    result[i * leftDNF.size() + j] = pair;
                }
			}
            break;
        }
    case OR: 
        {
            const TDNF& leftDNF = m_left->buidDNF();
            const TDNF& rightDNF = m_left->buidDNF();
            //объединяем их
            result = leftDNF;
            result.insert(result.end(), rightDNF.begin(), rightDNF.end());
            break;
        }
    case COMPARE:
        {
            std::vector<Inequality> one;
            one.resize(1);
            one[0] = *m_inequality;
            result.insert(result.begin(), one);
            break;
        }
	OPS_DEFAULT_CASE_LABEL
    }
    return result;
}

//возвращает равносильное условие без операций отрицания
void ComplexCondition::removeNot()
{
    switch (m_operation) 
	{
    case AND:
    case OR: 
		m_left->removeNot();
		m_right->removeNot();
        break;
    case NOT:
        switch (m_left->m_operation) 
		{
        case AND:
        {
                m_operation = OR;
                ComplexCondition* newLeft = new ComplexCondition(NOT,m_left->m_left);
                ComplexCondition* newRight = new ComplexCondition(NOT,m_left->m_right);
                m_left->m_left = NULL;
                m_left->m_right = NULL;
                delete m_left;
                setLeft(newLeft);
                setRight(newRight);
                break;
        }
        case OR: 
        {
                m_operation = AND;
                ComplexCondition* newLeft = new ComplexCondition(NOT,m_left->m_left);
                ComplexCondition* newRight = new ComplexCondition(NOT,m_left->m_right);
                m_left->m_left = NULL;
                m_left->m_right = NULL;
                delete m_left;
                setLeft(newLeft);
                setRight(newRight);
                break;
        }
        case NOT:
        {
                ComplexCondition* save = m_left->m_left;
                m_left->m_left = NULL;
                delete m_left;
                setLeft(save);
                break;
        }
        case COMPARE:
            m_inequality->makeOposite();
            break;
		OPS_DEFAULT_CASE_LABEL
		}
        break;
    case COMPARE:
        break;
	OPS_DEFAULT_CASE_LABEL
    }
}

void ComplexCondition::insertEverywhereNZerosBeforeNewParamCoefs(int N, int varNum)//varNum - кол-во обычных переменных (не новых параметров)
{
    switch (m_operation) 
	{
    case AND:
    case OR: 
		m_left->insertEverywhereNZerosBeforeNewParamCoefs(N, varNum);
		m_right->insertEverywhereNZerosBeforeNewParamCoefs(N, varNum);
		break;
    case NOT:
        m_left->insertEverywhereNZerosBeforeNewParamCoefs(N, varNum);
        break;
    case COMPARE:
        m_inequality->insertNZerosBeforeNewParamCoefs(N, varNum);
        break;
	OPS_DEFAULT_CASE_LABEL
    }
}

//end of namespace
}
}
