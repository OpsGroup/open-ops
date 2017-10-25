#include "Transforms/Loops/LoopCycleOffset/LoopCycleOffset.h"

#include "Reprise/Service/DeepWalker.h"
#include "Shared/RepriseClone.h"
#include "OPS_Core/Helpers.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include "Analysis/Montego/DependenceGraph/IndexAnalysis.h"

#include <iostream>
#include "OPS_Core/Localization.h"

namespace OPS
{

	namespace Transforms
	{
		using namespace OPS::Reprise;
		using namespace OPS::TransformationsHub;

		LoopCycleOffset::Replacer::Replacer(RepriseBase& old_expr, RepriseBase& new_expr)
			: m_old_expr(old_expr)
			, m_new_expr(new_expr)
		{
		}

		void LoopCycleOffset::Replacer::visit(ReferenceExpression& reference_expr)
		{
			if (!dynamic_cast<ExpressionBase*>(&m_old_expr) || !dynamic_cast<ExpressionBase*>(&m_new_expr))
				return;

			if (m_old_expr.cast_to<ExpressionBase>().isEqual(reference_expr))
				Editing::replaceExpression(reference_expr, ReprisePtr<ExpressionBase>(m_new_expr.cast_to<ExpressionBase>().clone()));
		}

		LoopCycleOffset::LoopCycleOffset(): TransformBase()
		{
			// Create parameters descriptions

			// 1-st parameter is FOR statement
			ArgumentInfo firstArgInfo(ArgumentValue::StmtFor, _TL("For operator which'll be transformed.", "Трансформируемый цикл For."));
			this->m_argumentsInfo.push_back(firstArgInfo);
			//m_argumentsInfo.addListArg(_TL("Dependencies type", "Тип зависимостей"), _TL("True dependence", "Истинная"), _TL("Antidependence", "Антизависимость"));
		};

		int indexOf(BlockStatement *for_body_statement, const RepriseBase* statement)
		{
			for (int index = 0; index < for_body_statement->getChildCount(); ++index)
				if (&for_body_statement->getChild(index) == statement)
					return index;
			return -1;
		}

		int checkDependencies(ForStatement *for_statement)
		{
			typedef std::map<int, int> slices_map; 
			typedef std::pair<int, int> slice_pair;

			BlockStatement* for_body_statement = &for_statement->getBody();

			Montego::DependenceGraph graph(for_statement->cast_to<StatementBase>());
			graph.removeCounterArcs();

			slices_map slice_parameters; //map of possible slices
			Montego::DependenceGraph::ArcList list = graph.getAllArcs();
			for (Montego::DependenceGraph::ArcList::iterator list_it = list.begin(); list_it != list.end(); ++list_it)
			{
				std::vector<bool> carrierFlags = findDepSupp(*(list_it->get()), graph);
				if (!std::any_of(carrierFlags.begin(), carrierFlags.end(), [](bool value) { return value; })) //check carrier existence
					continue; //if not, check next arc for carriers

				Montego::OccurrencePtr start = (*list_it)->getStartVertex().getSourceOccurrence();
				Montego::OccurrencePtr end = (*list_it)->getEndVertex().getSourceOccurrence();

				int start_index = indexOf(for_body_statement, start->getParentStatement());
				int end_index = indexOf(for_body_statement, end->getParentStatement());

#ifdef _DEBUG
				std::cout << start->getParentStatement()->dumpState() << " " << start_index << std::endl;
				std::cout << end->getParentStatement()->dumpState() << " " << end_index << std::endl;
#endif

				if (end_index < start_index) //increase value of all possible slices, which can be between verticies of dependence arc
				{
					for (int i = end_index; i < start_index; ++i)
					{
						slices_map::iterator it = slice_parameters.find(i);
						(it != slice_parameters.end()) ? ++(*it).second : slice_parameters[i] = 1;
					}
				}	
			}

			//find last operator with max slice parameter 
			int adviced_slice = -1;
			if (!slice_parameters.empty())
			{
				slice_pair max_pair(0, 0);
				std::for_each(slice_parameters.begin(), slice_parameters.end(), [&](slice_pair pair) { if (pair.second >= max_pair.second) max_pair = pair; });
				adviced_slice = max_pair.first;
			}
			return adviced_slice;
		}

		void LoopCycleOffset::makeTransformImpl(ProgramUnit* pProgram, const ArgumentValues& params)
		{
			// Basic validations

			// Check arguments
			OPS_ASSERT(pProgram != nullptr);
			OPS_ASSERT(getArgumentsInfo().validate(params));

			ForStatement* for_statement = dynamic_cast<ForStatement*>(params[0].getAsRepriseObject());

			OPS_ASSERT(for_statement != nullptr);

			BlockStatement* for_body_statement = &for_statement->getBody();

			int slice_index = checkDependencies(for_statement);
			OPS_ASSERT(slice_index != -1);
			
			RepriseBase* child = &for_body_statement->getChild(slice_index);
			OPS_ASSERT(child != nullptr);

			//if (slice_index == -1 || child == nullptr)
			//	return;

			std::vector<RepriseBase*> before_slice_operators;
			std::vector<RepriseBase*> after_slice_operators;

			// Copy statements before selected
			for (BlockStatement::Iterator it = for_body_statement->getFirst(); &(*it) <= child; ++it)
				before_slice_operators.push_back((*it).clone());

			// Remove from parent loop all copied statements before selected
			while (&*(for_body_statement->getFirst()) <= child)
				for_body_statement->erase(for_body_statement->getFirst());

			// Copy selected statement and all after it
			for (BlockStatement::Iterator it = for_body_statement->getFirst(); it != for_body_statement->getLast(); ++it)
				after_slice_operators.push_back((*it).clone());

			after_slice_operators.push_back((*for_body_statement->getLast()).clone());

			RepriseBase& counter = for_statement->getInitExpression().getChild(0);
			RepriseBase& start_value = for_statement->getInitExpression().getChild(1);
			RepriseBase& step = for_statement->getStepExpression().getChild(1);
			
			Replacer inner_replacer(counter, step);
			for_statement->getFinalExpression().accept(inner_replacer);

			// Add erased statements to end of loop
			for (std::vector<RepriseBase*>::iterator it = before_slice_operators.begin(); it != before_slice_operators.end(); ++it)
			{
				StatementBase* st = dynamic_cast<StatementBase*>(*it)->clone();
				st->accept(inner_replacer);
				for_body_statement->addLast(st);
			}

			BlockStatement& upperBlock = for_statement->getParentBlock();
			BlockStatement::Iterator for_statement_iterator = upperBlock.convertToIterator(for_statement);

			// Add statements before cycle
			Replacer outer_replacer(counter, start_value);
			for (std::vector<RepriseBase*>::iterator it = before_slice_operators.begin(); it != before_slice_operators.end(); ++it)
			{
				StatementBase* st = dynamic_cast<StatementBase*>(*it)->clone();
				st->accept(outer_replacer);
				upperBlock.addBefore(for_statement_iterator, st);
			}

			// Add statements after cycle
			for (std::vector<RepriseBase*>::reverse_iterator it = after_slice_operators.rbegin(); it != after_slice_operators.rend(); ++it)
				upperBlock.addAfter(for_statement_iterator, (*it)->clone()->cast_ptr<StatementBase>());
			
			//add final step to counter
			upperBlock.addAfter(for_statement_iterator, new ExpressionStatement(for_statement->getStepExpression().cast_ptr<ExpressionBase>()));
		}
	}
}
