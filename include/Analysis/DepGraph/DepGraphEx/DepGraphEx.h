#ifndef _DEPGRAPHEX_H_
#define _DEPGRAPHEX_H_

#include "Reprise/Reprise.h"
#include "Analysis/DepGraph/DepGraph.h"
#include <memory>
#include <vector>
#include <map>
#include <utility>


namespace DepGraph
{

	typedef OPS::Reprise::ReprisePtr<OPS::Reprise::ExpressionBase>		AutoExprNode;

	/// Интерфейс для предикатов
	class IPredicate
	{
	public:
		/// Получить предикат в виде выражения внутреннего представления
		virtual AutoExprNode GetExpr() const = 0;
		/// Получить красивое текстовое представление для предиката
		virtual std::string GetText() const = 0;
		/// Сделать полную копию предиката
		virtual IPredicate* Clone() const = 0;

        virtual ~IPredicate() {};
	};

	/// Создать предикат вида "LeftPred И RightPred"
	IPredicate* CreateAndPredicate(const IPredicate& leftPred, const IPredicate& rightPred);
	/// Создать предикат вида "LeftPred ИЛИ RightPred"
	IPredicate* CreateOrPredicate(const IPredicate& leftPred, const IPredicate& rightPred);
	/// Создать предикат вида "НЕ Pred"
	IPredicate* CreateNotPrecicate(const IPredicate& pred);

	/// Определяет, является ли предикат тождественно истинным т.е. true
	bool isTrue(const IPredicate& pred);
	/// Определяет, является ли предикат тождественно ложным т.е. false
	bool isFalse(const IPredicate& pred);

	typedef std::unique_ptr<IPredicate> PredicatePtr;

	PredicatePtr gcdTestSymbolicKnownBounds(const OccurDesc& rOccur1, const OccurDesc& rOccur2, unsigned supp);
	PredicatePtr banerjeeTestSymbolicMapped(const OccurDesc& rOccur1, const OccurDesc& rOccur2, unsigned supp, bool bLIDtest = false);
	PredicatePtr banerjeeTestSymbolicUnmapped(const OccurDesc& rOccur1, const OccurDesc& rOccur2, int carrier, int commonLoopNumb);

class IDepGraphEx
{
public:
	/// Узнать, есть ли условие существования дуги
	/** 
	    Функция позволяет определить, есть ли условие существования дуги
		зависимости указаной в качестве параметра.
		\return        - возвращает true если есть условие существования дуги.
			В том числе условием может быть константа false - т.е. дуги может
			не быть безусловно. Если функция вернула false, то это значит, что 
			дуга существует при любых значениях внешних переменных.
	**/
	virtual bool hasDepCond(LampArrow* pArrow) = 0;

	/// Получить предикат существования зависимости.
	/**
		Функция возвращает предикат, который является индикатором наличия 
		зависимости определяемой дугой pArrow графа	Лампорта. Если нужно узнать условие 
		существования конкретного носителя зависимости, то нужно пользоваться функцией
		getDepSuppCond().
		\param pArrow	- дуга, условие существования которой нужно узнать.
		\return			- возвращает указатель на объект типа IPredicate. 
	**/
	virtual PredicatePtr getDepCond(LampArrow* pArrow) = 0;

	//virtual PredicatePtr getNoDepCond(LampArrow* pArrow) = 0;

	/*	Получить условие при котором будет зависимость pArrow c 
		носителем nSupp.	*/
	virtual PredicatePtr getDepSuppCond(LampArrow* pArrow, int nSupp) = 0;
	
	//virtual PredicatePtr getNoDepSuppCond(LampArrow* pArrow, int nSupp) = 0;
    virtual ~IDepGraphEx() {};
};

class GlobalConditionTable //: public std::string
{
public:
	struct TableRec
	{
		/// Предикат
		//ExprNode* pred;
		/// Блок, в котором предикат вылиден
		OPS::Reprise::BlockStatement*	block;
		/// Значение, в которое обращается предикат
		bool	pred_value;

		TableRec(/*ExprNode* pPred,*/ OPS::Reprise::BlockStatement* pBlock, bool bPredVal)
			/*:pred(pPred)
			,*/:block(pBlock)
			,pred_value(bPredVal)	{	}
	};

	static GlobalConditionTable* Get(OPS::Reprise::ProgramUnit& rProg);


	typedef std::map<std::pair<const OPS::Reprise::ReferenceExpression*,const OPS::Reprise::ReferenceExpression*>, TableRec> Table;		// NB! Исправить!!!!!!!!!!!!!!!
	typedef Table::const_iterator ConstIterator;
	typedef Table::iterator Iterator;

	ConstIterator Begin() const;
	ConstIterator End() const;
	ConstIterator Find(const std::pair<const OPS::Reprise::ReferenceExpression*,const OPS::Reprise::ReferenceExpression*>& pSrcDest) const;
	void Add(const std::pair<const OPS::Reprise::ReferenceExpression*,const OPS::Reprise::ReferenceExpression*>& pSrcDest, const TableRec& rec);
	void Delete(const std::pair<const OPS::Reprise::ReferenceExpression*, const OPS::Reprise::ReferenceExpression*>& pSrcDest);

protected:
	Table m_mTable;
};
class ConditionTableMarker
{
public:

	~ConditionTableMarker();

	/**
	Clear all marks.
	*/
	void clear(void)
	{
		m_marks.clear();
	}
	/**
	Checks that node has at least one mark
	\param	node - Reprise node to check marks of
	\return at least one mark is present
	*/
	bool hasMarks(const OPS::Reprise::RepriseBase& node) const
	{
		return m_marks.find(&node) != m_marks.end();
	}

	/**
	Adds mark to specified node.
	\param	node - Reprise node to add mark to
	\param	kind - mark kind to add to node
	\return	true if added, otherwise - mark already present
	*/
	bool addMark(const OPS::Reprise::RepriseBase* node, GlobalConditionTable* info)
	{
		bool b = m_marks.find(node) != m_marks.end();
		m_marks[node] = info;
		return !b;
	}

	/**
	Marks getter.
	\param	node - Reprise node to get marks storage for
	\return	marks storage for node
	*/
	GlobalConditionTable* getMark(const OPS::Reprise::RepriseBase& node)
	{
		return m_marks[&node];
	}

private:
	typedef std::map<const OPS::Reprise::RepriseBase*, GlobalConditionTable*> TMarkedNodes;
	TMarkedNodes m_marks;
};

//ConditionTableMarker condTableMarker;

/// Функция строящая улучшеный граф зависимостей.
std::unique_ptr<IDepGraphEx> getDepGraphEx(IndOccurContainer& rOccursContainer, LamportGraph& rLamportGraph );

/// Тест дуги на свободные переменные
/** 
	Тест дуги на предмет того, что она создана только по причине наличия 
	свободных переменных в индексных выражениях. 
	\param rOccurs			- контейнер вхождений по которому построен граф
	\param rLampArrow		- дугу которую нужно тестировать 
	\throw FreeParamsError	  на вход подан неправильный контейнер вхождений
*/
bool testArrowOnFreeParams(const LampArrow& rLampArrow);


/// Тест графа на наличие дуг со свободными переменными
/** 
	Тестирует граф на предмет наличия в нем дуг порожденных свободными
	переменными в индексных выражениях массивов.
	\param rOccurs			- контейнер вхождений по которому построен граф
	\param rLamportGraph	- граф Лемпорта который нужно тестировать
	\throw FreeParamsError	  на вход подан неправильный контейнер вхождений
*/
bool testGraphOnFreeParams(IndOccurContainer& rOccurs, LamportGraph& rLamportGraph);


/// Составление списка дуг со свободными переменными.
/**
	Возвращает вектор дуг, которые созданы по причине наличия свободных
	переменных в индексных выражениях массивов. Если передаваемый вектор 
	не пуст - он НЕ очищается.

	\note	В возвращаемом векторе содержатся указатели на дуги графа Лемпорта.
	Именно те дуги, которые содержатся в графе, так что память из под них 
	очищать	не нужно и нельзя.
	
	\param rLamp			- граф Лемпорта дуги которого нужно тестировать
	\param rResult			- ссылка не вектор в который нужно складыват дуги
	\throw	FreeParamsError	  на вход подан неправильный контейнер вхождений

	\note	Если нужно только узнать есть-ли такие дуги, то лучше воспользоваться
	функцией testGraphOnFreeParams().
*/
void getArrowsWithFreeParams(LamportGraph& rLamp, 
							 std::vector<LampArrow*>& vResult );

OPS::Reprise::IfStatement* makeDynamicTransform(OPS::Reprise::StatementBase& pStatement, std::vector<LampArrow*>& vArrows, IDepGraphEx& depGraphEx);

}
#endif //_DEPGRAPHEX_H_
