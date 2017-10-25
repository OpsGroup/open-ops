#pragma once
#include <Analysis/LatticeGraph/LinearLib.h>

namespace OPS
{
namespace LatticeGraph
{

/*
Класс, описывающий вектор-счетчик n-мерного тесного гнезда циклов без внешних параметров в границах
*/
class LoopIndex
{				
    /// вектор счетчика.
    int* m_counterValues;

    /// размерность счетчика, количество циклов в гнезде
    int m_dim;

    SimpleLinearExpression* m_loopLowerBounds; 
    SimpleLinearExpression* m_loopUpperBounds; 


    /// Опорный многогранник, которому принадлежит вектор (не владеет)
//    Context* suppPolyhedron;   - делать LoopIndex на основе опорных многогранников - плохо, когда требуется пробегать
    //гнездо с индексами произвольного знака. Функции FindMaximum и Minimum не работают. Можно сделать замену, но
    //обратный переход осуществить сложно теоретически. Саша Шульженко сделал для частного класса задач. Если что-то
    //не так он выводил сообщение: "Unexpected situation: Couldn't compute added params values\nAbnormal function termination (FindLexMinimum)"
    //Теоретический алгоритм обратной замены существует (правда пока только у меня на черновиках).

    ///флаг, указывающий, что счетчик внутри границ циклов
    bool m_inBounds;
public:
    ///Как говориться, Construction/Destruction
    LoopIndex(int dim,SimpleLinearExpression* loopLowerBound,SimpleLinearExpression* loopUpperBound):m_dim(dim), m_loopLowerBounds(loopLowerBound),m_loopUpperBounds(loopUpperBound),m_inBounds(false){m_counterValues=new int[m_dim];}
	~LoopIndex(){if(m_counterValues) delete[] m_counterValues;}

    /// Очистить
	void Clear(){if(m_counterValues){delete[] m_counterValues;m_counterValues=NULL;}}

    ///инициализировать счетчик нижней границей
    void SetToLowerBounds();

    ///инициализировать счетчик верхней границей
    void SetToUpperBounds();

    //В результате работы этой функции, данный вектор (счетчик) принимает значение той итерации, 
    //после которой сразу выполняется итерация loopIndex ("сразу" означает, что у этого счетчика 
    //нет итерации до loopIndex)
    ///commonLoopNumb - кол-во общих циклов
    void InitWithLower(LoopIndex& loopIndex,int commonLoopNumb);
    //В результате работы этой функции, данный вектор (счетчик) принимает значение той итерации, 
    //перед которой сразу выполняется итерация loopIndex ("сразу" означает, что у этого счетчика 
    //нет итерации до loopIndex)
    ///commonLoopNumb - кол-во общих циклов
    void InitWithUpper(LoopIndex& loopIndex,int commonLoopNumb);

    /// Обращение к элементам вектора-счетчика
    int& operator[](int index){return m_counterValues[index];}
    int* getCounterValues(){return m_counterValues;}
    const int* getCounterValues()const {return m_counterValues;}

    /// размер
    int getSize()const {return m_dim;}


    //выполняет операцию ++, считая, что размерность индеса равна dimNum
    void increase(int dimNum);
    //выполняет операцию --, считая, что размерность индеса равна dimNum
    void decrease(int dimNum);

    ///увеличить значение вектор-итерации на 1 (одну итерацию). Если до увеличения, вектор описывал последнюю итерацию гнезда циклов, то флаг m_inBounds станет равен 0
    void operator++(int){increase(m_dim);}
    ///уменьшить на 1
    void operator--(int){decrease(m_dim);}

    bool operator==(LoopIndex& li);
    bool operator!=(LoopIndex& li){return !(*this==li);}

    //возвращает верхнюю границу для координаты номер i ТЕКУЩЕГО счетчика, 
    //подставляя в выражение m_loopLowerBounds[i] значения верхних счетчиков
    int getUpperBoundForCoord(int i);

    //возвращает верхнюю границу для координаты номер i ТЕКУЩЕГО счетчика
    //подставляя в выражение m_loopUpperBounds[i] значения верхних счетчиков
    int getLowerBoundForCoord(int i);

    /// Счетчик внутри границ цикла?
    bool isInBounds() const {return m_inBounds;}

    //Лексикографическое сравнение векторов
    static int LexCmp(const LoopIndex& a, const LoopIndex& b);
    //возвращаемые значения:
    //	<0		a < b
    //	 0		a = b
    //	>0		a > b

    static int LexCmp(const LoopIndex& a, const LoopIndex& b,int n);//Лексикографическое сравнение векторов. НО, сравниваются только n первых компонент. Полагается, что число компонент каждого вектора >=n
    //возвращаемые значения:
    //	<0		a < b
    //	 0		a = b
    //	>0		a > b
};

std::ostream& operator<<(std::ostream& os,LoopIndex& li);

}//end of namespace
}//end of namespace

