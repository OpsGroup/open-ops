#include "Reprise/ProgramFragment.h"
#include "Shared/StatementsShared.h"
#include "Shared/Checks.h"
#include "Shared/Checks/CompositionCheck/CompositionCheckObjects.h"
#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/DepGraph/id.h"

#include <iostream>

using namespace DepGraph;
using namespace Id;
using namespace OPS::Reprise;
using namespace OPS::Shared;
using namespace OPS::Shared::Checks;

using namespace std;

namespace OPS
{
namespace Transforms
{
	bool maySwapStmts (StatementBase* stmt1, StatementBase* stmt2)
	{
		OPS_ASSERT( stmt1 != NULL && stmt2 != NULL );

		// если они равны, то менять, хоть и бестолку, но можно
		if (stmt1 == stmt2)
			return true;

		// проверка наличия во фрагменте недопустимых операторы
		// так же нельзя их менять, если они содержат continue, break, goto или метки
		// если между стейтментами есть continue, break, goto или метки, указывающие на что-то между ними, 
		//то менять их нельзя
        if (!ProgramFragment::isFragment(*stmt1, *stmt2))
            return false;

		ProgramFragment programFragment(*stmt1, *stmt2);
		CompositionCheckObjects compositionCheckObjects;
		compositionCheckObjects << CompositionCheckObjects::CCOT_BlockStatement
			<< CompositionCheckObjects::CCOT_EmptyStatement 
			<< CompositionCheckObjects::CCOT_ExpressionStatement 
			<< CompositionCheckObjects::CCOT_ForStatement 
			<< CompositionCheckObjects::CCOT_IfStatement 
			<< CompositionCheckObjects::CCOT_SwitchStatement 
			<< CompositionCheckObjects::CCOT_WhileStatement;
		if (!makeCompositionCheck(programFragment, compositionCheckObjects))
			return false;

		// если стейтменты принадлежат разным родителям, их менять нельзя
		if (stmt1->getParent() != stmt2->getParent()) // без линеаризации блоков это работает не до конца корректно
			return false;

		// общий родитель stmt1 и stmt2
		RepriseBase* parent = stmt1->getParent();

		// два различных оператора имеют общего родителя IfStatement или SwitchStatement, значит, 
		// они лежат в разных ветках, иначе их общим родителем был бы BlockStatement
        if (parent->is_a<IfStatement>())
			return false;

		/* Анализ зависимостей */
		// создание и построение графа Лампорта
		std::unique_ptr<LamportGraph> lg(new LamportGraph());
		id theId(stmt1, stmt2);

		lg->Build( theId ); // имеют общего родителя
			
		// для построения зависимостей на фрагменте между stmt1 и stmt2 включительно необходимо узнать 
		// порядок выполнения stmt1 и stmt2 и в зависимости от него строить либо id(stmt1, stmt2), id(stmt2, stmt1)
		if (ProgramFragment::isFragment(*stmt1, *stmt2))
			lg->Build( theId );
		else 
			if (ProgramFragment::isFragment(*stmt2, *stmt1))
			{
				id theId2(stmt2, stmt1);
				lg->Build( theId2 );
			}
			else 
				return false;
			
			// тестирование зависимостей между stmt1 и stmt2
			if ( lg->TestDep(FLOWDEP, stmt1, stmt2)
				|| lg->TestDep(ANTIDEP, stmt1, stmt2)
				|| lg->TestDep(OUTPUTDEP, stmt1, stmt2) )
				return false;
			
			// тестирование зависимостей между stmt2 и stmt1
			if ( lg->TestDep(FLOWDEP, stmt2, stmt1)
				|| lg->TestDep(ANTIDEP, stmt2, stmt1)
				|| lg->TestDep(OUTPUTDEP, stmt2, stmt1) )
				return false;

			// тестирование зависимостей между [stmt1, stmt2) и stmt2
			if ( lg->TestDep(FLOWDEP, stmt1, stmt2, stmt2)
				|| lg->TestDep(ANTIDEP, stmt1, stmt2, stmt2)
				|| lg->TestDep(OUTPUTDEP, stmt1, stmt2, stmt2) )
				return false;

		// во всех остальных случаях их менять можно
		return true;
	}
	
	bool trySwapStmts (/*const*/ Reprise::StatementBase* /*const*/ stmt1, /*const*/ Reprise::StatementBase* /*const*/ stmt2)
	{
		if (!maySwapStmts(stmt1, stmt2))
			return false;

		return swapStmts(stmt1, stmt2);
	}
}
}
