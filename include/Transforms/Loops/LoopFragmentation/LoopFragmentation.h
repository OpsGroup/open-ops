/* LoopFragmentation.h
LoopFragmentation transform. Realized by Rogatov Kirill kirill4444@bk.ru */

#ifndef LOOP_FRAGMENTATION
#define LOOP_FRAGMENTATION

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include <vector>

namespace OPS
{
namespace Transforms
{
namespace Loops
{	

/*
loopFragmentation - makes loop talling transform
					for close nest of canonized loops.
Returns transformed loop nest.
In case of error generates LoopFragmentationException
and restores original loop nest.

Parameters:
forStmt - outer loop in close nest of canonized loops;
params - list of block sizes. It can be integer numbers
		 or any Reprise expressions;
intoNewBlock - result loops will be in new block;

WARNING: Following parameters can make not equivalent program!

disableDistributionCheck 	- disable check of loopDistribution
disableInterchangeCheck		- disable check of loopInterchange
deleteAllTails	- not generate additional loops for rest of iteration space
*/


OPS::Reprise::ForStatement&
loopFragmentation(OPS::Reprise::ForStatement &forStmt,
				  const std::vector<int> &params,
				  bool intoNewBlock = false,
				  bool disableDistributionCheck = false,
				  bool disableInterchangeCheck = false,
				  bool deleteAllTails = false);

OPS::Reprise::ForStatement&
loopFragmentation(OPS::Reprise::ForStatement &forStmt,
				  const std::vector<OPS::Reprise::ReprisePtr
						<OPS::Reprise::ExpressionBase> > &params,
				  bool intoNewBlock = false,
				  bool disableDistributionCheck = false,
				  bool disableInterchangeCheck = false,
				  bool deleteAllTails = false);



int getNestDepth(OPS::Reprise::ForStatement* loopPtr,
				bool checkForCanonized = true);


class LoopFragmentationException: public OPS::Exception
{
public:
	LoopFragmentationException(std::string message)
		: OPS::Exception("LoopFragmentationException: " + message) {}
};


/*
BlockNestPragmaWalker - processes pragmas block_nest(<blocks sizes>)

additional pragma parameters:
INTO_NEW_BLOCK	- result loops will be in new block
WARNING: Following parameters can make not equivalent program!
DISABLE_DISTRIBUTION_CHECK 	- disable check of loopDistribution
DISABLE_INTERCHANGE_CHECK	- disable check of loopInterchange
DISABLE_ALL_CHECKS	- disable both loopDistribution and loopInterchange checks
DELETE_TAILS	- not generate additional loops for rest of iteration space
*/

const std::string PRAGMA_NAME = "block_nest";

class BlockNestPragmaWalker: public OPS::Reprise::Service::DeepWalker
{
public:

	OPS::Reprise::StatementBase& transform(OPS::Reprise::StatementBase &stmt);

	#define VISIT_STMT(StmtType)                                               \
		void visit(StmtType& node)                                             \
		{                                                                      \
			if (node.hasNote(PRAGMA_NAME))                                     \
			{                                                                  \
				OPS::Reprise::StatementBase& stmt = transform(node);           \
				stmt.accept(*this);                                            \
			}                                                                  \
			else                                                               \
				OPS::Reprise::Service::DeepWalker::visit(node);                \
		}

	void visit(OPS::Reprise::BlockStatement& blockStmt);
	
	VISIT_STMT(OPS::Reprise::ForStatement)
	VISIT_STMT(OPS::Reprise::WhileStatement)
	VISIT_STMT(OPS::Reprise::IfStatement)
	VISIT_STMT(OPS::Reprise::GotoStatement)
	VISIT_STMT(OPS::Reprise::ReturnStatement)
    VISIT_STMT(OPS::Reprise::Canto::HirBreakStatement)
    VISIT_STMT(OPS::Reprise::Canto::HirContinueStatement)
	VISIT_STMT(OPS::Reprise::ExpressionStatement)
	VISIT_STMT(OPS::Reprise::EmptyStatement)

	bool hasErrors() const { return errorFlag; }

	BlockNestPragmaWalker(): errorFlag(false) {}

private:
	bool errorFlag;
};


}
}
}
#endif
