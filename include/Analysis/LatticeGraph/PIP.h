#ifndef _PIP_
#define _PIP_

#include "Analysis/DepGraph/Status.h"
#include "Analysis/LatticeGraph/LinearLib.h"
#include <ostream>
#include <list>
#include <vector>
	
/******** НАСТРОЙКИ ДЛЯ ОТЛАДКИ PIP ********/

//#define LATTICE_TABLES_DEBUG

/// Следующую строчку можно откомментировать только если откомментирована предыдущая
//#define LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS

#ifndef LATTICE_TABLES_DEBUG
#undef LATTICE_TABLES_DEBUG_SHOW_FEAS_SOLVING_PROCESS
#endif
/******** +=+=+=+=+=+=+=+=+=+=+=+++ ********/

#define SPARE_DIM 1
namespace OPS
{
namespace LatticeGraph
{

enum LWTNodeStatus{NO_SOLUTION=1};
enum TSIGN{NEGATIVE=1/* <0 */,POSITIVE/* >=0 */,ZERO/* =0 */,UNKNOWN};

struct Polyhedron;

/**
	Структурка, описывающая зависимость нового (добавляемого) параметра от уже имеющихся параметров, при добавлении отсечения в Симплекс Методе.
	Если добавленный параметр "q" равен "линейное выражение нацело деленное некоторое целое число", то 
	эта структура как раз и хранит это "линейное выражение нацело деленное некоторое целое число".
*/
struct SimpleLinearExpression;

struct NewParamEquation
{
	/// Размерность выражения. Равна кол-ву переменных от которых зависит Новый (Вводимый) параметр + свободный член.
	int m_dim;

	/// Число, на которое нужно разделить целочисленно все элементы массива.??? (Все выражение, а не массив)
	int m_denom;
	
	/// Данные. Коэффициенты при элементах в выражении. m_coefs[m_dim-1] - свободный член.
	int* m_coefs;

	NewParamEquation():m_dim(0),m_coefs(NULL){}
	NewParamEquation(int _dim):m_dim(_dim){m_coefs=new int[m_dim];}
	NewParamEquation(NewParamEquation& newParEq);
	~NewParamEquation(){Clear();}
	void Clear(){if(m_coefs){delete[] m_coefs;m_coefs=NULL;}}
	int GetFree(){return m_coefs[m_dim-1];}
	void operator=(NewParamEquation& newParEq);
    //делает аффинную замену переменных x=t(y). Матрица преобразования такая же как и в SimpleLinearExpression::transform
    void transform(int** transformMatrix,int dimi,int dimj);
    //возвращает линейное выражение в числителе (свободный член в SimpleLinearExpression стоит на первом месте!!!!!!!)
    SimpleLinearExpression* getLinExpr();
    void insertNZerosBeforeNewParamCoefs(int N, int varNum);//varNum - кол-во обычных переменных (не новых параметров)
    //возвращает систему из двух неравенств, определяющих новый параметр
    Polyhedron getInequalities();
    //возвращает number = 0 или 1 - левое и правое неравенства, определяющие новый параметр
    Inequality getInequality(int number);
};


/**
	Cписок, описывающий выражения для добавленных параметров.
	Порядок следования элементов списка строго фиксирован!
	Некоторый элемент списка определяет параметр, который используется в следующем по порядку элементе списка для определения следующего параметра.
*/
class NewParamVector
{
public:
    std::vector<NewParamEquation*> paramEqVector;

    NewParamVector(){};
	NewParamVector(NewParamVector& newParamList);
	~NewParamVector(){Clear();}
	std::vector<NewParamEquation*>::iterator Begin(){return paramEqVector.begin();}
	std::vector<NewParamEquation*>::iterator End(){return paramEqVector.end();}
	void PushBack(NewParamEquation* paramEq)
    {
        paramEqVector.push_back(paramEq);
    }
	NewParamEquation* Back(){return paramEqVector.back();}
	NewParamEquation*& operator[](int index){return paramEqVector[index];}
    NewParamEquation*& at(int index){return paramEqVector[index];}
	void DeleteBack();//{if(paramEqVector.size()){delete Back();paramEqVector.pop_back();}}
	void operator=(NewParamVector& newParamList);
	void Clear();
	int GetSize(){return (int)paramEqVector.size();}
	void Print(std::ostream& os,const char** paramNames,int paramNamesDim);
	void Print(std::ostream& os,std::string* paramNames,int paramNamesDim);
    //выводит выражения для параметров в строку, отступ от левого края - marginLeft
    //ровно GetSize() последних paramNames должны быть именами новых вводимых параметров
    std::string toString(std::vector<std::string>* paramNames,int marginLeft=0);

	/// Функция пытается подсчитать значения параметров, которые описываются равенствами списка. 
	/** Если некий параметр зависит от параметра, который не определяется уравнением из имеющегося списка (т.е. от внешнего параметра, коэффициент при котором не 0),
		то возвращается NULL (ошибка, подсчитать результат невозможно).
		Иначе возвращается вектор, кол-во элементов в котором равно числу параметров (или числу элементов списка). Каждый элемент хранит значение соответствующего параметра.
		Возвращенную память после использования нужно ВЕРНУТЬ системе (delete[] ..)
	*/ 
	int* ComputeParams();//работает, когда нет неизвестных параметров, т.е. первый извествен, а остальные определяются через него

    //вычисляет значения параметров для заданного вектора известных параметров
    std::vector<int>& evaluate(std::vector<int>& knownParams);  

    //делает аффинную замену переменных x=t(y)
    void transform(int** transformMatrix,int dimi,int dimj)
        {for (int i=0;i<GetSize();i++) 
            paramEqVector[i]->transform(transformMatrix,dimi,dimj);}

    void insertNZerosBeforeNewParamCoefs(int N, int varNum)//varNum - кол-во обычных переменных
        {for (int i=0;i<GetSize();i++) 
            paramEqVector[i]->insertNZerosBeforeNewParamCoefs(N,varNum);}
};

/**
	Структура, описывающая таблицу Симплекс Метода.
*/
struct Tableau
{
	/// Двумерный массив элементов таблицы.
	int** data;//m_coefs - 2-д массив, описывающий таблицу; sign - 1-д массив, описывающий знаки строк

	/// Одномерный массив, описывающий знаки строк. Элементы массива принимают значения, определенные enum TSIGN.
	TSIGN* sign;

	/// dimi, dimj - количество актуальных строк, столбцов в таблице (т.е. тех строк, столбцов, которые описывают данные в таблицы).
	int dimi,dimj;
	
	/// spareDim - кол-во свободных строк (столбцов); в каждый момент времени в таблице выделено памяти для dimi+spareDim строк и dimj+spareDim столбцов.
	int spareDim;

    /// Кол-во переменных таблицы (все остальные столбцы описывают свобдный член t(z) см. статью Фотрье).
	int varNum;

    //Количество положительных параметров (точнее неотрицательных)
    int positiveParamNum;

	/// Конструктор с автоматическим заполнением таблицы нулями. 
    //positiveParamNum - количество неотрицательных параметров кроме varNum - используется в ApplySimplex
    //коэффициенты при неотрицательных параметрах идут после неизвестных varNum штук и своб. члена
	Tableau(int _dimi,int _dimj,int _varNum,int _positiveParamNum);
	Tableau(Tableau& tb);
	~Tableau(){Clear();}
	void Clear();

	/// Удаляет избыточные строки
	void Simplify();

	Tableau& operator=(Tableau& tb);

	void AddSpareDims();

	/// Добавляет новую строку в конец таблицы. Строка зануляется.
	void AddNewRow();
};
std::ostream& operator<<(std::ostream& os,Tableau& tab);

struct TreeNode:public DepGraph::Status
{
    //симплекс таблица
	Tableau* tableau;
    //текущий знаменатель в целочисленных делениях, 
	int d;
    //контекст, с добавленными неравенствами, определяющими введенные новые параметры
	Polyhedron* context;
    //Cписок, описывающий выражения для параметров, добавленных в вешестоящих и этом узлах.
    //Каждый следующий параметр выражается через предыдущий
	NewParamVector* newParamVector;
    //условие охватывающего оператора if в форме Фотрье
    SimpleLinearExpression m_condition;
    //левая и правая симплекс таблицы - по правой идем, когда m_condition >=0, 
    //по левой - в противним случае
	TreeNode* left,*right;
	TreeNode():tableau(NULL),context(NULL),newParamVector(NULL),left(NULL),right(NULL){}
	TreeNode(Tableau* tab,int _d,Polyhedron* con,NewParamVector* newParList):tableau(tab),d(_d),context(con),newParamVector(newParList),left(NULL),right(NULL){}
	~TreeNode();
};

int FindNegative(Tableau* tab);
int  FindUnknown(Tableau* tab);
int LexComp(Tableau* tab,int i1,int i2,int d1,int d2);// Лексикограф. отношение для столбцов
int FindPivot(Tableau* tab,int psNum);// psNum==pivoting string Number
//возвращает 0, если в ходе работы функции получено, что данная пара: таблица, контекст - не допустимые (->не опт)
int CalcTableauSigns(Tableau* tab,Polyhedron* con);
int FindOpposite(Tableau* tab,int toString,int begCol);

//находит лексикографический МИНИМУМ. Добавляет найденный набор функций и их областей определения в solTabs
//деревой if-ов Фотрье получается в newRoot
//Неизвестные - первые varNum столбцов матрицы 
//root->tableau предполагаются положительными, параметры могут быть произвольного знака. 
//Количество положительных параметров содержится в root->tableau->positiveParamNum
//Память под solution, root и newParamVector надо освобождать вручную!!!!!!!!!!!!!!!!!!!
void ApplySimplex(TreeNode* root,std::list<TreeNode*>& solution,NewParamVector* newParamList=NULL);
void ApplySimplexForFeasibilityProblem(TreeNode* root,std::list<TreeNode*>& solution,NewParamVector* newParamList=NULL);

int lackDiv(int a,int b);//деление с недостатком а/b

/// Найти остаток от деления a на b. Получившийся остаток всегда >=0, НО b должно быть > 0.
int GetRemainder(int a,int b);//{int r=a%b;if(r<0)r+=b;return r;}


/// Построить обратную матрицу (к квадратной).
/*
	Функция возвращает обратную матрицу inverted к матрице src. 
	Массив, для хранения матрицы inverted, создается внутри данной функции и после использования подлежит УДАЛЕНИЮ из памяти тем, кто вызвал.
	Обратная матрица задается в целых числах -> каждый элемент ее нужно поделить на denom (если denom!=1).
	Возвр. 0, если обратная матрица найдена.
*/
int GetInvertedMatrix(int** src,int dimi,/*int dimj,*/int**& inverted,int& denom);

/// Построить целочисленную обратную матрицу к прямоугольной, возможно содержащей нулевые столбцы, на заданном множестве.
/*
	Функция возвращает обратную матрицу inverted к матрице src. 
	Массив, для хранения матрицы inverted, создается внутри данной функции и после использования подлежит УДАЛЕНИЮ из памяти тем, кто вызвал.
	Обратная матрица задается в целых числах -> каждый элемент ее нужно поделить на m_denom (если m_denom!=1).
	Возвр. 0, если целочисленная обратная матрица найдена.
*/
int GetInvertedMatrix(int** src,int dimi,int dimj,int**& inverted);

///Найти лексикографический минимум области.
/**
	Возвращает вектор, являющийся лексикографическим минимумом в области con. 
	Возвращает NULL в случае ошибки, либо в случае пустоты области.
	Память очищает тот, кто пользуется.
*/
int* FindLexMinimum(Polyhedron& con,int loopNumb);

///Найти лексикографический максимум области.
/**
	Возвращает вектор, являющийся лексикографическим максимумом в области con. 
	Возвращает NULL в случае ошибки, либо в случае пустоты области.
	Память очищает тот, кто пользуется.
*/
int* FindLexMaximum(Polyhedron& con,int loopNumb);


///Найти максимальное значение которое может принимать переменная в контексте
/**
	Возвращает максимальное значение некоторой переменной в контексте.
	Если возвращается false, значит это значение не вычислено.
	Параметр varPos определяет номера элементов массивов, которые описывают коэфф. при рассматриваемой переменной.
*/
//int* FindMaxValueForVar(Context& con,int varPos);

bool FindMaxValueForVar(const Polyhedron& con,int varPos, int& res);

}
}

#endif
