// LoopFragmentation.cpp

#include <Transforms/Loops/LoopFragmentation/LoopFragmentation.h>
#include "Transforms/Loops/LoopNesting/LoopNesting.h"
#include "Transforms/Loops/LoopDistribution/LoopDistribution.h"
#include "Transforms/Loops/LoopInterchange/LoopInterchange.h"
#include "Reprise/Reprise.h"
#include "Shared/ExpressionHelpers.h"
#include "Reprise/Service/DeepWalker.h"
#include <vector>	// for params
#include <sstream>	// for BlockNestPragmaWalker
#include <iostream>	// for std::cerr in BlockNestPragmaWalker

//~ #include "Backends/OutToC/OutToC.h"
//~ #include <iostream>

using namespace OPS::Reprise;
using namespace OPS::Reprise::Editing;
using namespace OPS::Transforms::Loops;
using namespace OPS::Shared::ExpressionHelpers;

namespace OPS
{
namespace Transforms
{
namespace Loops
{


// //// Local functions

ForStatement* getParentLoop(ForStatement* loopPtr);
ForStatement* getChildLoop (ForStatement* loopPtr);
ForStatement& myLoopDistribution(ForStatement &loop);
void myMakeLoopInterchange(ForStatement& forStatement);
int getNestDepth(ForStatement* loopPtr, bool checkForCanonized);
ForStatement* extractFromBlock(BlockStatement &block, vector<ReprisePtr<VariableDeclaration> > &vars);


// //// Realization


OPS::Reprise::ForStatement&
loopFragmentation(OPS::Reprise::ForStatement &forStmt,
				  const std::vector<int> &params,
				  bool intoNewBlock,
				  bool disableDistributionCheck,
				  bool disableInterchangeCheck,
				  bool deleteAllTails)
{
    IntegerHelper intHelper(OPS::Reprise::BasicType::BT_INT32);
    vector<ReprisePtr<ExpressionBase> > exprParams;

    // convert params from integer into ReprisePtr<ExpressionBase> >
    for (int parNum = 0; parNum < (int)params.size(); parNum++)
    {
        ReprisePtr<ExpressionBase> exprParam(
                    new StrictLiteralExpression(intHelper(params[parNum])));
        exprParams.push_back(exprParam);
    }

    return loopFragmentation(forStmt, exprParams,
							 intoNewBlock,
							 disableDistributionCheck,
							 disableInterchangeCheck,
							 deleteAllTails);
}

OPS::Reprise::ForStatement&
loopFragmentation(OPS::Reprise::ForStatement &forStmt,
				  const std::vector<OPS::Reprise::ReprisePtr
						<OPS::Reprise::ExpressionBase> > &params,
				  bool intoNewBlock,
				  bool disableDistributionCheck,
				  bool disableInterchangeCheck,
				  bool deleteAllTails)
{
    size_t nestDepth = getNestDepth(&forStmt);
    size_t dimCt = params.size();	// count of dimentions
    
	if (dimCt == 0)
		throw LoopFragmentationException("Try to transforn without parameters.");
		
    if (nestDepth < dimCt)
		throw LoopFragmentationException("Loop nest depth is less then number of dimentions.");

	// go to outer loop
	ForStatement* loopPtr = &forStmt;
	
    for (size_t loopNum = 0; loopNum < nestDepth - dimCt; ++loopNum)
        loopPtr = getChildLoop(loopPtr);
	
	ReprisePtr<ForStatement> originLoopRPtr(loopPtr);

	// create outer block to encapsulate new loops
	ReprisePtr<BlockStatement> newBlockRPtr(new BlockStatement());
	// and copy origin loop to it
	ReprisePtr<ForStatement> copyLoopRPtr(originLoopRPtr->clone());
	newBlockRPtr->addFirst(copyLoopRPtr->cast_ptr<StatementBase>());
	vector<ReprisePtr<VariableDeclaration> > vars;
	
	// replace origin loop on new block
	BlockStatement &parentBlock = originLoopRPtr->getParentBlock();
	parentBlock.replace(parentBlock.convertToIterator(originLoopRPtr->cast_ptr<StatementBase>()),
						newBlockRPtr.get());
	
	// find the most inner loop
    loopPtr = copyLoopRPtr.get();
    for (size_t loopNum = 0; loopNum < dimCt - 1; loopNum++)
        loopPtr = getChildLoop(loopPtr);

	ForStatement* outerLoopPtr = NULL;
	
	try
	{
	
	// step 1 - apply loopNesting and loopDistribution to loop nest
	// go from inner to outer loop
	for (size_t depth = dimCt; depth > 0; --depth)
	{
		if (!canApplyLoopNestingTo(*loopPtr))
			throw LoopFragmentationException("Can't apply loopNesting.");

		ReprisePtr<ExpressionBase> borderRPtr(getBasicForFinalExpression(*loopPtr).clone());

		// apply LoopNesting
		BlockStatement &blockStmt = makeLoopNesting(*loopPtr, *(params[depth - 1]));

		// delete tail if necessary
		bool canDeleteTail = !deleteAllTails;
		// if tail exists
		canDeleteTail = blockStmt.getChildCount() == 2;
		// and final expression is an intrger literal constant
		canDeleteTail &= borderRPtr->is_a<StrictLiteralExpression>() &&
			borderRPtr->cast_to<StrictLiteralExpression>().isInteger();
		// and step is an intrger literal constant
		canDeleteTail &= params[depth - 1]->is_a<StrictLiteralExpression>() &&
			params[depth - 1]->cast_to<StrictLiteralExpression>().isInteger();

		long borderVal = (canDeleteTail) ? borderRPtr->cast_to<StrictLiteralExpression>().getInt32() : 0;
		long stepVal = (canDeleteTail)
						? params[depth - 1]->cast_to<StrictLiteralExpression>().getInt32() : 0;

		canDeleteTail &= (stepVal != 0) && (borderVal % stepVal == 0);

		if (canDeleteTail || deleteAllTails)
			blockStmt.erase(&blockStmt.getChild(1).cast_to<StatementBase>());
		
		
		loopPtr = extractFromBlock(blockStmt, vars);

		// apply LoopDistribution
		ForStatement* loopPtr2 = loopPtr;
		
		for (size_t depth2 = depth - 1; depth2 > 0; --depth2)
		{
			loopPtr2 = getParentLoop(loopPtr2);

			if (disableDistributionCheck)
				loopPtr2 = &myLoopDistribution(*loopPtr2);
			else if (!LoopDistribution(loopPtr2))
				throw LoopFragmentationException("Can't apply loopDistribution.");
		}
		
		if (depth == 1)
			 outerLoopPtr = loopPtr;
		else loopPtr = getParentLoop(loopPtr);
	}
	//~ throw LoopFragmentationException("Test exception.");
	
	// Step 2 - apply loopInterchange
	if (outerLoopPtr == NULL)
		throw LoopFragmentationException("Internal error.");
	
	loopPtr = outerLoopPtr;

	size_t depth = 1;	// Depth of moving head
	for (; depth < dimCt * 2 - 1; ++depth)
		loopPtr = getChildLoop(loopPtr);

	for (size_t loopNum = 0; loopNum < dimCt; ++loopNum)
	{
		ForStatement* parLoopPtr = loopPtr;
		
		// Move head
		for (size_t depth2 = depth - 1; depth2 > 0; --depth2)   // Swap loops heads
		{
			if (!disableInterchangeCheck && !canApplyLoopInterchangeTo(*parLoopPtr))
				throw LoopFragmentationException("Can't apply loopInterchange.");
			
			parLoopPtr = getParentLoop(parLoopPtr);
			myMakeLoopInterchange(*parLoopPtr);
		}
		depth--;

		if (depth > 0) loopPtr = getParentLoop(loopPtr);
	}
	
	// pointer to outer loop for return
	outerLoopPtr = newBlockRPtr->getChild(0).cast_ptr<ForStatement>();
	
	// in case of error restore original loop
	}
	catch (LoopFragmentationException exception)
	{
		parentBlock.replace(parentBlock.convertToIterator(newBlockRPtr->cast_ptr<StatementBase>()),
							originLoopRPtr.get());
		throw exception;
	}

	if (intoNewBlock)
	{
		// move varDeclarations in new block
		for (size_t loopNum = 0; loopNum < dimCt; ++loopNum)
		{
			Declarations::VarIterator iter = newBlockRPtr->getDeclarations().getFirstVar();

			while (iter.isValid() &&
				   (!iter->hasDefinedBlock()
					   || &iter->getDefinedBlock() != newBlockRPtr.get()
					   || iter->getName() != vars[loopNum]->getName()))
				iter++;

			if (!iter.isValid())
				vars[loopNum]->setDefinedBlock(*newBlockRPtr);
		}
	}
	else
	{
		extractFromBlock(*newBlockRPtr, vars);
		
		// move varDeclarations in parent block
		for (size_t loopNum = 0; loopNum < dimCt; ++loopNum)
		{
			Declarations::VarIterator iter = parentBlock.getDeclarations().getFirstVar();
	
			while (iter.isValid() &&
				   (!iter->hasDefinedBlock()
					   || &iter->getDefinedBlock() != &parentBlock
					   || iter->getName() != vars[loopNum]->getName()))
				iter++;
	
			if (!iter.isValid())
				vars[loopNum]->setDefinedBlock(parentBlock);
		}
	}
	
	return *outerLoopPtr;
}



ForStatement* getParentLoop(ForStatement* loopPtr)
{
    //if (loopPtr->getParentBlock().getParent()->is_a<ForStatement>())
        return &loopPtr->getParentBlock().getParent()->cast_to<ForStatement>();
    //else return 0;
}
ForStatement*  getChildLoop(ForStatement* loopPtr)
{
    return &loopPtr->getBody().getChild(0).cast_to<ForStatement>();
}

ForStatement &myLoopDistribution(ForStatement &loop)
{   //Loop distribution without analysis
    if (loop.getBody().getChildCount() != 2)
        return loop;

    BlockStatement &parent = loop.getParentBlock();

    ReprisePtr<ForStatement> loop2Ptr(loop.clone());

    BlockStatement::Iterator iter = parent.convertToIterator(&loop);
    parent.addAfter(iter, loop2Ptr.get());

    loop.getBody().erase(&loop.getBody().getChild(1).cast_to<StatementBase>());
    loop2Ptr->getBody().erase(&loop2Ptr->getBody().getChild(0).cast_to<StatementBase>());

    return loop;
}

void myMakeLoopInterchange(ForStatement& forStatement)
{
    ForStatement* innerFor = forStatement.getBody().getFirst()->cast_ptr<ForStatement>();
    OPS_ASSERT(innerFor != 0);
    ReprisePtr<ExpressionBase> initExpr = OPS::Reprise::Editing::replaceExpression(
        forStatement.getInitExpression(), ReprisePtr<ExpressionBase>(&innerFor->getInitExpression()));
    ReprisePtr<ExpressionBase> finalExpr = OPS::Reprise::Editing::replaceExpression(
        forStatement.getFinalExpression(), ReprisePtr<ExpressionBase>(&innerFor->getFinalExpression()));
    ReprisePtr<ExpressionBase> stepExpr = OPS::Reprise::Editing::replaceExpression(
        forStatement.getStepExpression(), ReprisePtr<ExpressionBase>(&innerFor->getStepExpression()));

    //OPS::Reprise::Editing::replaceExpression(innerFor->getInitExpression(), initExpr);
    //OPS::Reprise::Editing::replaceExpression(innerFor->getFinalExpression(), finalExpr);
    //OPS::Reprise::Editing::replaceExpression(innerFor->getStepExpression(), stepExpr);
    innerFor->setInitExpression(initExpr.get());
    innerFor->setFinalExpression(finalExpr.get());
    innerFor->setStepExpression(stepExpr.get());
}

//Test for canonized or basic forStatements in nest
int getNestDepth(ForStatement* loopPtr, bool checkForCanonized)
{
    if (checkForCanonized)
    {
        if (!Editing::forHeaderIsCanonized(*loopPtr)) return 0;
    }
    else
        if (!Editing::forIsBasic(*loopPtr)) return 0;

    int depth = 1;
    bool flag = true;

    while (flag)
    {
        flag = loopPtr->getBody().getChildCount() == 1
                && loopPtr->getBody().getChild(0).is_a<ForStatement>();

        if (!flag) return depth;

        loopPtr = getChildLoop(loopPtr);
        flag = (checkForCanonized && Editing::forHeaderIsCanonized(*loopPtr))
               || (!checkForCanonized && Editing::forIsBasic(*loopPtr));

        if (!flag) return depth;
        depth++;
    }
    return 0;
}

ForStatement* extractFromBlock(BlockStatement &block, vector<ReprisePtr<VariableDeclaration> > &vars)
{   //Extracting from BlockStatement after LoopNesting transform.
    BlockStatement &parent = block.getParentBlock();
    ForStatement* firstLoop = &block.getChild(0).cast_to<ForStatement>();

    //move variable declaration
    VariableDeclaration &var = firstLoop->getInitExpression().
            cast_to<BasicCallExpression>().getArgument(0).
            cast_to<ReferenceExpression>().getReference();
    //varPtr.setDefinedBlock(globalParent);
    vars.push_back(ReprisePtr<VariableDeclaration>(&var));

    //move main and tail loops
    BlockStatement::Iterator insertIter = parent.convertToIterator(&block);
    while (!block.isEmpty())
    {
        ReprisePtr<StatementBase> st(&(*block.getFirst()));
        block.erase(st.get());
        parent.addBefore(insertIter, st.get());
    }

    //Delete empty block
    parent.erase(insertIter);
    return firstLoop;
}



// === BlockNestPragmaWalker ===


void BlockNestPragmaWalker::visit(OPS::Reprise::BlockStatement& blockStmt)
{
	for (int childNum = 0; childNum < blockStmt.getChildCount(); ++childNum)
	{
		StatementBase& stmt = blockStmt.getChild(childNum)
								.cast_to<StatementBase>();
		if (stmt.hasNote(PRAGMA_NAME))
			transform(stmt);
	}
	
	for (int childNum = 0; childNum < blockStmt.getChildCount(); ++childNum)
	{
		StatementBase& stmt = blockStmt.getChild(childNum)
								.cast_to<StatementBase>();
		stmt.accept(*this);
	}
}


StatementBase& BlockNestPragmaWalker::transform(StatementBase &stmt)
{
	try
	{
		if (!stmt.hasNote(PRAGMA_NAME))
			throw LoopFragmentationException("Current statement hasn't "
											 "pragma 'block_nest'.");
		
		if (!stmt.is_a<ForStatement>())
			throw LoopFragmentationException("Pragma 'block_nest' "
				"can be applied to close nest of ForStatement only.");
		
		// read pragma parameters
		std::string pragmaParams = stmt.getNote("block_nest").getString();
		stmt.removeNote(PRAGMA_NAME);
		std::stringstream paramStream(pragmaParams);
		
		// read block sizes
		size_t blockCt;
		paramStream >> blockCt;
		std::vector<int> params(blockCt);
		
		for (size_t blockNum = 0; blockNum < blockCt; ++blockNum)
			paramStream >> params[blockNum];
		
		// read additional parameters
		bool intoNewBlock, disDistrCheck, disInterCheck, deleteTails;
			 
		paramStream >> intoNewBlock
					>> disDistrCheck >> disInterCheck >> deleteTails;
		
		// try to apply loopFragmentation transform
		ForStatement &forStmt = stmt.cast_to<ForStatement>();
		ForStatement &newforStmt
			= loopFragmentation(forStmt, params, intoNewBlock,
								disDistrCheck, disInterCheck, deleteTails);

		//~ std::cout << "done.\n";
		return newforStmt;
	}
	catch (LoopFragmentationException exception)
	{
		std::cerr << "BlockNestPragmaWalker: "
				  << exception.getMessage() + "\n";
		errorFlag = true;
	}
	catch (RepriseError exception)
	{
		std::cerr << "BlockNestPragmaWalker: "
				  << "OPS Reprise exception: " + exception.getMessage() + "\n";
		errorFlag = true;
	}
	catch (Exception exception)
	{
		std::cerr << "BlockNestPragmaWalker: "
				  << "OPS exception: " + exception.getMessage() + "\n";
		errorFlag = true;
	}
	
	return stmt;
}



}   //Loops
}   //Transforms
}   //OPS

