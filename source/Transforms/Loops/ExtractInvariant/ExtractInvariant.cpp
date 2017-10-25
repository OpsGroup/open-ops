#include <Reprise/Reprise.h>
#include <Analysis/ComplexOccurrenceAnalysis/GrouppedOccurrences.h>
#include <Transforms/Loops/ExtractInvariant/ExtractInvariant.h>
#include <list>
#include <map>

using namespace OPS::Reprise;

namespace OPS
{
namespace Analysis
{
	/* From:
		for(i=...)
			A[j] = A[j] + B[i][j];
	   To:
		int tmp = A[j];
		for(i=...)
			tmp = tmp + B[i][j];
		A[j] = tmp;  */

void extractInvariant(Reprise::BasicCallExpression* expressionForReplace, Reprise::ForStatement* target)
{
    std::map<const Reprise::VariableDeclaration*, TopLevelOccurrenceList> Occurrences = 
									findAllTopLevelOccurrences(*target, Analysis::GBT_READ); 
	
	ReferenceExpression& varRef = expressionForReplace->getArgument(0).cast_to<ReferenceExpression>();   
	TopLevelOccurrenceList& ListOfMatches = Occurrences[&varRef.getReference()]; // list with occurrences of a

	VariableDeclaration* tmp = new VariableDeclaration(BasicType::int32Type(), "tmp"); // declaration of tmp
	tmp->setDefinedBlock(target->getParentBlock());
	SubroutineDeclaration& subroutine = target->getRootBlock().getParent()->cast_to<SubroutineDeclaration>();
	subroutine.getDeclarations().addVariable(tmp); 

	ReprisePtr<ReferenceExpression> pTmp(new ReferenceExpression(*tmp)); // building of first assign
	BasicCallExpression* assign = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN);
	assign->addArgument(pTmp -> clone());
	assign->addArgument(expressionForReplace->clone());
	
	ExpressionStatement* stmt = new ExpressionStatement(assign); // connection of first assign and ForStatement
	BlockStatement::Iterator itBlock = target->getParentBlock().convertToIterator(target);
	target->getParentBlock().addBefore(itBlock, stmt); 

	TopLevelOccurrenceList::iterator it = ListOfMatches.begin();
	for(; it != ListOfMatches.end(); it++) // passage through the list and replacing tmp
    {
		if ( (*it)->cast_to<BasicCallExpression>().isEqual(*expressionForReplace))
			Editing::replaceExpression(const_cast<ExpressionBase&>(*(*it)), pTmp);
    }

	Occurrences.clear();
    Occurrences = findAllTopLevelOccurrences(*target, Analysis::GBT_WRITE); 
	TopLevelOccurrenceList& ListOfMatches1 = Occurrences[&varRef.getReference()]; // list of occurrences that were changed

	it = ListOfMatches1.begin();
	bool flag = 0;
	for(; it != ListOfMatches1.end(); it++) //if such list doesn't empty then passage through thw list and replacing tmp
    {
		 if ( (*it)->cast_to<BasicCallExpression>().isEqual(*expressionForReplace))
		 {
			Editing::replaceExpression(const_cast<ExpressionBase&>(*(*it)), pTmp);
			flag = 1; // there are occurrences that were changed
		 }
    }

	if (flag) // building of second assign
	{
		BasicCallExpression* res = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN);
		res->addArgument(expressionForReplace -> clone());
		res->addArgument(pTmp->clone());

		stmt = new ExpressionStatement(res); // connection of second assign and ForStatement
		BlockStatement::Iterator itBlock1 = target->getRootBlock().convertToIterator(target);
		target->getRootBlock().addAfter(itBlock1, stmt);
	};
	       
}
}
} 
