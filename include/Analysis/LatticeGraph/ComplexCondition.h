#pragma once
#include "Analysis/LatticeGraph/LinearLib.h"
#include <iostream>

namespace OPS
{
namespace LatticeGraph 
{

/// Сложное условие (логическое выражение) в операторе if дерева решений Фотрье
struct ComplexCondition : public OPS::NonCopyableMix
{
public: 
    //операция текущего узла выражения
    //COMPARE - означает, что текущий узел - неравенство (лист дерева выражения)
    enum Operation
	{
		AND, 
		OR, 
		NOT, 
		COMPARE
	};

	typedef std::vector<std::vector<Inequality> > TDNF;


    ComplexCondition();
    ComplexCondition(Operation op, ComplexCondition* left, ComplexCondition* right = 0);
    ComplexCondition(Inequality* inequality);//неравенство не копируется!!!!
    ~ComplexCondition();//удаляет поддерево с вершиной в данном узле

    ComplexCondition* clone();//копирование

    std::string toString(std::vector<std::string>& paramNames);
    Operation getOperation();
    Inequality* getInequality();
    ComplexCondition* getLeft();
    ComplexCondition* getRight();
    ComplexCondition* getFather();

	/// копирования не происходит!!!!!
    void setLeft(ComplexCondition* left);
	/// копирования не происходит!!!!!
    void setRight(ComplexCondition* right);
    /// копирования не происходит!!!!!
    void setInequality(Inequality* inequality);

    //можно вызывать только после установки детей у father, иначе она выбросит исключение!!!!!
    void setFather(ComplexCondition* father);
    void setOperation(Operation operation);
    bool evaluate(std::vector<int>& argument);//вычисляет выражение
    //Применить трансформационную матрицу ко всем линейным выражениям
    void transform(int** matrix,int dimi,int dimj);
    //строит ДНФ, возвращает вектор коньюнкций(&&)
    TDNF buidDNF();
    void insertEverywhereNZerosBeforeNewParamCoefs(int N, int varNum);//varNum - кол-во обычных переменных (не новых параметров)
    //удаляет все операции отрицания
    void removeNot();

private:

    TDNF buidDNFHelper(); //можно применять только, если нет операций отрицания, пользуйтесь buildDNF()


    Operation m_operation;  
    //если текущий узел - это неравенство, то это коэффициенты линейного выражения неревенства
    Inequality* m_inequality;
    ComplexCondition* m_left ; // левое выражение (или выражение под операцией отрицания) == NULL, только если текущий узел - это неравенство
    ComplexCondition* m_right ; // правое выражение (== NULL для операции отрицания)
    ComplexCondition* m_father ;//родительское выражение (== NULL для предка всех предков)

};

//end of namespace
}
}
