#ifndef OPS_TRANSFORMATIOINS_SWAP_STATEMENTS_H_
#define OPS_TRANSFORMATIOINS_SWAP_STATEMENTS_H_

#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"
#include "Reprise/Reprise.h"
#include "Reprise/Statements.h"

namespace OPS
{
namespace Transforms
{
	/*
	* Возвращает true, если после перемены местами двух стетментов программа останентся эквивалентной исходной, 
	* и false в противном случае.
	*/ 
	bool maySwapStmts (/*const*/ Reprise::StatementBase* /*const*/ stmt1, /*const*/ Reprise::StatementBase* /*const*/ stmt2);

	/*
	* Возвращает true, если преобразование сработало (т. е. если maySwap от тех же аргуметов вернёт true), 
	* и false в противном случае.
	*/ 
	bool trySwapStmts  (/*const*/ Reprise::StatementBase* /*const*/ stmt1, /*const*/ Reprise::StatementBase* /*const*/ stmt2);

} // end namespace Transforms
} // end namespace OPS

#endif // OPS_TRANSFORMATIOINS_SWAP_STATEMENTS_H_
