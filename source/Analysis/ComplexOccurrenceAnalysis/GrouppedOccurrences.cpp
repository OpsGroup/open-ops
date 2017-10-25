#include "Analysis/ComplexOccurrenceAnalysis/GrouppedOccurrences.h"

#include <Analysis/Renewal/OccurrenceAnalysis/IOccurrenceAnalysisService.h>

#include <Reprise/Declarations.h>
#include <Reprise/Expressions.h>
#include <Reprise/Statements.h>

#include <OPS_Core/Helpers.h>

using namespace OPS::Renewal;
using namespace OPS::Reprise;

namespace OPS
{

namespace Analysis
{

static int calculateBracketsNestingLevel(const ExpressionBase& expression)
{
	int level = 0;

	RepriseBase* parrent = expression.getParent();

	while (parrent != NULL && parrent->is_a<ExpressionBase>())
	{
		if (parrent->is_a<BasicCallExpression>() &&
			parrent->cast_to<BasicCallExpression>().getKind() ==
				BasicCallExpression::BCK_ARRAY_ACCESS)
		{
			++level;
		}

		parrent = parrent->getParent();
	}

	return level;
}

static const VariableDeclaration* findAssociatedVariableDeclaration(
	const ExpressionBase& expression)
{
	using namespace OPS::Reprise;

	const VariableDeclaration* declaration = NULL;

	ExpressionBase* declarationSource = &const_cast<ExpressionBase&>(expression);

	for (;;)
	{
		if (declarationSource->is_a<ReferenceExpression>())
		{
			declaration = &declarationSource->cast_to<ReferenceExpression>().getReference();
		}

		if (declarationSource->getChildCount() == 0)
		{
			break;
		}

		declarationSource = &declarationSource->getChild(0).cast_to<ExpressionBase>();
	}

	return declaration;
}

static AbstractOccurrence::Type grouppedByTypeToOccurrenceType(GrouppedByType grouppedBy)
{
	switch (grouppedBy)
	{
	case GBT_READ:
		{
			return AbstractOccurrence::T_USSAGE;
		}
	case GBT_WRITE:
		{
			return AbstractOccurrence::T_GENERATOR;
		}
	default:
		{
			OPS_ASSERT(!"Unexpected GrouppedByType");

			break;
		}
	}

	return static_cast<AbstractOccurrence::Type>(-1);
}

OccurrencesByDeclarations findAllTopLevelOccurrences(const StatementBase& statement,
	GrouppedByType grouppedBy)
{
	OccurrencesByDeclarations occurrencesByDeclarations;

	// find all occurrences

	getOccurrenceAnalysisService().notifyContextChanged(statement);

	const Occurrences occurrences = getOccurrenceAnalysisService().findAllOccurrences(statement);

	// determine occurrence type

	const AbstractOccurrence::Type occurrenceType = grouppedByTypeToOccurrenceType(grouppedBy);

	// process occurrences

	for (Occurrences::const_iterator it = occurrences.begin(); it != occurrences.end(); ++it)
	{
		OccurrencePtr occurrence = *it;

		OPS_ASSERT(occurrence != NULL);

		if (calculateBracketsNestingLevel(occurrence->getOccurrenceNode()) == 0 &&
			occurrence->getType() == occurrenceType)
		{
			const VariableDeclaration* const declaration =
				findAssociatedVariableDeclaration(occurrence->getOccurrenceNode());

			occurrencesByDeclarations[declaration].push_back(&occurrence->getOccurrenceNode());
		}
	}

	return occurrencesByDeclarations;
}

}

}
