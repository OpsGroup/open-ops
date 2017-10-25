#ifndef OPS_SHARED_STATEMENTS_H
#define OPS_SHARED_STATEMENTS_H

#include "Reprise/Statements.h"

namespace OPS
{
namespace Shared
{

/* Определяет содержит ли outsideStmt insideStmt. 
* \return - возвращает true, если outsideStmt содержит insideStmt или они совпадают, 
* и false в противном случае.
*/
bool contain(const Reprise::StatementBase* const outsideStmt, const Reprise::StatementBase* const insideStmt);

/*
Вставка фрагмента [fromFirstIt, fromLastIt] в block перед выражением на которое указывает iterator 
*/
//BlockStatement::Iterator addBefore
//	(BlockStatement* const source, BlockStatement::Iterator& to, 
//	BlockStatement::Iterator fromFirstIt, BlockStatement::Iterator fromLastIt);

/*
Линеаризация block. Линеаризация, или упрощение, блоков происходит по следующей схеме: если внутри блока 
(т.е. оператора типа BlockStatement) находится другой блок, то содержимое вложенного блока делается содержимым 
внешнего, при этом вложенный блок становится пустым и удаляется. 
Функция действует рекурсивно и линеаризует и все вложенные блоки.
*/
//void toLinearBlock(Reprise::BlockStatement* const block);


/*
* Возвращает true, если преобразование сработало и false в противном случае (только если один оператор вложен в другой).
*/ 
bool swapStmts  (/*const*/ Reprise::StatementBase* /*const*/ stmt1, /*const*/ Reprise::StatementBase* /*const*/ stmt2);

/**
	Returns set of child statements
*/
std::set<OPS::Reprise::StatementBase*> getChildStatements(OPS::Reprise::StatementBase* pStatement, bool includeSourceStatement);

//если один находится внутри другого, то возвращается этот один
OPS::Reprise::RepriseBase* getFirstCommonParent(OPS::Reprise::RepriseBase& node1, OPS::Reprise::RepriseBase& node2);

OPS::Reprise::RepriseBase* getFirstCommonParentEx(Reprise::RepriseBase &node1, Reprise::RepriseBase &node2,
												  Reprise::RepriseBase*&node1Parent, Reprise::RepriseBase*&node2Parent);

bool contain(const Reprise::RepriseBase& outsideNode, const Reprise::RepriseBase& insideNode);

/* Находит наименьший простой блок, который включает заданный
* \return - возвращает наименьший блок, который включает заданный 
* и имеет на управляющем графе единственный вход и единственный выход
*/
OPS::Reprise::BlockStatement* getOuterSimpleBlock(OPS::Reprise::BlockStatement* block);

OPS::Reprise::BlockStatement* getSurroundingBlock(OPS::Reprise::StatementBase* currentStatement);

} // end namespace Shared
} // end namespace OPS


#endif // OPS_SHARED_STATEMENTS_H
