#ifndef _IDENTIF_
#define _IDENTIF_

#ifdef _MSC_VER
#pragma warning(disable : 4786)	// Запрещаем сообщение об обрубании имен в debug-версии
#endif

#include "Reprise/Expressions.h"
#include "Reprise/Statements.h"

namespace DepGraph
{
namespace Id
{

using namespace OPS::Reprise;
class myStmtStr
{ 
public:
	/// Оператор
	StatementBase * m_pStmt;
  /// текущая позиция
	long m_nCurPos;

	myStmtStr(StatementBase * pStmt, int nPos )	: m_pStmt(pStmt), m_nCurPos(nPos)
	{}

	void print();
};

/// Информация о вхождении переменной
struct OccurrenceInfo
{
	/// ссылка на имя в пространстве имен
	const VariableDeclaration* m_oDataObject;
	/// кол-во охватывающих циклов
	int m_numcycles;	
	// размерность переменной
	int m_dim;	
	/// данное вхождение является счетчиком циклов
	bool m_bIsLoopIndex; 
	///  ссылка на сам элемент, 
	ReferenceExpression * m_pED;
	
	/// Флаг, является ли это вхождение генератором
	/// Если = false, значит использование
	bool	m_generator;

	OccurrenceInfo();
	
	OccurrenceInfo(const VariableDeclaration* n1, bool gen, int LoopNum, int dim, bool bLin, ReferenceExpression * pED, bool bIsLoopIndex);
	bool IsIndexesLinear() const { return m_isIndexExprLinear; }
	bool IsArray() const { return m_dim != 0; }

//private:
	/// признак линейности индексных выражений
	bool m_isIndexExprLinear;  
};

typedef OccurrenceInfo elem; // deprecated name

/// Сведения о цикле
struct LoopInfo
{
  	/// Имя счетчика цикла 
	VariableDeclaration* m_oDataObject;
	/// Счетчик цикла
	ExpressionBase * pthis;
	/// Левая и правая границы и шаг
	ExpressionBase * m_left, *m_right, *m_step; 
	/// Сам оператор цикла
	ForStatement* pStmtFor;

	LoopInfo();
	explicit LoopInfo(OPS::Reprise::ForStatement& forStmt);

	void reset(OPS::Reprise::ForStatement& forStmt);
};

typedef LoopInfo elem1;	// deprecated name

/// Класс, строящий информацию о вхождениях переменных и циклах
/// в выбранном фрагменте программы.
class id 
{
	std::vector<myStmtStr> m_StmtStack;
	StatementBase *m_pFirst, * m_pEnd; 
	bool m_bFindNext;

public:

	id();

	///  устаревшая
	/// инициализация оператором типа OT_STMT_BLOCK
	/// оператор next пройдет все вложенные в него операторы
	//id(StatementBase * pStmt);  ///< [depricated]
	
	/// инициализация оператором типа OT_STMT_BLOCK
	/// оператор next пройдет все вложенные в него операторы
	id(BlockStatement & rBlock);

    /// инициализация оператором типа OT_STMT_BLOCK
	/// оператор next пройдет все вложенные в него операторы
	id(BlockStatement * pBlock);

	//id(Statement * pFirstStmt, Statement *pSecondStmt);
	// вводит интервал
	// next проходит этот интервал, потом 0
	id(StatementBase *pStmt1, StatementBase *pStmt2);

	/// переинициализация оператором (оператор Block) 
	/// оператор next пройдет все вложенные в него операторы
	//void reset(StatementBase *pStmt);  	///< [depricated]

	/// переинициализация оператором типа блок
	/// оператор next пройдет все вложенные в него операторы
	void reset(BlockStatement & rBlock);

	/// переинициализация двумя операторами
	/// оператор next пройдет все операторы от pStmt1  до pStmt2 
	/// если pStmt2 == 0,  тогда next пройдет все операторы от pStmt1 до конца
	///  тела функции, которой принадлежит pStmt1
	void reset(StatementBase *pStmt1, StatementBase *pStmt2);
	void resetForSSA(StatementBase *pStmt1, StatementBase *pStmt2);

  /// сборос положения в предыдущее инициализированное. 
	/// при этом считается что инициализация была либо оператором Block, либо 
	/// двумя операторами (второй может быть пустым)
	void reset();
	
	~id(){clear();}
	
  /// печать содержимого внутреннего вектора
	void print();
	
	/// получить следующий оператор
	StatementBase * next(); 

	/// получить следующий оператор
	/// без проверки задания границ
	StatementBase * nextL();

	/// получение предыдущего оператора или 0 если его нет
	/// поиск идет в охватывающем блоке в сслуяае инициализации блоком
	///  или во всем теле текущей функции
	StatementBase * prev(); 
	
	/// получить текущий оператор
	StatementBase * getThisOper();

	/// получить первый оператор отрезка
	StatementBase * first(){return m_pFirst;}

	/// получить последний оператор отрезка
	StatementBase * last(){return m_pEnd;}

    ///  возвращает описание объемлющих циклов 
    ///  по умолчанию просматривает все охватываещие цыклы в теле данной 
    ///    функции. Если bGoTop = 0 тогда просматриваются операторы внутри 
    ///    выбранного оператора или блока 
	void getOuterLoopsList(std::vector<LoopInfo> & A, bool bGoTop = true);

	/// если данный оператор - оператор цикла, то происходит 
	/// переинициализация телом данного цикла
	int initWithLoopBody(StatementBase * pLoop);
	

	/// очистка памяти
	void clear();

};

/// класс, инкапсулирующий операции с выражениям (поиск, перестроение, вычисление)
///  приведение подобных, получение коэффициентов
class ExprManagement 
{
public:

	/// Поиск в выражении pExpr  переменной pSource
	static bool findSubExpr(ExpressionBase * pExpr,  ExpressionBase * pSource);


};

ReprisePtr<ExpressionBase> getCoef(ExpressionBase& source, VariableDeclaration& itNO);

ReprisePtr<ExpressionBase> getFreeCoef(ExpressionBase& source, const std::vector<VariableDeclaration*>& exprBase);


/// проверка линейности pSource относительно pExpBase
bool checkLinByBases(ExpressionBase * pSource, std::vector<ExpressionBase*>& pExpBase);

}
}
#endif
