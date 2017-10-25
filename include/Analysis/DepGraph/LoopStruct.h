#ifndef _LOOP_STRUCTURE_DESC_
#define _LOOP_STRUCTURE_DESC_

#ifdef _MSC_VER
#pragma warning(disable : 4786)	// Запрещаем сообщение об обрубании имен в debug-версии
#endif

#include "Analysis/LatticeGraph/LinearLib.h"
#include <Shared/ParametricLinearExpressions.h>


namespace DepGraph
{

struct LoopBounds
{	//класс, описывающий границы одного(!) оператора цикла. Только для циклов с КУСОЧНО-ЛИНЕЙНЫМИ границами
	//левая граница цикла - max от всех выражений массива "m_lower"
	//правая граница - min от всех выражений массива "m_upper", например:
    //for (j = max(i+n+2, 3n-3, 3k+n-1);  j< min(3i+5n-2, 4n+k-1, 2k+7n-i, 100); j++)
	//Первый элемент массива, описывающего линейное выражение SimpleLinearExpression, 
    //содержит числовую константу (либо 0, если ее нет)
	//Далее в этом массиве идут коэфф при счетчиках циклов в порядке вложенности
	//Пример для левых границ.
	//	1 for (i=2+m-n,...
	//	2	for (j=3+n-k ...
	//
	//	Массив m_lower для гнезда 1:
	//	номера элементов массива	0	
	//	переменные					1	(считается, что граница цикла не может содержать счетчик этого цикла => коэфф. при i отсутствует)
	//  элементы массива 			2	
	//  Элементы класса m_lowerExternalParamCoefs для гнезда 1:
    //	переменные					m	|	n	
    //  коэффициенты    			1	|	-1
	//	Массив m_lower для гнезда 2:
	//	номера элементов массива	0	|	1	
	//	переменные					1	|	i	
	//  элементы массива :			3		0	
    //  Элементы класса m_lowerExternalParamCoefs для гнезда 2:
    //	переменные					n	|	k	
    //  коэффициенты    			1	|	-1
	
    //свободный члены и коэффициенты при счетчиках внешних циклов в порядке вложенности (сперва самый внешний)
    OPS::LatticeGraph::SimpleLinearExpression *m_lower, *m_upper;
    
    //VariableDeclaration внешних переменных и коэффициенты при них в линейных выражениях границ циклов
	OPS::Shared::CanonicalLinearExpression m_lowerExternalParamCoefs, m_upperExternalParamCoefs;
	
    int m_lowerNumb,m_upperNumb;//количество линейных выражений под функцией max и под min
	
	LoopBounds()
		:m_lower(NULL)
		,m_upper(NULL)
		,m_lowerNumb(0)
		,m_upperNumb(0)
	{}

	LoopBounds(int _lowerNumb,int _upperNumb);// !! После вызова этой функции НУЖНО ОБЯЗЯТЕЛЬНО инициализировать массивы m_upper, m_lower !!

	LoopBounds(const LoopBounds& lb);
	~LoopBounds(){clear();}
	LoopBounds& operator=(const LoopBounds& lb);
	void clear();
	void print(std::ostream& os,std::string* paramNames,int paramNamesDim,int mode=1,int position=0);

	/// Возвращает свободное выражение, т.е. не зависящее от счетчиков циклов, для нижней границы
	OPS::Shared::CanonicalLinearExpression getLowerFreeExpression() const;
	/// Возвращает свободное выражение, т.е. не зависящее от счетчиков циклов, для верхненей границы
	OPS::Shared::CanonicalLinearExpression getUpperFreeExpression() const;
};


struct LoopBoundsNode//:public LoopBounds
{//Описывает структуру набора гнезд произвольно вложенных циклов
 //pointers - массив указателей на описания заголовков циклов, находящихся в теле рассматриваемого цикла. В ПОРЯДКЕ СЛЕДОВАНИЯ по тексту программы
	LoopBounds* loopBounds;
	int pointerNumb;
	LoopBoundsNode** pointers;

	LoopBoundsNode():loopBounds(NULL),pointerNumb(0),pointers(NULL){}
//	LoopBoundsNode(int m_lowerNumb,int m_upperNumb):LoopBounds(m_lowerNumb,m_upperNumb),pointerNumb(0),pointers(NULL){}
	LoopBoundsNode(LoopBounds& lb,int _pointerNumb);
	LoopBoundsNode(LoopBounds* lb,int _pointerNumb);
	~LoopBoundsNode(){Clear();}
	void Clear();
};

/// Скоструировать выражение-дерево, описывающее левую границу гнезда циклов по структуре LoopBounds.
/**
	По структуре LoopBounds,именам переменных и Namespace, в котором эти переменные видны, а также итераторе на описанную где-то функцию максимума из двух целых чисел построить выражение-дерево, описывающее левую границу цикла.
	Память освобождает тот, кто пользуется результатом.
	Кол-во элементов массива varNames равно m_lower[x].dim-1 (по правилам заполнения структуры LoopBounds).
*/
OPS::Reprise::ExpressionBase* GetLeftExprNode(LoopBounds& lb,OPS::Reprise::Declarations& ns,std::string* varNames,OPS::Reprise::SubroutineDeclaration& maxFuncIter);

/// Скоструюровать выражение-дерево, описывающее правую границу гнезда циклов по структуре LoopBounds.
/**
	По структуре LoopBounds,именам переменных и Namespace, в котором эти переменные видны, а также итераторе на описанную где-то функцию минимума из двух целых чисел построить выражение-дерево, описывающее правую границу цикла.
	Память освобождает тот, кто пользуется результатом.
	Кол-во элементов массива varNames равно m_lower[x].dim-1 (по правилам заполнения структуры LoopBounds).
*/
OPS::Reprise::ExpressionBase* GetRightExprNode(LoopBounds& lb,OPS::Reprise::Declarations& ns,std::string* varNames,OPS::Reprise::SubroutineDeclaration& minFuncIter);


/*
// создание  BaseNode* Bound по линейному выражению SimpleLinearExpression* le.(Вопросы Мише)
void BoundBN(SimpleLinearExpression*& le, char**& paramNames, int paramNamesDim, BaseNode*& Bound);

//копирование ветки First в Second (Вопросы Мише)
bool CopyExpr(BaseNode *First, BaseNode *&Second);
*/
	}


#endif
